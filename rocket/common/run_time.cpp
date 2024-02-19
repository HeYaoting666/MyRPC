//
// Created by 20771 on 2024/2/18.
//

#include "run_time.h"

namespace rocket {

thread_local RunTime* t_run_time = nullptr;

RunTime* RunTime::GetRunTime() {
    if (t_run_time) return t_run_time;

    t_run_time = new RunTime();
    return t_run_time;
}

} // rocket