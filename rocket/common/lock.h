//
// Created by 20771 on 2024/1/10.
//

#ifndef ROCKET_MUTEX_H
#define ROCKET_MUTEX_H

#include <pthread.h>
#include <semaphore.h>

namespace rocket {

// 互斥锁
class Mutex {
private:
    pthread_mutex_t m_mutex{};

public:
    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }

    pthread_mutex_t* getMutex() {
        return &m_mutex;
    }
};

template <typename T>
class ScopeMutex {
private:
    T& m_mutex;
    bool m_is_lock;

public:
    explicit ScopeMutex(T& mutex): m_mutex(mutex), m_is_lock(false) {
        m_mutex.lock();
        m_is_lock = true;
    }

    ~ScopeMutex() {
        if(m_is_lock) m_mutex.unlock();
    }

    void lock() {
        if(!m_is_lock) {
            m_mutex.lock();
            m_is_lock = true;
        }
    }

    void unlock() {
        if(m_is_lock) {
            m_mutex.unlock();
            m_is_lock = false;
        }
    }
};

// 信号量
class Sem {
private:
    sem_t m_sem{};
public:
    Sem() {
        if(sem_init(&m_sem, 0, 0) != 0) {
            throw std::exception();
        }
    }
    explicit Sem(int num) {
        if(sem_init(&m_sem, 0, num) != 0) {
            throw std::exception();
        }
    }
    ~Sem() {
        sem_destroy(&m_sem);
    }

    bool wait() { return sem_wait(&m_sem) == 0; }
    bool post() { return sem_post(&m_sem) == 0; }
};

// 条件变量
class Cond {
private:
    pthread_cond_t m_cond{};
public:
    Cond() {
        if(pthread_cond_init(&m_cond, nullptr) != 0) {
            throw std::exception();
        }
    }
    ~Cond() {
        pthread_cond_destroy(&m_cond);
    }

    bool wait(pthread_mutex_t *m_mutex){
        int ret = pthread_cond_wait(&m_cond, m_mutex);
        return ret == 0;
    }
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t){
        int ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        return ret == 0;
    }
    bool signal() { return pthread_cond_signal(&m_cond) == 0; }
    bool broadcast() { return pthread_cond_broadcast(&m_cond) == 0; }
};

}

#endif //ROCKET_MUTEX_H
