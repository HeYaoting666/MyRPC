//
// Created by 20771 on 2024/1/12.
//

#ifndef ROCKET_EVENTLOOP_H
#define ROCKET_EVENTLOOP_H

#include <pthread.h>
#include <set>
#include <functional>
#include <sys/epoll.h>

#include "fd_event.h"
#include "timer.h"


#define ADD_TO_EPOLL() \
    auto it = m_listen_fds.find(event->getFd()); \
    int op = EPOLL_CTL_ADD; \
    if (it != m_listen_fds.end()) { \
        op = EPOLL_CTL_MOD; \
    }                  \
    epoll_event tmp = event->getEpollEvent(); \
    INFOLOG("epoll_event.events = %d", (int)tmp.events); \
    int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp); \
    if (rt == -1) { \
        ERRORLOG("failed epoll_ctl when add fd[%d], errno=%d, error=%s", event->getFd(), errno, strerror(errno)); \
        exit(0); \
    } \
    m_listen_fds.insert(event->getFd()); \
    DEBUGLOG("add event success, fd[%d]", event->getFd())\

#define DELETE_TO_EPOLL() \
    auto it = m_listen_fds.find(event->getFd()); \
    if (it == m_listen_fds.end()) { \
        return; \
    } \
    int op = EPOLL_CTL_DEL; \
    epoll_event tmp = event->getEpollEvent(); \
    int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), nullptr); \
    if (rt == -1) { \
        ERRORLOG("failed epoll_ctl when add fd[%d], errno=%d, error=%s",  event->getFd(), errno, strerror(errno)); \
        exit(0); \
    } \
    m_listen_fds.erase(event->getFd()); \
    DEBUGLOG("delete event success, fd[%d]", event->getFd()); \


namespace rocket {


/**
 * @brief EventLoop IO事件管理
 */
class EventLoop {
public:
    typedef std::function<void()> Fn;
    static const int MAX_EVENT_NUMBER = 10;

private:
    /// 标识本 event_loop 所属线程
    pid_t m_thread_id;
    /// epoll文件描述符
    int m_epoll_fd;
    /// 管理已经被注册至epoll事件表的文件描述符
    std::set<int> m_listen_fds;

    /// eventloop 任务队列
    std::queue<Fn> m_pending_tasks;
    Mutex m_mutex;

    WakeUpFdEvent* m_wakeup_fd_event = nullptr;
    Timer* m_timer = nullptr;

    bool m_is_looping = false;

private:
    EventLoop();
public:
    static EventLoop* GetCurrentEventLoop(); // 获取EvenLoop实例调用EventLoop构造函数,单例模式
    ~EventLoop();

    void loop();   // 启动EventLoop事件循环
    void wakeup(); // 向wakeup_fd中写入1字节字符，用于唤醒epoll_wait
    void stop();   //

    bool isInLoopThread() const { return getThreadId() == m_thread_id; }
    bool isLooping() const { return m_is_looping; }

    void addEpollEvent(FdEvent* event);
    void deleteEpollEvent(FdEvent* event);

    void addTimerEvent(const TimerEvent::spointer& event);

private:
    void initWakeUpFdEevent();
    void initTimer();

    void addTask(const Fn& cb, bool is_wake_up = false);
};

}

#endif //ROCKET_EVENTLOOP_H
