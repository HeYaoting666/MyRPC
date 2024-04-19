//
// Created by 20771 on 2024/1/19.
//

#include "tcp_server.h"

namespace rocket {

rocket::TCPServer::TCPServer(const NetAddr::spointer& local_addr){
    // 初始化 TCPAcceptor
    m_acceptor = std::make_shared<TCPAcceptor>(local_addr);

    // 初始化 主线程 EventLoop
    m_eventloop = EventLoop::GetCurrentEventLoop();

    // 初始化 从线程 IOThreadGroup
    m_io_thread_group = new IOThreadGroup(Config::GetGlobalConfig()->m_io_threads_nums);

    // 初始化 监听套接字事件 m_listen_fd_event 不使用ET模式
    m_listen_fd_event = new FdEvent(m_acceptor->getListenFd());
    m_listen_fd_event->setEvent(FdEvent::IN_EVENT, std::bind(&TCPServer::onAccept, this), false);
    m_eventloop->addEpollEvent(m_listen_fd_event);

    // 初始化 定时器事件 m_clear_client_timer_event
    m_clear_client_timer_event = std::make_shared<TimerEvent>(
            5000, true, std::bind(&TCPServer::onClearClientTimerFunc, this));
    m_eventloop->addTimerEvent(m_clear_client_timer_event);

    INFOLOG("rocket TCPServer listen success on [%s]", m_acceptor->getLocalAddr()->toString().c_str())
}

TCPServer::~TCPServer() {
    if (m_eventloop) {
        m_eventloop->stop();
        delete m_eventloop;
        m_eventloop = nullptr;
    }
    if (m_io_thread_group) {
        m_io_thread_group->join();
        delete m_io_thread_group;
        m_io_thread_group = nullptr;
    }
    if (m_listen_fd_event) {
        delete m_listen_fd_event;
        m_listen_fd_event = nullptr;
    }
}

void TCPServer::start() {
    m_io_thread_group->start();
    m_eventloop->loop();
}

void TCPServer::onAccept() {
    auto accept_ret = m_acceptor->TCPAccept(); // <客户端套接字， 客户端地址>
    auto io_thread_handler = m_io_thread_group->getIOThread(); // 从线程组中获取线程句柄

    auto connection = std::make_shared<TCPConnection>(
            io_thread_handler->getEventloop(),
            accept_ret.first,
            128,
            accept_ret.second,
            m_acceptor->getLocalAddr());
    connection->setState(Connected);

    m_clients.insert(connection);
    INFOLOG("TCPServer success get client, fd=%d", accept_ret.first)
}

void TCPServer::onClearClientTimerFunc() {
    auto it = m_clients.begin();
    for (it = m_clients.begin(); it != m_clients.end(); ) {
        if ((*it) != nullptr && (*it).use_count() > 0 && (*it)->getState() == Closed) {
            // need to delete TCPConnection
            DEBUGLOG("TCPConnection [fd:%d] will delete, state=%d", (*it)->getFd(), (*it)->getState());
            it = m_clients.erase(it);
        } else {
            ++it;
        }
    }
}


}
