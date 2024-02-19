//
// Created by 20771 on 2024/1/23.
//

#ifndef ROCKET_TCP_CLIENT_H
#define ROCKET_TCP_CLIENT_H


#include "tcp_acceptor.h"
#include "tcp_connection.h"
#include "../net/eventloop.h"
#include "../common/error_code.h"
#include "../coder/abstract_protocol.h"

namespace rocket {

class TCPClient {
public:
    typedef std::shared_ptr<TCPClient> spointer;

private:
    NetAddr::spointer m_local_addr; // 本地地址
    NetAddr::spointer m_peer_addr;  // 对端地址

    EventLoop* m_client_event_loop;
    FdEvent* m_client_event;
    int m_client_fd;
    TCPConnection::spointer m_connection;

    int m_connect_error_code;
    std::string m_connect_error_info;

public:
    explicit TCPClient(NetAddr::spointer  peer_addr);

    ~TCPClient();

public:
    // 异步进行 connect，即非阻塞connect
    // 如果 connect 完成，done 会被执行
    void TCPConnect(const std::function<void()>& done);

    // 异步的发送 message
    // 如果发送 message 成功，会调用 done 函数， 函数的入参就是 message 对象
    void writeMessage(const AbstractProtocol::spointer& message,
                      const std::function<void(AbstractProtocol::spointer)>& done);

    // 异步的读取 message
    // 如果读取 message 成功，会调用 done 函数， 函数的入参就是 message 对象
    void readMessage(const std::string& msg_id,
                     const std::function<void(AbstractProtocol::spointer)>& done);

    void addTimerEvent(const TimerEvent::spointer& timer_event) {
        m_client_event_loop->addTimerEvent(timer_event);
    }

    void stop() {
        if (m_client_event_loop->isLooping()) {
            m_client_event_loop->stop();
        }
    }

    int getConnectErrorCode() const { return m_connect_error_code; }

    std::string getConnectErrorInfo() const { return m_connect_error_info; }

    NetAddr::spointer getPeerAddr() const { return m_peer_addr; }

    NetAddr::spointer getLocalAddr() const { return m_local_addr; }

private:
    void initLocalAddr();

};

}


#endif //ROCKET_TCP_CLIENT_H
