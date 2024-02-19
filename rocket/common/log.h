//
// Created by 20771 on 2024/1/9.
//

#ifndef ROCKET_LOG_H
#define ROCKET_LOG_H

#include <string>
#include <vector>
#include <queue>
#include <sstream>
#include <memory>
#include <cstring>
#include <cassert>
#include <csignal>
#include <sys/time.h>
#include "run_time.h"
#include "util.h"
#include "lock.h"
#include "config.h"
#include "../net/timer_event.h"


#define DEBUGLOG(str, ...) \
    if (rocket::Logger::GetGlobalLogger()->getLogLevel() && rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::DEBUG) \
    { \
    rocket::Logger::GetGlobalLogger()->pushLog(rocket::LogEvent(rocket::DEBUG).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
    } \

#define INFOLOG(str, ...) \
    if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::INFO) \
    { \
    rocket::Logger::GetGlobalLogger()->pushLog(rocket::LogEvent(rocket::LogLevel::INFO).toString() \
    + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
    } \

#define ERRORLOG(str, ...) \
    if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::ERROR) \
    { \
    rocket::Logger::GetGlobalLogger()->pushLog(rocket::LogEvent(rocket::LogLevel::ERROR).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
    } \


#define APPDEBUGLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::DEBUG) \
  { \
    rocket::Logger::GetGlobalLogger()->pushAppLog(rocket::LogEvent(rocket::LogLevel::DEBUG).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
  } \

namespace rocket {

class TimerEvent;

// 日志级别
enum LogLevel {
    UNKNOWN = 0,
    DEBUG = 1,
    INFO = 2,
    ERROR = 3
};

std::string logLevelToString(LogLevel level);

LogLevel stringToLogLevel(const std::string& str_level);

template<typename... Args>
std::string formatString(const char* str, Args&&... args) {
    size_t size = snprintf(nullptr, 0, str, args...);
    std::string result;
    if(size > 0) {
        result.resize(size);
        snprintf(&result[0], size + 1, str, args...);
    }
    return result;
}



/************************************
 *           日志事件LogEvent         *
 ************************************/
class LogEvent {
private:
    pid_t m_pid;          // 进程号
    pid_t m_thread_id;    // 线程号
    LogLevel m_level;        // 日志级别

public:
    explicit LogEvent(LogLevel level): m_level(level), m_pid(0), m_thread_id(0) { }

    std::string toString();
};



/************************************
 *         日志异步写AsyncLogger      *
 ************************************/
class AsyncLogger {
public:
    typedef std::shared_ptr<AsyncLogger> spointer;

public:
    pthread_t m_thread;

private:
    std::string m_file_name;    // 日志输出文件名字
    std::string m_file_path;    // 日志输出路径
    int m_max_file_size;        // 日志单个文件最大大小, 单位为字节

    Mutex m_mutex;
    Sem m_sempahore;
    Cond m_condtion;
    std::queue<std::vector<std::string>> m_block_que; // 阻塞队列

    std::string m_date;    // 当前打印日志的文件日期
    FILE* m_file_handler;  // 当前打开的日志文件句柄
    int m_no;              // 日志文件序号
    bool m_reopen_flag;
    bool m_stop_flag;

public:
    AsyncLogger(std::string file_name, std::string file_path, int max_file_size);
    ~AsyncLogger() {
        if(m_file_handler)
            fclose(m_file_handler);
    }

public:
    void flush() { if(m_file_handler) fflush(m_file_handler); }

    void stop() { m_stop_flag = true; }

    void pushLogBuffer(std::vector<std::string>& vec);

private:
    static void* Loop(void*); // 异步日志线程函数
};


/************************************
 *          日志触发器Logger          *
 ************************************/
class Logger {
private:
    LogLevel m_set_level;   // 日志等级
    int m_type;             // 0为同步输出到控制台，1为异步输出至文件

    std::vector<std::string> m_buffer;
    std::vector<std::string> m_app_buffer;
    Mutex m_mutex;
    Mutex m_app_mutex;

    AsyncLogger::spointer m_async_logger;     // 异步日志线程
    AsyncLogger::spointer m_async_app_logger; // 异步日志线程，应用层

    TimerEvent::spointer m_timer_event;       // 异步写入定时

public:
    LogLevel getLogLevel() const { return m_set_level; }

    AsyncLogger::spointer getAsyncLogger() { return m_async_logger; }
    AsyncLogger::spointer getAsyncAppLogger() { return m_async_app_logger; }

    void pushLog(const std::string& msg);     // 将日志消息写入 m_buffer 缓冲区中，等待同步
    void pushAppLog(const std::string& msg);  // 将日志消息写入 m_app_buffer 缓冲区中，等待同步

    void syncLoop();       // 同步 m_buffer 到 async_logger 中的 block_que

    void flush();          // 停止异步写，同时将数据刷新至磁盘

public:
    static void InitGlobalLogger(int type = 1);
    static Logger* GetGlobalLogger();

private:
    explicit Logger(LogLevel set_level, int type = 1);
    ~Logger() = default;

    void init();
};

}

#endif //ROCKET_LOG_H
