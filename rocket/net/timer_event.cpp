//
// Created by 20771 on 2024/2/18.
//

#include "timer_event.h"
#include "../common/log.h"


namespace rocket {

TimerEvent::TimerEvent(int interval, bool is_repeated, Fn task):
        m_interval(interval), m_is_repeated(is_repeated), m_task(std::move(task)), m_is_canceled(false)
{
    m_arrive_time = getNowMs() + m_interval;
    DEBUGLOG("success create timer event, will execute at [%lld]", m_arrive_time)
}

}