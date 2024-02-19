//
// Created by 20771 on 2024/2/18.
//

#include "timer.h"

namespace rocket {

Timer::Timer(): FdEvent() {
    m_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    DEBUGLOG("timer fd[%d]", m_fd)

    // 把 fd 可读事件放到了 eventloop 上监听
    setEvent(FdEvent::IN_EVENT, std::bind(&Timer::onTimer, this));
}

void Timer::onTimer() {
    // 处理缓冲区数据
    char buf[128];
    while (true) {
        if((read(m_fd, buf, 128) == -1) && errno == EAGAIN)
            break;
    }

    // 执行定时任务
    int64_t now_time = getNowMs();
    std::vector<TimerEvent::spointer> tmps; // 存储超时的TimerEvent

    ScopeMutex<Mutex> locker(m_mutex);
    auto it = m_pending_events.begin();
    for(; it != m_pending_events.end(); ++it) {
        if(it->first <= now_time) {
            if(it->second->isCancel()) continue;
            tmps.emplace_back(it->second);
        }
        else {
            break;
        }
    }
    m_pending_events.erase(m_pending_events.begin(), it);
    locker.unlock();

    // 执行定时任务, 并把需要重复执行的 TimerEvent 再次添加至队列中
    for(const auto& time_event : tmps) {
        // 执行定时任务
        auto time_cb = time_event->getCallBack();
        if(time_cb) time_cb();

        // 把需要重复执行的 TimerEvent 再次添加至队列中
        if(time_event->isRepeated()) {
            time_event->resetArriveTime();   // 调整 arrive_time
            addTimerEvent(time_event); // 添加回队列中，重新设置timer_fd
        }
    }
}

void Timer::addTimerEvent(const TimerEvent::spointer& event) {
    bool flag_reset = false;

    ScopeMutex<Mutex> locker(m_mutex);
    if(m_pending_events.empty()) {
        flag_reset = true;
    }
    else {
        auto it = m_pending_events.begin();
        if((*it).second->getArriveTime() > event->getArriveTime()) // 若插入任务的指定时间比所有任务的指定时间都要早，则需要重新修改定时任务指定时间
            flag_reset = true;
    }
    m_pending_events.emplace(event->getArriveTime(), event);
    locker.unlock();

    if(flag_reset)
        resetTimerFd();
}

void Timer::resetTimerFd() {
    // 取出定时器中最早时间
    ScopeMutex<Mutex> locker(m_mutex);
    if(m_pending_events.empty()) {
        locker.unlock();
        return;
    }
    auto it = m_pending_events.begin();
    int64_t arrive_time = it->second->getArriveTime();
    locker.unlock();

    int64_t interval = 0;
    int64_t now_time = getNowMs();
    if(arrive_time > now_time)
        interval = arrive_time - now_time;
    else
        interval = 100;

    // 给 m_client_fd 设置指定时间
    timespec ts{};
    memset(&ts, 0, sizeof(ts));
    itimerspec value{};
    memset(&value, 0, sizeof(value));


    ts.tv_sec = interval / 1000;
    ts.tv_nsec = (interval % 1000) * 1000000;
    value.it_value = ts;

    int ret = timerfd_settime(m_fd, 0, &value, nullptr); // 指定时间一道便会触发可读事件
    if(ret != 0)
        ERRORLOG("timerfd_settime error, errno=%d, error=%s", errno, strerror(errno))
    DEBUGLOG("timer reset to %lld", now_time + interval)
}

} // rocket