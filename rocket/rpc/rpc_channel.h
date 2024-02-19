//
// Created by 20771 on 2024/1/31.
//

#ifndef ROCKET_RPC_CHANNEL_H
#define ROCKET_RPC_CHANNEL_H

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include "rpc_controller.h"
#include "../coder/tinypb_protocol.h"
#include "../tcp/tcp_acceptor.h"
#include "../tcp/tcp_client.h"
#include "../common/msg_id.h"

namespace rocket {

class RpcChannel : public google::protobuf::RpcChannel,
                   public std::enable_shared_from_this<RpcChannel> {
public:
    typedef std::shared_ptr<google::protobuf::RpcController> controller_spointer;
    typedef std::shared_ptr<google::protobuf::Message> message_spointer;
    typedef std::shared_ptr<google::protobuf::Closure> closure_spointer;

private:
    NetAddr::spointer m_peer_addr;

    controller_spointer m_controller;
    message_spointer m_request;
    message_spointer m_response;
    closure_spointer m_closure;

    bool m_is_init;

    TCPClient::spointer m_client;

public:
    explicit RpcChannel(NetAddr::spointer  peer_addr);
    ~RpcChannel() override;

public:
    void Init(const controller_spointer& controller, const message_spointer& req,
              const message_spointer& res, const closure_spointer& done);

    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done) override;

    TCPClient* getTcpClient() { return m_client.get(); }

private:
    void callBack();
};

}


#endif //ROCKET_RPC_CHANNEL_H
