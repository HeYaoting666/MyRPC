//
// Created by 20771 on 2024/1/17.
//

#include "io_thread.h"

namespace rocket {

IOThread::IOThread(): m_thread_id(-1), m_thread(0), m_event_loop(nullptr) {
    pthread_create(&m_thread, nullptr, Main, this);

    // wait, 直到新线程执行完 Main 函数的前置
    m_init_semaphore.wait();
    DEBUGLOG("IOThread [%d] create success", m_thread_id)
}

IOThread::~IOThread() {
    if(m_event_loop) {
        m_event_loop->stop();
        delete m_event_loop;
        m_event_loop = nullptr;
    }
}

void *IOThread::Main(void *arg) {
    auto io_thread = static_cast<IOThread*>(arg);

    io_thread->m_thread_id = getThreadId();
    io_thread->m_event_loop = new EventLoop();

    // 唤醒等待的线程
    io_thread->m_init_semaphore.post();

    // 让IO 线程等待，直到我们主动的启动
    DEBUGLOG("IOThread [%d] wait start semaphore", io_thread->m_thread_id)
    io_thread->m_start_semaphore.wait();

    DEBUGLOG("IOThread [%d] start loop ", io_thread->m_thread_id)
    io_thread->m_event_loop->loop();
    DEBUGLOG("IOThread [%d] end loop ", io_thread->m_thread_id)

    return nullptr;
}

void IOThread::start() {
    DEBUGLOG("Now invoke IOThread [%d]", m_thread_id)
    m_start_semaphore.post();
}

void IOThread::join() const {
    pthread_join(m_thread, nullptr);
}



IOThreadGroup::IOThreadGroup(int size): m_size(size), m_index(0) {
    m_io_thread_groups.resize(size);
    for(int i = 0; i < size; ++i) {
        m_io_thread_groups[i] = std::make_shared<IOThread>();
    }
}

void IOThreadGroup::start() {
    for(const auto& io_thread: m_io_thread_groups) {
        io_thread->start();
    }
}

void IOThreadGroup::join() {
    for(const auto& io_thread: m_io_thread_groups) {
        io_thread->join();
    }
}

IOThread::spointer IOThreadGroup::getIOThread() {
    m_index = (m_index + 1) % m_size;
    return m_io_thread_groups[m_index];
}

}
