//
// Created by 20771 on 2024/1/31.
//

#include "rpc_channel.h"

namespace rocket {

RpcChannel::RpcChannel(NetAddr::spointer  peer_addr):
    m_peer_addr(std::move(peer_addr)),
    m_controller(nullptr),
    m_request(nullptr),
    m_response(nullptr),
    m_closure(nullptr),
    m_is_init(false),
    m_client(nullptr) {
    INFOLOG("%s", "RpcChannel")
}

RpcChannel::~RpcChannel() {
    INFOLOG("%s", "~RpcChannel")
}

void RpcChannel::callBack() {
    auto my_controller = dynamic_cast<RpcController*>(m_controller.get());
    if (m_closure) {
        m_closure->Run();
        my_controller->SetFinished(true);
    }
}

void RpcChannel::Init(const RpcChannel::controller_spointer& controller, const RpcChannel::message_spointer& req,
                      const RpcChannel::message_spointer& res, const RpcChannel::closure_spointer& done) {
    if(m_is_init)
        return;

    m_controller = controller;
    m_request = req;
    m_response = res;
    m_closure = done;
    m_is_init = true;
}


void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) {

    // 创建请求协议 req_protocol
    std::shared_ptr<rocket::TinyPBProtocol> req_protocol = std::make_shared<rocket::TinyPBProtocol>();

    auto my_controller = dynamic_cast<RpcController*>(controller);
    if(my_controller == nullptr) {
        ERRORLOG("%s", "failed callmethod, RpcController convert error")
        callBack();
        return;
    }

    // 判断初始化是否成功
    if(!m_is_init) {
        std::string err_info = "RpcChannel not call init()";
        my_controller->SetError(ERROR_RPC_CHANNEL_INIT, err_info);
        ERRORLOG("%s | %s, RpcChannel not init ", req_protocol->m_msg_id.c_str(), err_info.c_str())
        callBack();
        return;
    }

    // 获取 msg_id
    if(my_controller->GetMsgId().empty()) {
        // 先从 runtime 里面取, 取不到再生成一个
        // 这样的目的是为了实现 msg_id 的透传，假设服务 A 调用了 B，那么同一个 msgid 可以在服务 A 和 B 之间串起来，方便日志追踪
        std::string msg_id = RunTime::GetRunTime()->m_msgid;
        if(!msg_id.empty()) {
            req_protocol->m_msg_id = msg_id;
            my_controller->SetMsgId(msg_id);
        }
        else {
            req_protocol->m_msg_id = MsgIDUtil::GetMsgID();
            my_controller->SetMsgId(req_protocol->m_msg_id);
        }
    }
    else {
        // 如果 controller 指定了 msg_id, 直接使用
        req_protocol->m_msg_id = my_controller->GetMsgId();
    }

    // 获取 method_name
    req_protocol->m_method_name = method->full_name();
    INFOLOG("%s | call method name [%s]", req_protocol->m_msg_id.c_str(), req_protocol->m_method_name.c_str())

    // request 序列化至 m_pb_data
    if(!request->SerializePartialToString(&req_protocol->m_pb_data)) {
        std::string err_info = "failed to serialize";
        my_controller->SetError(ERROR_FAILED_SERIALIZE, err_info);
        ERRORLOG("%s | %s, origin request [%s] ", req_protocol->m_msg_id.c_str(), err_info.c_str(), request->ShortDebugString().c_str())
        callBack();
        return;
    }

    // 设置 rpc 超时事件
    TimerEvent::spointer time_event = std::make_shared<TimerEvent>(my_controller->GetTimeout(), false, [my_controller, this]() mutable {
        INFOLOG("%s | call rpc timeout arrive", my_controller->GetMsgId().c_str())
        if (my_controller->Finished()) {
            INFOLOG("%s | rpc is finished", my_controller->GetMsgId().c_str())
            return;
        }

        my_controller->StartCancel();
        my_controller->SetError(ERROR_RPC_CALL_TIMEOUT, "rpc call timeout " + std::to_string(my_controller->GetTimeout()));
        callBack();
    });

    // 启动 rpc 事件
    m_client = std::make_shared<TCPClient>(m_peer_addr);
    m_client->addTimerEvent(time_event);
    m_client->TCPConnect([req_protocol, my_controller, this]() mutable {
        if(m_client->getConnectErrorCode() != 0) { // tcp 连接失败
            my_controller->SetError(m_client->getConnectErrorCode(), m_client->getConnectErrorInfo());
            ERRORLOG("%s | connect error, error code[%d], error info[%s], peer addr[%s]",
                     req_protocol->m_msg_id.c_str(),
                     my_controller->GetErrorCode(),
                     my_controller->GetErrorInfo().c_str(),
                     m_client->getPeerAddr()->toString().c_str())
            callBack();
            return;
        }

        // tcp 连接成功
        INFOLOG("%s | connect success, peer addr[%s], local addr[%s]",
                req_protocol->m_msg_id.c_str(),
                m_client->getPeerAddr()->toString().c_str(),
                m_client->getLocalAddr()->toString().c_str())

        // 1. 把 message 对象写入到 Connection 的 buffer, done 也写入
        // 2. 启动 connection 可写事件
        m_client->writeMessage(req_protocol,[req_protocol, this](const AbstractProtocol::spointer& msg) mutable {
            INFOLOG("%s | send rpc request success. call method name[%s], peer addr[%s], local addr[%s]",
                    req_protocol->m_msg_id.c_str(),
                    req_protocol->m_method_name.c_str(),
                    m_client->getPeerAddr()->toString().c_str(),
                    m_client->getLocalAddr()->toString().c_str())

            DEBUGLOG("send message success, request[%s]", m_request->ShortDebugString().c_str())
        });

        // 1. 监听可读事件
        // 2. 从 buffer 里 decode 得到 message 对象, 判断是否 msg_id 相等，相等则读成功，执行其回调
        m_client->readMessage(req_protocol->m_msg_id, [my_controller, this](const AbstractProtocol::spointer& msg) mutable {
            auto rsp_protocol = std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg);
            if (rsp_protocol->m_err_code != 0) {
                ERRORLOG("%s | call rpc method[%s] failed, error code[%d], error info[%s]",
                         rsp_protocol->m_msg_id.c_str(),
                         rsp_protocol->m_method_name.c_str(),
                         rsp_protocol->m_err_code,
                         rsp_protocol->m_err_info.c_str())

                my_controller->SetError((int)rsp_protocol->m_err_code, rsp_protocol->m_err_info);
                callBack();
                return;
            }

            if (!(m_response->ParseFromString(rsp_protocol->m_pb_data))){
                ERRORLOG("%s | serialize error", rsp_protocol->m_msg_id.c_str())
                my_controller->SetError(ERROR_FAILED_SERIALIZE, "serialize error");
                callBack();
                return;
            }

            INFOLOG("%s | call rpc success, call method name[%s], peer addr[%s], local addr[%s]",
                    rsp_protocol->m_msg_id.c_str(),
                    rsp_protocol->m_method_name.c_str(),
                    m_client->getPeerAddr()->toString().c_str(),
                    m_client->getLocalAddr()->toString().c_str())

            DEBUGLOG("get response success, response[%s]", m_response->ShortDebugString().c_str())
            callBack();
        });
    });
}


}