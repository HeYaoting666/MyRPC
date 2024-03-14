//
// Created by 20771 on 2024/1/30.
//

#include "order.pb.h"
#include "../rocket/tcp/tcp_client.h"
#include "../rocket/rpc/rpc_channel.h"

int main() {
    rocket::Config::SetGlobalConfig("./conf/rocket_client.xml");
    rocket::Logger::InitGlobalLogger(0);

    // 定义rpc channel
    rocket::IPNetAddr::spointer addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12310);
    auto channel = std::make_shared<rocket::RpcChannel>(addr);

    // 定义消息体 message
    auto request = std::make_shared<makeOrderRequest>();
    auto response = std::make_shared<makeOrderResponse>();
    request->set_price(100);
    request->set_goods("apple");

    // 定义rpc controller
    auto controller = std::make_shared<rocket::RpcController>();
    controller->SetMsgId("99998888"); // 设置 msg_id
    controller->SetTimeout(5000);            // 设置超时时间为 5s

    // 定义rpc closure
    std::shared_ptr<rocket::RpcClosure> closure = std::make_shared<rocket::RpcClosure>([&channel, request, response, controller]() {
        if (controller->GetErrorCode() == 0) {
            INFOLOG("call rpc success, request[%s], response[%s]", request->ShortDebugString().c_str(), response->ShortDebugString().c_str())
            // 执行业务逻辑
            if (response->order_id() == "xxx") {
                // xx
            }
        }
        else {
            ERRORLOG("call rpc failed, request[%s], error code[%d], error info[%s]",
                     request->ShortDebugString().c_str(),
                     controller->GetErrorCode(),
                     controller->GetErrorInfo().c_str())
        }
        INFOLOG("%s", "now exit eventloop")
        channel->getTcpClient()->stop();
    });

    // 调用 rpc 服务
    channel->Init(controller, request, response, closure);
    // 调用rpc channel CallMethod方法
    Order_Stub(channel.get()).makeOrder(controller.get(), request.get(), response.get(), closure.get());

    return 0;
}