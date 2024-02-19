//
// Created by 20771 on 2024/1/22.
//

#include "tcp_connection.h"

namespace rocket {

TCPConnection::TCPConnection(EventLoop* event_loop, int fd, int buffer_size, NetAddr::spointer peer_addr,
                             NetAddr::spointer local_addr, TCPConnectionType type):
        m_event_loop(event_loop),
        m_conn_fd(fd),
        m_peer_addr(std::move(peer_addr)),
        m_local_addr(std::move(local_addr)),
        m_conn_type(type),
        m_conn_state(NotConnected)
{
    m_in_buffer = std::make_shared<TCPBuffer>(buffer_size);
    m_out_buffer = std::make_shared<TCPBuffer>(buffer_size);

    m_conn_event = FdEventGroup::GetFdEventGroup()->getFdEvent(m_conn_fd);
    m_conn_event->setNonBlock();
    if(m_conn_type == TCPConnectionByServer)
        listenRead(); // 注册可读事件

    m_coder = new TinyPBCoder();
}

void TCPConnection::onRead() {
    if (m_conn_state != Connected) {
        ERRORLOG("onRead error, client has already disconnected, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_conn_fd)
        return;
    }

    // 调用系统的 read 函数从 socket 缓冲区读取字节至 in_buffer
    bool is_close = false;
    while(true) {
        if(m_in_buffer->writeAble() == 0) {
            m_in_buffer->resizeBuffer( static_cast<int>(2 * m_in_buffer->m_buffer.size()));
        }

        int read_count = m_in_buffer->writeAble();
        int write_index = m_in_buffer->writeIndex();
        int ret = static_cast<int>( read(m_conn_fd, &(m_in_buffer->m_buffer[write_index]), read_count) );
        DEBUGLOG("success read %d bytes from addr[%s], client fd[%d]", ret, m_peer_addr->toString().c_str(), m_conn_fd)
        if(ret == 0) {  // 客户端断开连接
            is_close = true;
            break;
        }
        if(ret == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) // 全部读取完成
                break;
            else {
                ERRORLOG("onRead Error, client fd[%d]", m_peer_addr->toString().c_str(), m_conn_fd)
                return;
            }
        }
        m_in_buffer->moveWriteIndex(ret);
    }

    if(is_close) {
        INFOLOG("peer closed, peer addr [%s], clientfd [%d]", m_peer_addr->toString().c_str(), m_conn_fd)
        clear();
        return;
    }

    execute(); // 进行 RPC 协议解析
}

void TCPConnection::execute() {
    // 从 buffer 里 decode 得到 message 对象
    std::vector<AbstractProtocol::spointer> messages;
    m_coder->decode(messages, m_in_buffer);

    // 若连接类型为服务端
    // 将 RPC 请求执行业务逻辑，获取 RPC 响应, 再把 RPC 响应发送回去
    if(m_conn_type == TCPConnectionByServer) {
        for(auto& req_msg: messages) {
            INFOLOG("success get request[%s] from client[%s]", req_msg->m_msg_id.c_str(), m_peer_addr->toString().c_str())

            // 1. 针对每一个请求，调用 rpc 方法，获取 响应message
            // 2. 将响应体 rsp_msg 放入到发送缓冲区，监听可写事件回包
            std::shared_ptr<TinyPBProtocol> rsp_msg = std::make_shared<TinyPBProtocol>();
            RpcDispatcher::GetRpcDispatcher()->dispatch(req_msg, rsp_msg, this);
        }
    }
    // 若连接类型为客户端
    // 根据 msg_id 进行校验，执行回调函数
    else {
        for(auto& rsp_msg: messages) {
            auto it = m_read_dones.find(rsp_msg->m_msg_id); // 根据 msg_id 进行校验
            if(it != m_read_dones.end()) { // 校验成功则执行其回调
                it->second(rsp_msg);
                m_read_dones.erase(it);
            }
        }
    }
}

void TCPConnection::onWrite() {
    if (m_conn_state != Connected) {
        ERRORLOG("onWrite error, client has already disconnected, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_conn_fd)
        return;
    }

    // 若连接类型为客户端则需要对 message 编码
    if(m_conn_type == TCPConnectionByClient) {
        std::vector<AbstractProtocol::spointer> messages;
        messages.reserve(m_write_dones.size());

        for(auto& pair: m_write_dones) {
            messages.emplace_back(pair.first);
        }
        // 对 message 进行编码后写入至 out_buffer 中
        m_coder->encode(messages, m_out_buffer);
    }

    bool is_write_all = false;
    while(true) {
        if (m_out_buffer->readAble() == 0) {
            DEBUGLOG("%s", "no data need to send")
            is_write_all = true;
            break;
        }

        int write_size = m_out_buffer->readAble();
        int read_index = m_out_buffer->readIndex();
        int ret = static_cast<int>( write(m_conn_fd, &(m_out_buffer->m_buffer[read_index]), write_size) );
        if(ret == -1 && errno == EAGAIN) {
            // 发送缓冲区已满，不能再发送了。
            // 这种情况我们等下次 fd 可写的时候再次发送数据即可
            ERRORLOG("%s", "write data error, errno==EAGAIN and rt == -1")
            break;
        }
        DEBUGLOG("success write %d bytes to addr[%s], client fd[%d]", ret, m_peer_addr->toString().c_str(), m_conn_fd)
        m_out_buffer->moveReadIndex(ret);
    }

    // 若数据全部发送完毕则关闭可写监听事件
    if(is_write_all) {
        m_conn_event->cancelEvent(FdEvent::OUT_EVENT);
        m_event_loop->addEpollEvent(m_conn_event);
    }

    // 若连接类型为客户端当发送 message 成功后，会调用 done 函数， 函数的入参就是 message 对象
    if (m_conn_type == TCPConnectionByClient) {
        for (auto& m_write_done : m_write_dones) {
            m_write_done.second(m_write_done.first);
        }
        m_write_dones.clear();
    }
}

void TCPConnection::listenRead(bool is_et) {
    m_conn_event->setEvent(FdEvent::IN_EVENT, std::bind(&TCPConnection::onRead, this), is_et);
    m_event_loop->addEpollEvent(m_conn_event);
}

void TCPConnection::listenWrite(bool is_et) {
    m_conn_event->setEvent(FdEvent::OUT_EVENT, std::bind(&TCPConnection::onWrite, this), is_et);
    m_event_loop->addEpollEvent(m_conn_event);
}

void TCPConnection::pushSendMessage(const AbstractProtocol::spointer& message,
                                    const std::function<void(AbstractProtocol::spointer)>& done) {
    m_write_dones.emplace_back(message, done);
}

void TCPConnection::pushReadMessage(const std::string& msg_id,
                                    const std::function<void(AbstractProtocol::spointer)>& done) {
    m_read_dones.insert({msg_id, done});
}

void TCPConnection::reply(std::vector<AbstractProtocol::spointer>& replay_messages) {
    m_coder->encode(replay_messages, m_out_buffer);
    listenWrite();
}

void TCPConnection::clear() {
    if(m_conn_state == Closed)
        return;

    m_conn_event->cancelEvent(FdEvent::IN_EVENT);
    m_conn_event->cancelEvent(FdEvent::OUT_EVENT);
    m_event_loop->deleteEpollEvent(m_conn_event);

    m_conn_state = Closed;
}

}
