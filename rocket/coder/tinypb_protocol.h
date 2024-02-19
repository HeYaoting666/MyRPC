//
// Created by 20771 on 2024/1/25.
//

#ifndef ROCKET_TINYPB_PROTOCOL_H
#define ROCKET_TINYPB_PROTOCOL_H

#include "abstract_protocol.h"

namespace rocket {

struct TinyPBProtocol: public AbstractProtocol{
public:
    static char PB_START;     // 开始符 0x02 (1B)
    static char PB_END;       // 结束符 0x03 (1B)

public:
    uint32_t m_pk_len{};       // 整包长度 (4B)
    uint32_t m_msg_id_len{};   // msg_id 的长度(4B)，msg_id 继承父类

    uint32_t m_method_name_len{};  // 方法名长度 (4B)
    std::string m_method_name;     // 方法名

    uint32_t m_err_code{};       // 错误码 (4B)
    uint32_t m_err_info_len{};   // 错误信息长度 (4B)
    std::string m_err_info;      // 错误信息

    std::string m_pb_data;       // 序列化数据

    uint32_t m_check_sum{};      // 校验码 (4B)

    bool parse_success {false};


public:
    TinyPBProtocol() = default;
    ~TinyPBProtocol() override = default;

};

} // rocket

#endif //ROCKET_TINYPB_PROTOCOL_H
