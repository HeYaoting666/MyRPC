//
// Created by 20771 on 2024/1/12.
//
#include "eventloop.h"

namespace rocket {

static thread_local EventLoop* t_current_eventloop = nullptr;

EventLoop *EventLoop::GetCurrentEventLoop() {
    if (t_current_eventloop) {
        return t_current_eventloop;
    }
    t_current_eventloop = new EventLoop();
    return t_current_eventloop;
}

rocket::EventLoop::EventLoop():
    m_stop_flag(false),
    m_is_looping(false),
    m_wakeup_fd_event(nullptr),
    m_timer(nullptr)
{
    if(t_current_eventloop != nullptr) {
        ERRORLOG("failed to create event loop, this thread %d has created event loop", m_thread_id)
        exit(0);
    }

    m_thread_id = getThreadId();
    m_epoll_fd = epoll_create(10);
    if(m_epoll_fd == -1) {
        ERRORLOG("failed to create event loop, epoll_create error, error info[%d]", errno)
        exit(0);
    }

    // 初始化 wakeup 和 timer
    initWakeUpFdEevent();
    initTimer();

    INFOLOG("success create event loop in thread[%d]", m_thread_id)
    t_current_eventloop = this;
}

EventLoop::~EventLoop() {
    close(m_epoll_fd);
    if(m_wakeup_fd_event) {
        delete m_wakeup_fd_event;
        m_wakeup_fd_event = nullptr;
    }
    if(m_timer) {
        delete m_timer;
        m_timer = nullptr;
    }
}

void EventLoop::loop() {
    m_is_looping = true;
    epoll_event events[MAX_EVENT_NUMBER];

    while(!m_stop_flag) {
        // 从队列中取出需要执行的任务
        ScopeMutex<Mutex> locker(m_mutex);
        std::queue< std::function<void()> > tmp_tasks;
        m_pending_tasks.swap(tmp_tasks);
        locker.unlock();

        // 执行从队列中取出的任务
        while(!tmp_tasks.empty()) {
            auto call_back = tmp_tasks.front();
            tmp_tasks.pop();

            if(call_back) call_back();
        }

//        int time_out = MAX_TIMEOUT;
        int time_out = -1;
//        DEBUGLOG("%s", "now begin to epoll_wait")
        int num_events = epoll_wait(m_epoll_fd, events, MAX_EVENT_NUMBER, time_out);
//        DEBUGLOG("now end epoll_wait, num_events = %d", num_events)
        if (num_events < 0 && errno != EINTR) {
            ERRORLOG("epoll_wait error, errno=%d, error=%s", errno, strerror(errno))
            continue;
        }

        for(int i = 0; i < num_events; ++i) {
            epoll_event trigger_event = events[i];
            auto fd_event = static_cast<FdEvent*>(trigger_event.data.ptr);
            if(!fd_event) {
                ERRORLOG("%s", "fd_event = NULL, continue")
                continue;
            }

            if(trigger_event.events & EPOLLIN) {
                DEBUGLOG("fd[%d] trigger EPOLLIN event", fd_event->getFd())
                addTask(fd_event->handler(FdEvent::IN_EVENT));
            }
            if(trigger_event.events & EPOLLOUT) {
                DEBUGLOG("fd[%d] trigger EPOLLOUT event", fd_event->getFd())
                addTask(fd_event->handler(FdEvent::OUT_EVENT));
            }
            if(trigger_event.events & EPOLLERR) {
                DEBUGLOG("fd[%d] trigger EPOLLERROR event", fd_event->getFd())
                if(fd_event->handler(FdEvent::ERROR_EVENT) != nullptr) {
                    DEBUGLOG("fd[%d] add error callback", fd_event->getFd())
                    addTask(fd_event->handler(FdEvent::ERROR_EVENT));
                }
                deleteEpollEvent(fd_event); // 删除出错的套接字
            }
        }
    }
}

void EventLoop::wakeup() {
    INFOLOG("%s", "WAKE UP")
    m_wakeup_fd_event->wakeup();
}

void EventLoop::stop() {
    m_stop_flag = true;
    wakeup();
}

void EventLoop::addEpollEvent(FdEvent* event) {
    if(isInLoopThread()){
        ADD_TO_EPOLL()
    }
    else {
        // 当前操作的线程与event_loop所属线程不同，将任务加入本event_loop的任务队列中
        // 通过wakeup唤醒另一线程的event_loop
        auto call_back = [this, event]() { ADD_TO_EPOLL() };
        addTask(call_back, true);
    }
}

void EventLoop::deleteEpollEvent(FdEvent *event) {
    if (isInLoopThread()) {
        DELETE_TO_EPOLL()
    }
    else {
        // 当前操作的线程与本event_loop所属线程不同，将任务加入本event_loop的任务队列中
        // 通过wakeup唤醒另一线程的event_loop
        auto call_back = [this, event]() { DELETE_TO_EPOLL() };
        addTask(call_back, true);
    }
}

void EventLoop::addTimerEvent(const TimerEvent::spointer& event) {
    m_timer->addTimerEvent(event);
    INFOLOG("%s", "addTimerEvent success")
}


void EventLoop::initWakeUpFdEevent() {
    m_wakeup_fd_event = new WakeUpFdEvent(); // 创建对应event
    addEpollEvent(m_wakeup_fd_event);  // 将wakeup事件注册至epoll内核事件表中
}

void EventLoop::initTimer() {
    m_timer = new Timer();        // 创建对应event
    addEpollEvent(m_timer);  // 将timer事件注册至epoll内核事件表中
}

void EventLoop::addTask(const EventLoop::Fn& cb, bool is_wake_up /*=false*/) {
    ScopeMutex<Mutex> locker(m_mutex);
    m_pending_tasks.push(cb);
    locker.unlock();

    if(is_wake_up)
        wakeup();
}


}