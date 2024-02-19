//
// Created by 20771 on 2024/1/9.
//

#ifndef ROCKET_UTIL_H
#define ROCKET_UTIL_H

#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/syscall.h>

namespace rocket {

pid_t getPid();

pid_t getThreadId();

int64_t getNowMs();

uint32_t getInt32FromNetByte(const char* buf);

}

#endif //ROCKET_UTIL_H
