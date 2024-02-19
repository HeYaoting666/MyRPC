//
// Created by 20771 on 2024/1/18.
//

#ifndef ROCKET_TCP_ACCEPTOR_H
#define ROCKET_TCP_ACCEPTOR_H

#include <arpa/inet.h>
#include "../common/log.h"


namespace rocket {

/***********************************************************
 *  NetAddr： 地址基类
 *  IPNetAddr： IP地址管理类，管理ip地址，端口port和相关协议族family
 ***********************************************************/
class NetAddr {
public:
    typedef std::shared_ptr<NetAddr> spointer;

public:
    virtual ~NetAddr() = default;

public:
    virtual sockaddr* getSockAddr() = 0;

    virtual socklen_t getSockLen() = 0;

    virtual int getFamily() = 0;

    virtual std::string toString() = 0;

    virtual bool isValid() = 0;
};

class IPNetAddr: public NetAddr {
private:
    std::string m_ip;
    uint16_t m_port;
    sockaddr_in m_addr{};

public:
    IPNetAddr(std::string ip, uint16_t port);    // 以 "ip" + "port" 的形式初始化

    explicit IPNetAddr(const std::string& addr); // 以 "ip:port" 的形式初始化

    explicit IPNetAddr(sockaddr_in addr);        // 以 "sockaddr_in" 的形式初始化

    ~IPNetAddr() override = default;

public:
    static bool CheckValid(const std::string& addr);

public:
    sockaddr* getSockAddr() override { return reinterpret_cast<sockaddr*>(&m_addr); }

    socklen_t getSockLen() override { return sizeof(m_addr); }

    int getFamily() override { return m_addr.sin_family; }

    std::string toString() override { return m_ip + ":" + std::to_string(m_port); }

    bool isValid() override;
};


/***********************************************************
 *  TCPAcceptor： tcp连接监听套接字管理
 *  负责初始化tcp连接监听套接字，处理相关tcp连接accept
 ***********************************************************/
class TCPAcceptor {
public:
    typedef std::shared_ptr<TCPAcceptor> spointer;

private:
    NetAddr::spointer m_local_addr; // 服务端监听的地址，addr -> ip:port
    int m_listenfd;                 // 监听套接字

public:
    explicit TCPAcceptor(const NetAddr::spointer& local_addr); // socket(), bind(), listen()

public:
    std::pair<int, NetAddr::spointer> TCPAccept(); // 处理tcp连接，返回<连接套接字, 客户端地址> TCPAccept()

    int getListenFd() const { return m_listenfd; }
    NetAddr::spointer getLocalAddr() const { return m_local_addr; }
};

}

#endif //ROCKET_TCP_ACCEPTOR_H
