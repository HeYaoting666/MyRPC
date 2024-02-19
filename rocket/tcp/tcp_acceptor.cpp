//
// Created by 20771 on 2024/1/18.
//

#include "tcp_acceptor.h"

namespace rocket {

IPNetAddr::IPNetAddr(std::string ip, uint16_t port): m_ip(std::move(ip)), m_port(port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(m_port);
    m_addr.sin_addr.s_addr = inet_addr(m_ip.c_str());
}

IPNetAddr::IPNetAddr(const std::string& addr) {
    size_t i = addr.find_first_of(':');
    if(i == std::string::npos) {
        ERRORLOG("invalid ipv4 addr %s", addr.c_str())
        return;
    }

    m_ip = addr.substr(0, i);
    m_port = std::stoi(addr.substr(i + 1, addr.size() - i - 1));

    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(m_port);
    m_addr.sin_addr.s_addr = inet_addr(m_ip.c_str());
}

IPNetAddr::IPNetAddr(sockaddr_in addr): m_addr(addr) {
    m_ip = std::string(inet_ntoa(m_addr.sin_addr));
    m_port = ntohs(m_addr.sin_port);
}

bool IPNetAddr::CheckValid(const std::string& addr) {
    size_t i = addr.find_first_of(':');
    if(i == std::string::npos) {
        return false;
    }

    std::string ip = addr.substr(0, i);
    std::string port = addr.substr(i + 1, addr.size() - i - 1);
    if(ip.empty() || port.empty()) {
        return false;
    }

    int iport = std::stoi(port);
    if (iport <= 0 || iport > 65536) {
        return false;
    }

    return true;
}

bool IPNetAddr::isValid() {
    if (m_ip.empty()) {
        return false;
    }
    if (inet_addr(m_ip.c_str()) == INADDR_NONE) {
        return false;
    }
    if (m_port < 0) {
        return false;
    }
    return true;
}



TCPAcceptor::TCPAcceptor(const NetAddr::spointer& local_addr): m_local_addr(local_addr) {
    if(!m_local_addr->isValid()) {
        ERRORLOG("invalid local addr %s", local_addr->toString().c_str())
        exit(0);
    }

    // socket()
    m_listenfd = socket(m_local_addr->getFamily(), SOCK_STREAM, 0);
    if(m_listenfd < 0) {
        ERRORLOG("invalid listenfd[%d]", m_listenfd)
        exit(0);
    }

    // 设置listenfd属性，跳过 wait_time 快速重新使用ip：port
    int on = 1;
    if(setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
        ERRORLOG("setsockopt REUSEADDR error, errno=%d, error=%s", errno, strerror(errno))
        exit(0);
    }

    // bind()
    if(bind(m_listenfd, m_local_addr->getSockAddr(), m_local_addr->getSockLen()) != 0) {
        ERRORLOG("bind error, errno=%d, error=%s", errno, strerror(errno))
        exit(0);
    }

    // listen()
    if(listen(m_listenfd, 1000) != 0) {
        ERRORLOG("listen error, errno=%d, error=%s", errno, strerror(errno))
        exit(0);
    }
}

std::pair<int, NetAddr::spointer> TCPAcceptor::TCPAccept() {
    if(m_local_addr->getFamily() == AF_INET) {
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);

        int client_fd = accept(m_listenfd, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);
        if(client_fd < 0) {
            ERRORLOG("accept error, errno=%d, error=%s", errno, strerror(errno))
            return {-1, nullptr};
        }

        NetAddr::spointer sp_client_addr = std::make_shared<IPNetAddr>(client_addr);
        INFOLOG("A client have been accepted successfully, client_addr [%s]", sp_client_addr->toString().c_str())

        return {client_fd, sp_client_addr};
    }
    else {
        // ...
        return  {-1, nullptr};
    }
}

}