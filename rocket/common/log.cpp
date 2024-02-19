//
// Created by 20771 on 2024/1/9.
//

#include "log.h"
#include "../net/eventloop.h"

namespace rocket {

static Logger* g_logger = nullptr;

Logger* Logger::GetGlobalLogger() {
    return g_logger;
}

void CoreDumpHandler(int signal_no) {
    ERRORLOG("%s", "progress received invalid signal, will exit")
    g_logger->flush();
    pthread_join(g_logger->getAsyncLogger()->m_thread, nullptr);
    pthread_join(g_logger->getAsyncAppLogger()->m_thread, nullptr);

    signal(signal_no, SIG_DFL);
    raise(signal_no);
}

std::string logLevelToString(LogLevel level) {
    switch (level) {
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        case ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

rocket::LogLevel stringToLogLevel(const std::string& str_level) {
    if (str_level == "DEBUG") {
        return rocket::DEBUG;
    }
    else if (str_level == "INFO") {
        return rocket::INFO;
    }
    else if (str_level == "ERROR") {
        return rocket::ERROR;
    }
    else {
        return rocket::UNKNOWN;
    }
}



/********************************************
 * LogEvent
 *******************************************/
std::string LogEvent::toString() {
    struct timeval now_time{};
    struct tm now_time_t{};
    gettimeofday(&now_time, nullptr);
    localtime_r(&now_time.tv_sec, &now_time_t); // 将结果存入至 now_time_t 中

    char buf[128];
    strftime(buf, 128, "%y-%m-%d %H:%M:%S", &now_time_t);
    std::string now_time_str(buf);
    size_t ms = now_time.tv_usec / 1000;
    now_time_str += "." + std::to_string(ms);

    m_pid = getPid();
    m_thread_id = getThreadId();

    std::stringstream ss;
    ss << "[" << logLevelToString(m_level) << "]\t"
       << "[" << now_time_str << "]\t"
       << "[" << m_pid << ":" << m_thread_id << "]\t";

    // 获取当前线程处理的请求的 msg_id
    std::string msgid = RunTime::GetRunTime()->m_msgid;
    std::string method_name = RunTime::GetRunTime()->m_method_name;
    if (!msgid.empty())
        ss << "[" << msgid << "]\t";

    if (!method_name.empty())
        ss << "[" << method_name << "]\t";

    return ss.str();
}


/********************************************
 * AsyncLogger
 *******************************************/
AsyncLogger::AsyncLogger(std::string file_name, std::string file_path, int max_file_size):
    m_thread(0),
    m_file_name(std::move(file_name)),
    m_file_path(std::move(file_path)),
    m_max_file_size(max_file_size),
    m_file_handler(nullptr),
    m_no(0),
    m_reopen_flag(false),
    m_stop_flag(false)
{
    assert(pthread_create(&m_thread, nullptr, Loop, this) == 0);
    m_sempahore.wait(); // wait, 等待线程创建完成,使主从线程同步
}

// 将 block_que 里面的全部数据打印到文件中，然后线程睡眠，直到有新的数据再重复这个过程
void* AsyncLogger::Loop(void* arg) {
    auto logger = reinterpret_cast<AsyncLogger*>(arg);
    logger->m_sempahore.post();

    while(true) {
        ScopeMutex<Mutex> locker(logger->m_mutex);
        //  队列中无元素则阻塞等待，否则取出队首元素
        while(logger->m_block_que.empty()) {
            logger->m_condtion.wait(logger->m_mutex.getMutex());
        }

        // 取出队首元素
        std::vector<std::string> log_buffer;
        log_buffer.swap(logger->m_block_que.front());
        logger->m_block_que.pop();
        locker.unlock();

        // 获取当前时间和日志文件名
        struct timeval now{};
        struct tm now_time{};
        const char* format = "%Y%m%d";
        char date[32];
        gettimeofday(&now, nullptr);
        localtime_r(&(now.tv_sec), &now_time);
        strftime(date, sizeof(date), format, &now_time);

        // 判断日期是否相同
        if(std::string(date) != logger->m_date) {
            logger->m_no = 0;
            logger->m_reopen_flag = true;
            logger->m_date = std::string(date);
        }
        if(logger->m_file_handler == nullptr)
            logger->m_reopen_flag = true;

        std::string log_file_name = logger->m_file_path + logger->m_file_name + "_"
                    + std::string(date) + "_log.";

        // 判断是否写入新文件
        if(logger->m_reopen_flag) {
            if(logger->m_file_handler)
                fclose(logger->m_file_handler);
            logger->m_file_handler = fopen(log_file_name.c_str(), "a");
            logger->m_reopen_flag = false;
        }

        // 若文件大小达到上限，则文件序号加1，写入新文件中
        if(ftell(logger->m_file_handler) >= logger->m_max_file_size) {
            fclose(logger->m_file_handler);

            log_file_name += std::to_string(++logger->m_no);
            logger->m_file_handler = fopen(log_file_name.c_str(), "a");
            logger->m_reopen_flag = false;
        }

        // 将日志内容写入目标文件
        for(const auto& log: log_buffer) {
            if(!log.empty())
                fwrite(log.c_str(), 1, log.length(), logger->m_file_handler);
        }
        fflush(logger->m_file_handler);

        if(logger->m_stop_flag)
            break;
    }
    return nullptr;
}

void AsyncLogger::pushLogBuffer(std::vector<std::string>& vec) {
    ScopeMutex<Mutex> locker(m_mutex);
    m_block_que.push(vec);
    m_condtion.broadcast(); // 唤醒异步日志线程
}


/********************************************
 * Logger
 *******************************************/
Logger::Logger(LogLevel set_level, int type): m_set_level(set_level), m_type(type) {
    if(m_type == 0) return;

    // 创建异步写线程
    m_async_logger = std::make_shared<AsyncLogger>(
            Config::GetGlobalConfig()->m_log_file_name + "_rpc",
            Config::GetGlobalConfig()->m_log_file_path,
            Config::GetGlobalConfig()->m_log_max_file_size);

    m_async_app_logger = std::make_shared<AsyncLogger>(
            Config::GetGlobalConfig()->m_log_file_name + "_app",
            Config::GetGlobalConfig()->m_log_file_path,
            Config::GetGlobalConfig()->m_log_max_file_size);
}

void Logger::InitGlobalLogger(int type) {
    LogLevel log_level = stringToLogLevel(Config::GetGlobalConfig()->m_log_level);
    g_logger = new Logger(log_level, type);
    g_logger->init();
}

void Logger::init() {
    if(m_type == 0) return;

    m_timer_event = std::make_shared<TimerEvent>(Config::GetGlobalConfig()->m_log_sync_interval,
                                                 true, std::bind(&Logger::syncLoop, this));
    EventLoop::GetCurrentEventLoop()->addTimerEvent(m_timer_event);

    signal(SIGSEGV, CoreDumpHandler);
    signal(SIGABRT, CoreDumpHandler);
    signal(SIGTERM, CoreDumpHandler);
    signal(SIGKILL, CoreDumpHandler);
    signal(SIGINT, CoreDumpHandler);
    signal(SIGSTKFLT, CoreDumpHandler);
}

void Logger::pushLog(const std::string &msg) {
    if(m_type == 0) {
        printf("%s", msg.c_str());
        return;
    }
    ScopeMutex<Mutex> locker(m_mutex);
    m_buffer.push_back(msg);
}

void Logger::pushAppLog(const std::string& msg) {
    ScopeMutex<Mutex> locker(m_app_mutex);
    m_app_buffer.push_back(msg);
}


void Logger::syncLoop() {
    // 同步 m_buffer 到 async_logger 中的 block_que
    std::vector<std::string> tmp1;
    ScopeMutex<Mutex> locker1(m_mutex);
    tmp1.swap(m_buffer);
    locker1.unlock();

    if(!tmp1.empty())
        m_async_logger->pushLogBuffer(tmp1);

    // 同步 m_buffer 到 async_app_logger 中的 block_que
    std::vector<std::string> tmp2;
    ScopeMutex<Mutex> locker2(m_app_mutex);
    tmp2.swap(m_app_buffer);
    locker2.unlock();

    if(!tmp2.empty())
        m_async_app_logger->pushLogBuffer(tmp2);
}

// 停止异步写，同时将数据刷新至磁盘
void Logger::flush() {
    syncLoop();
    m_async_logger->stop();
    m_async_logger->flush();
}


}

