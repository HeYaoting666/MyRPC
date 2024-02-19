//
// Created by 20771 on 2024/2/18.
//

#ifndef ROCKET_TIMER_H
#define ROCKET_TIMER_H

#include <map>
#include "fd_event.h"
#include "../common/lock.h"

namespace rocket {

/************************************
 *  Timer 用于管理TimerEvent
 ************************************/
class Timer: public FdEvent {
private:
    std::multimap<int64_t, TimerEvent::spointer> m_pending_events; // 根据事务的 arrive_time 进行排序
    Mutex m_mutex;

public:
    Timer();

public:
    void onTimer(); // 定时器回调函数，并删除超时的定时事件

    void addTimerEvent(const TimerEvent::spointer& event);

private:
    void resetTimerFd();

};

} // rocket

#endif //ROCKET_TIMER_H
