//
// Created by 20771 on 2024/1/17.
//

#ifndef ROCKET_IO_THREAD_H
#define ROCKET_IO_THREAD_H
#include "eventloop.h"

namespace rocket {

/******************************************
 *  IOThread 从线程IO事件管理，封装了EventLoop
 ******************************************/
class IOThread {
public:
    typedef std::shared_ptr<IOThread> spointer;

private:
    pid_t m_thread_id;  // 线程id
    pthread_t m_thread; // 线程句柄
    EventLoop* m_event_loop; // 当前 io 线程的 EventLoop 对象

    Sem m_init_semaphore;
    Sem m_start_semaphore;

public:
    IOThread();
    ~IOThread();

public:
    EventLoop* getEventloop() { return m_event_loop; }
    void start();
    void join() const;

public:
    static void* Main(void* arg);
};



/******************************************
 *  IOThreadGroup IO线程组，管理IOThread
 *  由主线程调用
 ******************************************/
class IOThreadGroup {
private:
    int m_size;
    int m_index;
    std::vector<IOThread::spointer> m_io_thread_groups;

public:
    explicit IOThreadGroup(int size);

public:
    void start(); // 开启每个 IOThread 的 EventLoop事件
    void join();
    IOThread::spointer getIOThread();
};

}


#endif //ROCKET_IO_THREAD_H
