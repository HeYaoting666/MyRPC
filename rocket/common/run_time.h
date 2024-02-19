//
// Created by 20771 on 2024/2/18.
//

#ifndef ROCKET_RUN_TIME_H
#define ROCKET_RUN_TIME_H

#include <string>

namespace rocket {

class RunTime {
public:
    std::string m_msgid;
    std::string m_method_name;

public:
    static RunTime* GetRunTime();
};

}

#endif //ROCKET_RUN_TIME_H
