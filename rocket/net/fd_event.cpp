//
// Created by 20771 on 2024/1/12.
//
#include "fd_event.h"

namespace rocket {

static FdEventGroup* g_fd_event_group = nullptr;

FdEventGroup* FdEventGroup::GetFdEventGroup() {
    if (g_fd_event_group != nullptr) {
        return g_fd_event_group;
    }

    g_fd_event_group = new FdEventGroup(128);
    return g_fd_event_group;
}

void FdEvent::setNonBlock() const {
    int flag = fcntl(m_fd, F_GETFL);
    if (flag & O_NONBLOCK)
        return;
    fcntl(m_fd, F_SETFL, flag | O_NONBLOCK);
}

FdEvent::Fn FdEvent::handler(TriggerEvent event_type) {
    switch (event_type) {
        case IN_EVENT:
            return m_read_callback;
        case OUT_EVENT:
            return m_write_callback;
        case ERROR_EVENT:
            return m_error_callback;
        default:
            return nullptr;
    }
}

void FdEvent::setEvent(TriggerEvent event_type, const Fn &call_back,
                       bool is_et, const Fn &error_call_back) {
    if(event_type == IN_EVENT) {
        m_event.events |= EPOLLIN;
        m_read_callback = call_back;
    }
    else if (event_type == OUT_EVENT) {
        m_event.events |= EPOLLOUT;
        m_write_callback = call_back;
    }
    if(is_et)
        m_event.events |= EPOLLET;

    if(error_call_back == nullptr)
        m_error_callback = nullptr;
    else
        m_error_callback = error_call_back;

    m_event.data.ptr = this; // 将本文件描述符管理指针传给 m_event.data.ptr 用于后续加入epoll内核事件表中
}

void FdEvent::cancelEvent(TriggerEvent event_type, bool is_et) {
    if(event_type == IN_EVENT)
        m_event.events &= (~EPOLLIN);
    else if(event_type == OUT_EVENT)
        m_event.events &= (~EPOLLOUT);

    if(is_et) m_event.events &= (~EPOLLET);
}



FdEventGroup::FdEventGroup(int size): m_size(size) {
    for (int i = 0; i < m_size; ++i) {
        m_fd_group.push_back(new FdEvent(i));
    }
}

FdEventGroup::~FdEventGroup() {
    for (int i = 0; i < m_size; ++i) {
        if (m_fd_group[i] != nullptr) {
            delete m_fd_group[i];
            m_fd_group[i] = nullptr;
        }
    }
}

FdEvent* FdEventGroup::getFdEvent(int fd) {
    ScopeMutex<Mutex> locker(m_mutex);
    if(fd < m_fd_group.size())
        return m_fd_group[fd];

    int new_size = static_cast<int>(fd * 1.5);
    for(auto i = m_fd_group.size(); i < new_size; ++i)
        m_fd_group.push_back(new FdEvent(static_cast<int>(i)));

    return m_fd_group[fd];
}



WakeUpFdEvent::WakeUpFdEvent(): FdEvent() {
    m_fd = eventfd(0, EFD_NONBLOCK); // 创建wakeup_fd并设置为非阻塞
    if (m_fd < 0) {
        ERRORLOG("failed to create event loop, eventfd create error, error info[%d]", errno)
        exit(0);
    }
    INFOLOG("wakeup fd[%d]", m_fd)

    auto cb = [this]() { // 定义回调函数
        char buf[8];
        while(read(m_fd, buf, 8) != -1 && errno != EAGAIN) {}
        DEBUGLOG("read full bytes from wakeup fd[%d]", m_fd)
    };
    setEvent(FdEvent::IN_EVENT, cb); // 设置wakeup事件为读就绪同时传入对应的回调函数
}

void WakeUpFdEvent::wakeup() {
    char buf[8] = {'a'};
    auto ret = write(m_fd, buf, 8);
    if(ret != 8)
        ERRORLOG("write to wakeup fd less than 8 bytes, fd[%d]", m_fd)
    DEBUGLOG("success write %d bytes", 8)
}

}

