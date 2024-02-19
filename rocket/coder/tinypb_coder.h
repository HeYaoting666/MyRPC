//
// Created by 20771 on 2024/1/25.
//

#ifndef ROCKET_TINYPB_CODER_H
#define ROCKET_TINYPB_CODER_H

#include "abstract_coder.h"
#include "tinypb_protocol.h"

namespace rocket {

class TinyPBCoder: public AbstractCoder{
public:
    TinyPBCoder() = default;

public:
    // 将 message 对象转化为字节流，写入到 in_buffer，编码
    void encode(std::vector<AbstractProtocol::spointer> &in_messages,
                TCPBuffer::spointer in_buffer) override;

    // 将 buffer 里面的字节流转换为 message 对象，解码
    void decode(std::vector<AbstractProtocol::spointer> &out_messages,
                TCPBuffer::spointer buffer) override;
};

} // rocket

#endif //ROCKET_TINYPB_CODER_H
