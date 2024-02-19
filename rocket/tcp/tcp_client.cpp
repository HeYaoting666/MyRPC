//
// Created by 20771 on 2024/1/23.
//

#include "tcp_client.h"


namespace rocket {

TCPClient::TCPClient(rocket::NetAddr::spointer  peer_addr): m_peer_addr(std::move(peer_addr)) {
    m_client_event_loop = EventLoop::GetCurrentEventLoop();

    m_client_fd = socket(m_peer_addr->getFamily(), SOCK_STREAM, 0);
    if(m_client_fd < 0) {
        ERRORLOG("%s", "TcpClient::TcpClient() error, failed to create fd")
        return;
    }
    m_client_event = FdEventGroup::GetFdEventGroup()->getFdEvent(m_client_fd);
    m_client_event->setNonBlock();

    m_connection = std::make_shared<TCPConnection>(m_client_event_loop,
                                                   m_client_fd,
                                                   128,
                                                   m_peer_addr,
                                                   nullptr,
                                                   TCPConnectionByClient);
    m_connect_error_code = 0;
}

TCPClient::~TCPClient() {
    DEBUGLOG("%s", "TcpClient::~TcpClient()")

    if(m_client_event_loop->isLooping())
        m_client_event_loop->stop();

    if (m_client_fd > 0)
        close(m_client_fd);
}

void TCPClient::TCPConnect(const std::function<void()>& done) {
    int ret = connect(m_client_fd, m_peer_addr->getSockAddr(), m_peer_addr->getSockLen());
    if(ret == 0) {
        DEBUGLOG("connect [%s] success", m_peer_addr->toString().c_str())
        m_connection->setState(Connected);
        initLocalAddr(); // 初始化本地地址
        if(done) done(); // 执行 done
    }
    else if(ret == -1) {
        if(errno == EINPROGRESS) {
            // epoll 监听可写事件，然后判断错误码，非阻塞连接
            m_client_event->setEvent(FdEvent::OUT_EVENT, [done, this](){
                int ret = connect(m_client_fd, m_peer_addr->getSockAddr(), m_peer_addr->getSockLen());
                if((ret < 0 && errno == EISCONN) || (ret == 0)) {
                    DEBUGLOG("connect [%s] success", m_peer_addr->toString().c_str())
                    m_connection->setState(Connected);
                    initLocalAddr();
                }
                else {
                    if (errno == ECONNREFUSED) { // 连接时对端关闭
                        m_connect_error_code = ERROR_PEER_CLOSED;
                        m_connect_error_info = "connect refused, sys error = " + std::string(strerror(errno));
                    }
                    else {
                        m_connect_error_code = ERROR_FAILED_CONNECT;
                        m_connect_error_info = "connect unknown error, sys error = " + std::string(strerror(errno));
                    }
                    ERRORLOG("connect error, errno=%d, error=%s", errno, strerror(errno))
                    close(m_client_fd);
                    m_client_fd = socket(m_peer_addr->getFamily(), SOCK_STREAM, 0);
                }
                // 连接完后需要去掉可写事件的监听，不然会一直触发
                m_client_event->cancelEvent(FdEvent::OUT_EVENT);
                m_client_event_loop->addEpollEvent(m_client_event);
                if(done) done(); // 执行 done
            });

            m_client_event_loop->addEpollEvent(m_client_event);
        }
        else {
            ERRORLOG("connect error, errno=%d, error=%s", errno, strerror(errno));
            m_connect_error_code = ERROR_FAILED_CONNECT;
            m_connect_error_info = "connect error, sys error = " + std::string(strerror(errno));
            close(m_client_fd);
            m_client_fd = socket(m_peer_addr->getFamily(), SOCK_STREAM, 0);

            if(done) done(); // 执行 done
        }
    }

    if(!m_client_event_loop->isLooping())
        m_client_event_loop->loop();
}

void TCPClient::writeMessage(const AbstractProtocol::spointer& message,
                             const std::function<void(AbstractProtocol::spointer)>& done) {
    // 1. 把 message 对象写入到 Connection 的 buffer, done 也写入
    // 2. 启动 connection 可写事件
    m_connection->pushSendMessage(message, done);
    m_connection->listenWrite();
}

void TCPClient::readMessage(const std::string &msg_id,
                            const std::function<void(AbstractProtocol::spointer)>& done) {
    // 1. 监听可读事件
    // 2. 从 buffer 里 decode 得到 message 对象, 判断是否 msg_id 相等，相等则读成功，执行其回调
    m_connection->pushReadMessage(msg_id, done);
    m_connection->listenRead();
}

void TCPClient::initLocalAddr() {
    sockaddr_in local_addr{};
    socklen_t len = sizeof(local_addr);

    int ret = getsockname(m_client_fd, reinterpret_cast<sockaddr*>(&local_addr), &len);
    if(ret != 0) {
        ERRORLOG("initLocalAddr error, getsockname error. errno=%d, error=%s", errno, strerror(errno));
        return;
    }
    m_local_addr = std::make_shared<IPNetAddr>(local_addr);
}

}
