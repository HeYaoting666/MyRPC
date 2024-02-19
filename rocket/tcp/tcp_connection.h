//
// Created by 20771 on 2024/1/22.
//

#ifndef ROCKET_TCP_CONNECTION_H
#define ROCKET_TCP_CONNECTION_H

#include "tcp_acceptor.h"
#include "tcp_buffer.h"
#include "../net/eventloop.h"
#include "../coder/tinypb_protocol.h"
#include "../coder/tinypb_coder.h"
#include "../rpc/rpc_dispatcher.h"

namespace rocket {

enum TCPConnectionType {
    TCPConnectionByServer = 1,  // 作为服务端使用，代表跟对端客户端的连接
    TCPConnectionByClient = 2,  // 作为客户端使用，代表跟对端服务端的连接
};

enum TCPState {
    NotConnected = 1,
    Connected = 2,
    HalfClosing = 3,
    Closed = 4,
};

/******************************************************
 *    TCPConnection 从线程事件循环
 *    Read: 读取客户发送的数据，组装为RPC请求
 *    Execute: 将 RPC 请求作为入参，执行业务逻辑得到 RPC 响应
 *    Write: 将RPC响应发送给客户端
 ******************************************************/
class TCPConnection {
public:
    typedef std::shared_ptr<TCPConnection> spointer;

private:
    EventLoop* m_event_loop;      // 持有该连接的IO线程的 event_loop 事件

    NetAddr::spointer m_local_addr;   // 本地地址
    NetAddr::spointer m_peer_addr;    // 对端地址

    TCPBuffer::spointer m_in_buffer;  // 接收缓冲区
    TCPBuffer::spointer m_out_buffer; // 发送缓冲区

    FdEvent* m_conn_event;
    int m_conn_fd;

    TCPConnectionType m_conn_type;  // 连接类型
    TCPState m_conn_state;          // 连接状态

    std::vector<
        std::pair<AbstractProtocol::spointer,
        std::function<void(AbstractProtocol::spointer)>>
        > m_write_dones;      // vec[pair<message, func()>]

    std::map<
        std::string,
        std::function<void(AbstractProtocol::spointer)>
        > m_read_dones;       // map<msg_id, func()>

    AbstractCoder* m_coder;   // 编码解码器

public:
    TCPConnection(EventLoop* event_loop, int fd, int buffer_size, NetAddr::spointer peer_addr,
                  NetAddr::spointer local_addr, TCPConnectionType type = TCPConnectionByServer);

public:
    void pushReadMessage(const std::string& msg_id,
                         const std::function<void(AbstractProtocol::spointer)>& done);

    void pushSendMessage(const AbstractProtocol::spointer& message,
                         const std::function<void(AbstractProtocol::spointer)>& done);

    void reply(std::vector<AbstractProtocol::spointer>& replay_messages); // 用于 rpc_dispatcher 写回调用

    void listenRead(bool is_et = true);  // 启动监听可读事件，默认为ET模式

    void listenWrite(bool is_et = true); // 启动监听可写事件，默认为ET模式

    void setState(TCPState state) { m_conn_state = state; }

    TCPState getState() const { return m_conn_state; }

    void setConnectionType(TCPConnectionType type) { m_conn_type = type; }

    TCPConnectionType getConnectionType() const { return m_conn_type; }

    int getFd() const { return m_conn_fd; }

    NetAddr::spointer getLocalAddr() const { return m_local_addr; }

    NetAddr::spointer getPeerAddr() const {return m_peer_addr; }

private:
    void clear();       // 处理一些关闭连接后的清理动作

    void onRead();      // 可读事件回调函数，读取客户发送的数据，组装为RPC请求

    void execute();     // 将 RPC 请求作为入参，执行业务逻辑得到 RPC 响应

    void onWrite();     // 可写事件回调函数，将RPC响应 out_buffer 发送给客户端
};

}


#endif //ROCKET_TCP_CONNECTION_H
