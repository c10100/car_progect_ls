#include "common.h"
#include "protocol.h"

/*
 * 协议编解码: 教学项目直接按结构体二进制拷贝。
 * 优点: 简单; 缺点: 依赖两端对齐一致。
 * 若要跨平台稳妥, 可改造为逐字段 memcpy 或 cJSON 序列化 (TODO)。
 */

int proto_pack(const proto_msg_t *msg, char *buf, int buf_len)
{
    if (!msg || !buf) return ERR;
    if (buf_len < (int)sizeof(proto_msg_t)) {
        LOGE("buf too small: %d < %d", buf_len, (int)sizeof(proto_msg_t));
        return ERR;
    }
    memcpy(buf, msg, sizeof(proto_msg_t));
    return sizeof(proto_msg_t);
}

int proto_unpack(const char *buf, int len, proto_msg_t *msg)
{
    if (!buf || !msg) return ERR;
    if (len < (int)sizeof(proto_msg_t)) {
        LOGE("recv len %d < msg size %d, incomplete packet", len, (int)sizeof(proto_msg_t));
        return ERR;
    }
    memcpy(msg, buf, sizeof(proto_msg_t));
    return OK;
}

const char *proto_cmd_str(int cmd)
{
    switch (cmd) {
    case E_MSG_REGISTER:     return "REGISTER";
    case E_MSG_REGISTER_ACK: return "REGISTER_ACK";
    case E_MSG_LOGIN:        return "LOGIN";
    case E_MSG_LOGIN_ACK:    return "LOGIN_ACK";
    case E_MSG_LOGOUT:       return "LOGOUT";
    case E_MSG_GET_ONLINE:   return "GET_ONLINE";
    case E_MSG_ONLINE_LIST:  return "ONLINE_LIST";
    case E_MSG_ONLINE_NTF:   return "ONLINE_NTF";
    case E_MSG_CHAT:         return "CHAT";
    case E_MSG_SET:          return "SET";
    case E_MSG_GAME:         return "GAME";
    default:                 return "UNKNOWN";
    }
}
