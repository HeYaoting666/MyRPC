//
// Created by 20771 on 2024/1/12.
//

#ifndef ROCKET_FD_EVENT_H
#define ROCKET_FD_EVENT_H

#include <functional>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include "../common/log.h"

namespace rocket {

/************************************
 *    文件描述符事件管理 FdEvent
 *    管理事件 fd, event 和对应的回调函数
 *
 *    FdEventGroup 用于管理一组 FdEvent
 ************************************/
class FdEvent {
public:
    typedef std::function<void()> Fn;
    enum TriggerEvent {
        IN_EVENT = EPOLLIN,
        OUT_EVENT = EPOLLOUT,
        ERROR_EVENT = EPOLLERR,
    };

protected:
    int m_fd {-1};
    epoll_event m_event {};

    Fn m_read_callback;
    Fn m_write_callback;
    Fn m_error_callback;

public:
    FdEvent() = default;
    explicit FdEvent(int fd): m_fd(fd) { }

    ~FdEvent() = default;

public:
    int getFd() const { return m_fd; }
    epoll_event getEpollEvent() const { return m_event; }

    void setNonBlock() const;

    Fn handler(TriggerEvent event_type);

    // 设置监听事件和相应的回调函数，用于后续epoll事件注册
    void setEvent(TriggerEvent event_type, const Fn &call_back,
                  bool is_et = true, const Fn &error_call_back = nullptr);

    // 取消监听事件
    void cancelEvent(TriggerEvent event_type, bool is_et = true);
};

class FdEventGroup {
private:
    int m_size;
    std::vector<FdEvent*> m_fd_group;
    Mutex m_mutex;

public:
    explicit FdEventGroup(int size);
    ~FdEventGroup();

public:
    FdEvent* getFdEvent(int fd);

public:
    static FdEventGroup* GetFdEventGroup();
};



/************************************
 *  WakeUpFdEvent 用于唤醒 epoll_wait
 ************************************/
class WakeUpFdEvent : public FdEvent {
public:
    WakeUpFdEvent();
    ~WakeUpFdEvent() = default;

public:
    void wakeup();
};

}

#endif //ROCKET_FD_EVENT_H
