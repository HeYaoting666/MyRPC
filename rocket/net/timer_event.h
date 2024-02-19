//
// Created by 20771 on 2024/2/18.
//

#ifndef ROCKET_TIMER_EVENT_H
#define ROCKET_TIMER_EVENT_H

#include <memory>
#include <functional>
#include "../common/util.h"

namespace rocket {
    
/************************************
 *  TimerEvent 定时事件
 ************************************/
class TimerEvent {
public:
    typedef std::shared_ptr<TimerEvent> spointer;
    typedef std::function<void()> Fn;

private:
    int64_t m_arrive_time; // 定时器执行时间 ms
    int64_t m_interval;    // 定时器执行时间间隔 ms
    bool m_is_repeated;    // 是否重复
    bool m_is_canceled;    // 是否被删除
    Fn m_task;             // 定时器回调函数

public:
    TimerEvent(int interval, bool is_repeated, Fn task);

public:
    int64_t getArriveTime() const { return m_arrive_time; }

    void setCancel(bool val) { m_is_canceled = val; }

    void resetArriveTime() { m_arrive_time = getNowMs() + m_interval; }

    bool isCancel() const { return m_is_canceled; }

    bool isRepeated() const { return m_is_repeated; }

    Fn getCallBack() const { return m_task; }
};

}

#endif //ROCKET_TIMER_EVENT_H
