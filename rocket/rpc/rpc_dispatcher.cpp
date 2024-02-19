//
// Created by 20771 on 2024/1/29.
//

#include "rpc_dispatcher.h"

namespace rocket {

static RpcDispatcher* g_rpc_dispatcher = nullptr;

RpcDispatcher *RpcDispatcher::GetRpcDispatcher() {
    if (g_rpc_dispatcher != nullptr) return g_rpc_dispatcher;

    g_rpc_dispatcher = new RpcDispatcher();
    return g_rpc_dispatcher;
}

void RpcDispatcher::dispatch(const AbstractProtocol::spointer& request, const AbstractProtocol::spointer& response,
                             TCPConnection* connection) {
    auto req_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(request);
    auto rsp_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(response);
    rsp_protocol->m_msg_id = req_protocol->m_msg_id;
    rsp_protocol->m_method_name = req_protocol->m_method_name;

    // 获取请求消息方法名并解析出 service_name 和 method_name
    std::string method_full_name = req_protocol->m_method_name;
    std::string service_name;
    std::string method_name;
    if(!parseServiceFullName(method_full_name, service_name, method_name)) {
        setTinyPBError(rsp_protocol, ERROR_PARSE_SERVICE_NAME, "parse service name error");
        return;
    }

    // 查找对应服务 service对象 和 method对象
    // 取出 service 对象
    auto it = m_service_map.find(service_name);
    if(it == m_service_map.end()) {
        ERRORLOG("%s | service name[%s] not found", req_protocol->m_msg_id.c_str(), service_name.c_str())
        setTinyPBError(rsp_protocol, ERROR_SERVICE_NOT_FOUND, "service not found");
        return;
    }
    service_spointer service = it->second;

    // 根据 service对象 得到 method对象
    auto method = service->GetDescriptor()->FindMethodByName(method_name);
    if(method == nullptr) {
        ERRORLOG("%s | method name[%s] not found in service[%s]", req_protocol->m_msg_id.c_str(), method_name.c_str(), service_name.c_str())
        setTinyPBError(rsp_protocol, ERROR_SERVICE_NOT_FOUND, "method not found");
        return;
    }

    // 反序列化，将请求体 req_protocol 中的 pb_data 反序列化为 req_msg
    auto req_msg = service->GetRequestPrototype(method).New();
    if(!req_msg->ParseFromString(req_protocol->m_pb_data)) {
        ERRORLOG("%s | deserialize error", req_protocol->m_msg_id.c_str(), method_name.c_str(), service_name.c_str())
        setTinyPBError(rsp_protocol, ERROR_FAILED_DESERIALIZE, "deserialize error");

        delete req_msg;
        return;
    }
    INFOLOG("%s | get rpc request[%s]", req_protocol->m_msg_id.c_str(), req_msg->ShortDebugString().c_str())

    // 初始化 rpc_controller
    auto rpc_controller = new RpcController();
    rpc_controller->SetLocalAddr(connection->getLocalAddr());
    rpc_controller->SetPeerAddr(connection->getPeerAddr());
    rpc_controller->SetMsgId(req_protocol->m_msg_id);

    // 初始化 RunTime
    RunTime::GetRunTime()->m_msgid = req_protocol->m_msg_id;
    RunTime::GetRunTime()->m_method_name = method_name;

    // 初始化 rpc_closure
    auto rsp_msg = service->GetResponsePrototype(method).New();
    auto rpc_closure = new RpcClosure([req_msg, rsp_msg, req_protocol, rsp_protocol, connection] {
        if(!rsp_msg->SerializeToString(&(rsp_protocol->m_pb_data))) {
            ERRORLOG("%s | serialize error, origin message [%s]", req_protocol->m_msg_id.c_str(), rsp_msg->ShortDebugString().c_str())
            setTinyPBError(rsp_protocol, ERROR_FAILED_SERIALIZE, "serialize error");
        }
        else {
            rsp_protocol->m_err_code = 0;
            rsp_protocol->m_err_info = "";
            INFOLOG("%s | dispatch success, request[%s], response[%s]", req_protocol->m_msg_id.c_str(), req_msg->ShortDebugString().c_str(), rsp_msg->ShortDebugString().c_str())
        }
        std::vector<AbstractProtocol::spointer> replay_messages;
        replay_messages.emplace_back(rsp_protocol);
        connection->reply(replay_messages);
    });

    // 调用 RPC 方法
    service->CallMethod(method, rpc_controller, req_msg, rsp_msg, rpc_closure);

    delete rpc_controller;
//    delete rpc_closure;
    delete req_msg;
    delete rsp_msg;
}

void RpcDispatcher::registerService(const service_spointer& service) {
    std::string service_name = service->GetDescriptor()->full_name();
    m_service_map[service_name] = service;
}

bool RpcDispatcher::parseServiceFullName(const std::string& full_name, std::string& service_name,
                                         std::string& method_name) {
    if(full_name.empty()) {
        ERRORLOG("%s", "full name empty")
        return false;
    }

    size_t i = full_name.find_first_of('.');
    if(i == std::string::npos) {
        ERRORLOG("not find . in full name [%s]", full_name.c_str())
        return false;
    }
    service_name = full_name.substr(0, i);
    method_name = full_name.substr(i + 1, full_name.length() - i - 1);

    INFOLOG("parse service_name[%s] and method_name[%s] from full name [%s]", service_name.c_str(), method_name.c_str(), full_name.c_str())

    return true;
}

void RpcDispatcher::setTinyPBError(const std::shared_ptr<TinyPBProtocol>& msg, int32_t err_code, const std::string& err_info) {
    msg->m_err_code = err_code;
    msg->m_err_info = err_info;
    msg->m_err_info_len = err_info.length();
}

}