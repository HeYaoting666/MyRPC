//
// Created by 20771 on 2024/1/9.
//
#include "util.h"

namespace rocket {

// 保存进程号和线程号，减少系统调用
static pid_t g_pid = 0;
static thread_local pid_t t_thread_id = 0;

pid_t getPid() {
    if (!g_pid)
        g_pid = getpid();
    return g_pid;
}

pid_t getThreadId() {
    if (!t_thread_id)
        t_thread_id = static_cast<pid_t>(syscall(SYS_gettid));
    return t_thread_id;
}

int64_t getNowMs() {
    timeval val{};
    gettimeofday(&val, nullptr);
    return val.tv_sec * 1000 + val.tv_usec / 1000;
}

uint32_t getInt32FromNetByte(const char* buf) {
    uint32_t re;
    memcpy(&re, buf, sizeof(re));
    return ntohl(re);
}


}