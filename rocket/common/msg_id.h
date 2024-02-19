//
// Created by 20771 on 2024/1/31.
//

#ifndef ROCKET_MSG_ID_H
#define ROCKET_MSG_ID_H

#include <string>
#include <fcntl.h>
#include <unistd.h>
#include "log.h"


namespace rocket {

class MsgIDUtil {
public:
    static std::string GetMsgID();
};

}

#endif //ROCKET_MSG_ID_H
