//
// Created by 20771 on 2024/1/17.
//

#ifndef ROCKET_IO_THREAD_H
#define ROCKET_IO_THREAD_H
#include "eventloop.h"

namespace rocket {

/**
 * @brief IOThread IO事件管理线程，封装了EventLoop
 */
class IOThread {
public:
    typedef std::shared_ptr<IOThread> spointer;

private:
    pid_t m_id = -1;         // 线程id
    pthread_t m_thread = 0;  // 线程句柄
    EventLoop* m_event_loop = nullptr; // 当前 io 线程的 EventLoop 对象

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



/**
 * @brief IOThreadGroup IO线程组，管理IOThread,由主线程调用
 */
class IOThreadGroup {
private:
    int m_size;      // 线程组大小
    int m_index = 0; // 线程组索引下标，用于获取线程组当中的线程
    std::vector<IOThread::spointer> m_io_thread_groups; // 线程组，存储每个 IOThread 的指针

public:
    explicit IOThreadGroup(int size);

public:
    void start(); // 开启每个 IOThread 的 EventLoop事件
    void join();  // 回收线程组当中每个线程
    IOThread::spointer getIOThread(); // 获取线程组当中的线程句柄
};

}


#endif //ROCKET_IO_THREAD_H
