//
// Created by 20771 on 2024/1/29.
//

#include "../rocket/tcp/tcp_server.h"
#include "order.pb.h"

class OrderImpl: public Order {
public:
    void makeOrder(google::protobuf::RpcController* controller,
                   const ::makeOrderRequest* request,
                   ::makeOrderResponse* response,
                   ::google::protobuf::Closure* done) override {
        if (request->price() < 10) {
            response->set_ret_code(-1);
            response->set_res_info("short balance");
            return;
        }
        response->set_order_id("20230514");
        APPDEBUGLOG("%s", "call makeOrder success")
        if (done) {
            done->Run();
            delete done;
        }
    }
};

int main() {
    rocket::Config::SetGlobalConfig("./conf/rocket_server.xml");
    rocket::Logger::InitGlobalLogger(0);

    auto service = std::make_shared<OrderImpl>();
    rocket::RpcDispatcher::GetRpcDispatcher()->registerService(service);

    auto addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", rocket::Config::GetGlobalConfig()->m_port);
    rocket::TCPServer server(addr);
    server.start();

    return 0;
}