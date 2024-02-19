//
// Created by 20771 on 2024/1/29.
//

#ifndef ROCKET_RPC_DISPATCHER_H
#define ROCKET_RPC_DISPATCHER_H

#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include "rpc_controller.h"
#include "rpc_closure.h"
#include "../common/run_time.h"
#include "../common/error_code.h"
#include "../coder/tinypb_protocol.h"
#include "../tcp/tcp_connection.h"

namespace rocket {

class TCPConnection; // 互相包含头文件，声明 TCPConnection 类

class RpcDispatcher {
public:
    typedef std::shared_ptr<google::protobuf::Service> service_spointer;
    static RpcDispatcher* GetRpcDispatcher();

private:
    std::map<std::string, service_spointer> m_service_map; // <服务名, 服务对象>

public:
    void dispatch(const AbstractProtocol::spointer& request, const AbstractProtocol::spointer& response, TCPConnection* connection);

    // 注册prc服务对象
    void registerService(const service_spointer& service);

private:
    // 解析服务名称 "OrderServer.make_order" service_name = "OrderServer", method_name = "make_order"
    static bool parseServiceFullName(const std::string& full_name, std::string& service_name, std::string& method_name);

    // 设置错误信息
    static void setTinyPBError(const std::shared_ptr<TinyPBProtocol>& msg, int32_t err_code, const std::string& err_info);
};

}

#endif //ROCKET_RPC_DISPATCHER_H
