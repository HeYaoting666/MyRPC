//
// Created by 20771 on 2024/1/31.
//

#include "msg_id.h"

namespace rocket {

static int g_msg_id_length = 20;
static int g_random_fd = -1;

static thread_local std::string t_msg_id_no;
static thread_local std::string t_max_msg_id_no;

std::string MsgIDUtil::GetMsgID() {
    if(t_msg_id_no.empty() || t_msg_id_no == t_max_msg_id_no) {
        // 获取随机文件句柄
        if(g_random_fd == -1)
            g_random_fd = open("/dev/urandom", O_RDONLY);

        std::string res(g_msg_id_length, 0);
        if( (read(g_random_fd, &res[0], g_msg_id_length)) != g_msg_id_length ) {
            ERRORLOG("%s", "read form /dev/urandom error")
            return "";
        }

        for(int i = 0; i < g_msg_id_length; ++i) {
            uint8_t x = static_cast<uint8_t>(res[i]) % 10;
            res[i] = x + '0';
            t_max_msg_id_no += "9";
        }
        t_msg_id_no = res;
    }
    else {
        size_t i = t_msg_id_no.length() - 1;
        while (t_msg_id_no[i] == '9' && i > 0) {
            i--;
        }
        t_msg_id_no[i] += 1;
        for (size_t j = i + 1; j < t_msg_id_no.length(); ++j) {
            t_msg_id_no[j] = '0';
        }
    }

    return t_msg_id_no;
}

}