//
// Created by 20771 on 2024/1/25.
//

#include "tinypb_coder.h"

namespace rocket {

void TinyPBCoder::encode(std::vector<AbstractProtocol::spointer> &in_messages,
                         TCPBuffer::spointer in_buffer) {
    for(auto& msg: in_messages) {
        auto message = std::dynamic_pointer_cast<TinyPBProtocol>(msg);

        if(message->m_msg_id.empty())
            message->m_msg_id = "123456789";
        DEBUGLOG("msg_id = %s", message->m_msg_id.c_str())

        uint32_t pk_len = 2 + 24 + message->m_msg_id.length() + message->m_method_name.length() +
                message->m_err_info.length() + message->m_pb_data.length();
        DEBUGLOG("pk_len = %d", pk_len)

        char* buf = reinterpret_cast<char*>(malloc(pk_len));
        char* tmp = buf;

        // 加入开始符标志
        *tmp = TinyPBProtocol::PB_START;
        ++tmp;

        // 加入 pk_len
        uint32_t pk_len_net = htonl(pk_len);
        memcpy(tmp, &pk_len_net, sizeof(pk_len_net));
        tmp += sizeof(pk_len_net);

        // 加入 msg_id_len
        uint32_t msg_id_len = message->m_msg_id.length();
        uint32_t msg_id_len_net = htonl(msg_id_len);
        memcpy(tmp, &msg_id_len_net, sizeof(msg_id_len_net));
        tmp += sizeof(msg_id_len_net);

        // 加入 msg_id
        if(!message->m_msg_id.empty()) {
            memcpy(tmp, &(message->m_msg_id[0]), msg_id_len);
            tmp += msg_id_len;
        }

        // 加入 method_name_id
        uint32_t method_name_len = message->m_method_name.length();
        uint32_t method_name_len_net = htonl(method_name_len);
        memcpy(tmp, &method_name_len_net, sizeof(method_name_len_net));
        tmp += sizeof(method_name_len_net);

        // 加入 method_name
        if(!message->m_method_name.empty()) {
            memcpy(tmp, &(message->m_method_name[0]), method_name_len);
            tmp += method_name_len;
        }

        // 加入 err_code
        uint32_t err_code_net = htonl(message->m_err_code);
        memcpy(tmp, &err_code_net, sizeof(err_code_net));
        tmp += sizeof(err_code_net);

        // 加入 err_info_len
        uint32_t err_info_len = message->m_err_info.length();
        uint32_t err_info_len_net = htonl(err_info_len);
        memcpy(tmp, &err_info_len_net, sizeof(err_info_len_net));
        tmp += sizeof(err_info_len_net);

        // 加入 err_info
        if (!message->m_err_info.empty()) {
            memcpy(tmp, &(message->m_err_info[0]), err_info_len);
            tmp += err_info_len;
        }

        // 加入 pb_data
        if (!message->m_pb_data.empty()) {
            memcpy(tmp, &(message->m_pb_data[0]), message->m_pb_data.length());
            tmp += message->m_pb_data.length();
        }

        // 加入 check_sum
        uint32_t check_sum_net = htonl(1);
        memcpy(tmp, &check_sum_net, sizeof(check_sum_net));
        tmp += sizeof(check_sum_net);

        // 加入结束符标志
        *tmp = TinyPBProtocol::PB_END;

        message->m_pk_len = pk_len;
        message->m_msg_id_len = msg_id_len;
        message->m_method_name_len = method_name_len;
        message->m_err_info_len = err_info_len;
        message->parse_success = true;

        DEBUGLOG("encode message[%s] success", message->m_msg_id.c_str());

        // 将编码后的字节流写入至 in_buffer 中
        if(buf != nullptr && pk_len != 0)
            in_buffer->writeToBuffer(buf, static_cast<int>(pk_len));

        // 释放分配的内存
        if(buf != nullptr)
            free(buf);
    }
}

void TinyPBCoder::decode(std::vector<AbstractProtocol::spointer> &out_messages,
                         TCPBuffer::spointer buffer) {
    while (true) {
        std::vector<char> tmp_buffer = buffer->m_buffer;
        uint32_t start_index = buffer->readIndex();
        uint32_t end_index = -1;
        uint32_t pk_len = 0;
        bool parse_success = false;

        // 遍历 buffer，找到 PB_START，找到之后解析出整包的长度，然后得到结束符的位置，判断是否为 PB_END
        uint32_t i;
        for(i = start_index; i < buffer->writeIndex(); ++i) {
            if(tmp_buffer[i] == TinyPBProtocol::PB_START) {
                if(i + 1 < buffer->writeIndex()) {
                    // 读下去四个字节获取整包长度，由于是网络字节序，需要转为主机字节序
                    pk_len = getInt32FromNetByte(&tmp_buffer[i + 1]);
                    DEBUGLOG("get pk_len = %d", pk_len)

                    // 结束符的索引
                    uint32_t j = i + pk_len - 1;
                    if(tmp_buffer[j] == TinyPBProtocol::PB_END) {
                        start_index = i;
                        end_index = j;
                        parse_success = true;
                        break;
                    }
                }
            }
        }
        if(i >= buffer->writeIndex()) {
            DEBUGLOG("%s", "decode end, read all buffer data")
            break;
        }

        // 长度解析成功
        if(parse_success) {
            std::shared_ptr<TinyPBProtocol> message = std::make_shared<TinyPBProtocol>();
            message->m_pk_len = pk_len;

            // 解析 msg_id_len
            uint32_t msg_id_len_index = start_index + sizeof(TinyPBProtocol::PB_START) + sizeof(message->m_pk_len);
            if (msg_id_len_index >= end_index) {
                message->parse_success = false;
                ERRORLOG("parse error, msg_id_len_index[%d] >= end_index[%d]", msg_id_len_index, end_index)
                continue;
            }
            message->m_msg_id_len = getInt32FromNetByte(&tmp_buffer[msg_id_len_index]);
            DEBUGLOG("parse msg_id_len=%d", message->m_msg_id_len)

            // 解析 msg_id
            uint32_t msg_id_index = msg_id_len_index + sizeof(message->m_msg_id_len);
            message->m_msg_id = std::string(&tmp_buffer[msg_id_index], message->m_msg_id_len);
            DEBUGLOG("parse msg_id=%s", message->m_msg_id.c_str())

            // 解析 method_name_len
            uint32_t method_name_len_index = msg_id_index + message->m_msg_id_len;
            if (method_name_len_index >= end_index) {
                message->parse_success = false;
                ERRORLOG("parse error, method_name_len_index[%d] >= end_index[%d]", method_name_len_index, end_index)
                continue;
            }
            message->m_method_name_len = getInt32FromNetByte(&tmp_buffer[method_name_len_index]);
            DEBUGLOG("parse method_name_len=%d", message->m_method_name_len)

            // 解析 method_name
            uint32_t method_name_index = method_name_len_index + sizeof(message->m_method_name_len);
            message->m_method_name = std::string(&tmp_buffer[method_name_index], message->m_method_name_len);
            DEBUGLOG("parse method_name=%s", message->m_method_name.c_str())

            // 解析 err_code
            uint32_t err_code_index = method_name_index + message->m_method_name_len;
            if (err_code_index >= end_index) {
                message->parse_success = false;
                ERRORLOG("parse error, err_code_index[%d] >= end_index[%d]", err_code_index, end_index)
                continue;
            }
            message->m_err_code = getInt32FromNetByte(&tmp_buffer[err_code_index]);

            // 解析 error_info_len
            uint32_t error_info_len_index = err_code_index + sizeof(message->m_err_code);
            if (error_info_len_index >= end_index) {
                message->parse_success = false;
                ERRORLOG("parse error, error_info_len_index[%d] >= end_index[%d]", error_info_len_index, end_index)
                continue;
            }
            message->m_err_info_len = getInt32FromNetByte(&tmp_buffer[error_info_len_index]);

            // 解析 error_info
            uint32_t err_info_index = error_info_len_index + sizeof(message->m_err_info_len);
            message->m_err_info = std::string(&tmp_buffer[err_info_index], message->m_err_info_len);
            DEBUGLOG("parse error_info=%s", message->m_err_info.c_str())

            // 解析 pb_data
            uint32_t pd_data_index = err_info_index + message->m_err_info_len;
            uint32_t pb_data_len = message->m_pk_len - message->m_msg_id_len -
                    message->m_method_name_len - message->m_err_info_len - 2 - 24;
            message->m_pb_data = std::string(&tmp_buffer[pd_data_index], pb_data_len);

            // 这里校验和去解析
            message->parse_success = true;

            // 将解析结果放入out_message, 并重新调整 buffer 中 readIndex 位置
            out_messages.push_back(message);
            buffer->moveReadIndex(static_cast<int>(end_index + 1 - start_index));
        }
    }
}

}