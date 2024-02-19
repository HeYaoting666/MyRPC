//
// Created by 20771 on 2024/1/24.
//

#ifndef ROCKET_ABSTRACT_CODER_H
#define ROCKET_ABSTRACT_CODER_H

#include <vector>
#include "../tcp/tcp_buffer.h"
#include "abstract_protocol.h"

namespace rocket {

class AbstractCoder {
public:

    // 将 message 对象转化为字节流，写入到 buffer
    virtual void encode(std::vector<AbstractProtocol::spointer> &in_messages, TCPBuffer::spointer buffer) = 0;

    // 将 buffer 里面的字节流转换为 message 对象
    virtual void decode(std::vector<AbstractProtocol::spointer> &out_messages, TCPBuffer::spointer buffer) = 0;

    virtual ~AbstractCoder() = default;

};

}

#endif //ROCKET_ABSTRACT_CODER_H
