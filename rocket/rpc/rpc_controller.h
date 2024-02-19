//
// Created by 20771 on 2024/1/30.
//

#ifndef ROCKET_RPC_CONTROLLER_H
#define ROCKET_RPC_CONTROLLER_H

#include <string>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>

#include "../tcp/tcp_acceptor.h"

namespace rocket {

class RpcController: public google::protobuf::RpcController {
private:
    int32_t m_error_code;
    std::string m_error_info;
    std::string m_msg_id;

    bool m_is_failed;
    bool m_is_canceled;
    bool m_is_finished;

    NetAddr::spointer m_local_addr;
    NetAddr::spointer m_peer_addr;

    int m_timeout;   // ms

public:
    RpcController(): m_error_code(0),
        m_is_failed(false),
        m_is_finished(false),
        m_is_canceled(false),
        m_timeout(1000)
    {
        INFOLOG("%s", "RpcController")
    }

    ~RpcController() override { INFOLOG("%s", "~RpcController") }

public:
    void Reset() override;
    bool Failed() const override;
    std::string ErrorText() const override;
    void StartCancel() override;
    void SetFailed(const std::string& reason) override;
    void NotifyOnCancel(google::protobuf::Closure* callback) override;



    void SetError(int32_t error_code, const std::string& error_info);
    void SetMsgId(const std::string& msg_id);
    void SetFinished(bool value);
    void SetLocalAddr(const NetAddr::spointer& addr);
    void SetPeerAddr(const NetAddr::spointer& addr);
    void SetTimeout(int timeout);



    int32_t GetErrorCode() const;
    std::string GetErrorInfo() const;
    std::string GetMsgId();
    bool Finished() const;
    bool IsCanceled() const override;
    NetAddr::spointer GetLocalAddr();
    NetAddr::spointer GetPeerAddr();
    int GetTimeout() const;
};

}
#endif //ROCKET_RPC_CONTROLLER_H
