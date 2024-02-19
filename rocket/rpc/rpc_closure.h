//
// Created by 20771 on 2024/1/30.
//

#ifndef ROCKET_RPC_CLOSURE_H
#define ROCKET_RPC_CLOSURE_H

#include <functional>
#include <google/protobuf/stubs/callback.h>
#include "rpc_interface.h"

namespace rocket {

class RpcClosure : public google::protobuf::Closure {
private:
    std::function<void()> m_cb;

public:
    explicit RpcClosure(std::function<void()> cb): m_cb(std::move(cb))
    {
        INFOLOG("%s", "RpcClosure")
    }

    ~RpcClosure() override {
        INFOLOG("%s", "~RpcClosure")
    }

public:
    void Run() override {
        if(m_cb != nullptr)
            m_cb();
    }
};

}

#endif //ROCKET_RPC_CLOSURE_H
