//
// Created by 20771 on 2024/1/23.
//

#ifndef ROCKET_ABSTRACT_PROTOCOL_H
#define ROCKET_ABSTRACT_PROTOCOL_H

#include <memory>

namespace rocket {

struct AbstractProtocol : public std::enable_shared_from_this<AbstractProtocol> {
public:
    typedef std::shared_ptr<AbstractProtocol> spointer;

    virtual ~AbstractProtocol() = default;

public:
    std::string m_msg_id;     // 请求号，唯一标识一个请求或者响应
};

}
#endif //ROCKET_ABSTRACT_PROTOCOL_H
