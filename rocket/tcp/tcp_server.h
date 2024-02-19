//
// Created by 20771 on 2024/1/19.
//

#ifndef ROCKET_TCP_SERVER_H
#define ROCKET_TCP_SERVER_H

#include "tcp_acceptor.h"
#include "tcp_connection.h"
#include "../net/eventloop.h"
#include "../net/io_thread.h"

namespace rocket {

/******************************************************
 *    TCPServer 主线程事件循环
 *    负责监听连接套接字，并处理客户连接
 ******************************************************/
class TCPServer {
private:
    EventLoop* m_main_event_loop;     // 主线程事件循环 负责监听连接套接字
    FdEvent* m_listen_fd_event;       // 监听套接字事件

    IOThreadGroup* m_io_thread_group; // 从线程组 负责io和逻辑事件处理
    std::set<TCPConnection::spointer> m_clients;

    TCPAcceptor::spointer m_acceptor; // 包含 local_addr 和 listenfd

    TimerEvent::spointer m_clear_client_timer_event;

public:
    explicit TCPServer(const NetAddr::spointer& local_addr);
    ~TCPServer();

public:
    void start();    // 启动从线程事件循环和主线程事件循环

private:
    void onAccept(); // 连接套接字回调函数，负责处理客户连接

    void onClearClientTimerFunc(); // 清除 closed 的连接
};

}


#endif //ROCKET_TCP_SERVER_H
