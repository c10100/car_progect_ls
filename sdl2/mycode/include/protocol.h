#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

/*
 * protocol.h - 自定义应用层协议 (客户端 <-> 聊天/游戏服务器共用)
 *
 * 对应课程设计图:
 *   struct ClientMsg {
 *       int cmd;            // 数据类型: 命令 还是 聊天数据
 *       union {
 *           struct cmdMsg cmdData;   // 设置类命令
 *           struct srcData srcMsg;   // 聊天数据
 *       };
 *   };
 *
 * 说明:
 *   1. TCP 是字节流, 一次 recv 可能收到半条/多条消息, 实际项目中建议
 *      采用 "定长包头(总长度) + 包体" 或 '\n' 分隔; 本工程消息体固定
 *      sizeof(proto_msg_t), 配合循环 recv 保证收完整 (见 net_utils.c).
 *   2. 结构体直接 memcpy 传输 (教学项目), 要求两端编译器/对齐一致;
 *      正式项目应序列化 (cJSON / protobuf) 或手工封包.
 */

/* ===================== 命令字定义 ===================== */
typedef enum {
    /* 账号相关 */
    E_MSG_REGISTER     = 1,   /* 注册: 携带 user_info_t                    */
    E_MSG_REGISTER_ACK = 2,   /* 注册应答: src_data[0] 1=成功 0=失败(已存在) */
    E_MSG_LOGIN        = 3,   /* 登录: 携带 account + passwd                */
    E_MSG_LOGIN_ACK    = 4,   /* 登录应答: src_data[0] 1=正确 0=错误         */
    E_MSG_LOGOUT       = 5,   /* 下线                                      */

    /* 在线列表 */
    E_MSG_GET_ONLINE   = 6,   /* 请求获取所有在线用户                       */
    E_MSG_ONLINE_LIST  = 7,   /* 服务器下发在线列表: user 数组              */
    E_MSG_ONLINE_NTF   = 8,   /* 上线/下线 通知 (sub_cmd: 1上线 0下线)      */

    /* 聊天 */
    E_MSG_CHAT         = 9,   /* 聊天数据: src_data 为文本, dst_account 指定接收者 */

    /* 设置 (对应图中 E_CLIENT_SET / E_SET_PASSWD / E_SET_PIC) */
    E_MSG_SET          = 10,  /* 设置类, 具体子命令见 set_cmd_e            */

    /* 游戏数据 (联机贪吃蛇) */
    E_MSG_GAME         = 11,  /* 游戏数据, 通过 UDP 中转服务器收发          */
} proto_cmd_e;

/* 设置类子命令 */
typedef enum {
    E_SET_PASSWD = 1,         /* 修改密码: src_data = 新密码                */
    E_SET_PIC    = 2,         /* 修改头像: src_data = 头像文件路径/图片数据 */
    E_SET_SIGN   = 3,         /* 修改个性签名: src_data = 签名文本          */
} set_cmd_e;

/* ===================== 数据结构 ===================== */
/* 用户信息 (对应截图: IP地址 端口号 账号 密码 昵称 个性签名 头像) */
typedef struct {
    char account[32];         /* 账号 */
    char passwd[32];          /* 密码 */
    char nickname[32];        /* 昵称 */
    char signature[128];      /* 个性签名 */
    char avatar[64];          /* 头像资源路径, 如 "A:/avatar/1.png" */
    char ip[16];              /* 客户端 IP (注册时填写) */
    int  port;                /* 客户端端口 */
} user_info_t;

/* 命令包体 (对应图中 struct cmdMsg) */
typedef struct {
    int  set_cmd;             /* set_cmd_e */
    char src_data[1024];      /* 附带数据 */
} proto_cmd_msg_t;

/* 一条完整协议消息 (对应图中 struct ClientMsg) */
typedef struct {
    int  cmd;                 /* proto_cmd_e */
    int  sub_cmd;             /* 子命令/标志位 */
    user_info_t user;         /* 发送者信息 (服务器填) */
    char dst_account[32];     /* 私聊目标账号, 为空表示群发 */
    union {
        proto_cmd_msg_t cmdData;          /* 设置/命令类 */
        char src_data[1032];              /* 聊天/游戏原始数据 (字节对齐后与cmdData等大) */
    } u;
} proto_msg_t;

/* 登录应答专用, 简化取值 */
#define ACK_OK    1
#define ACK_FAIL  0

/* ===================== 协议编解码 API ===================== */
int  proto_pack(const proto_msg_t *msg, char *buf, int buf_len);
int  proto_unpack(const char *buf, int len, proto_msg_t *msg);
const char *proto_cmd_str(int cmd);   /* 调试用: cmd 转可读字符串 */

#endif /* __PROTOCOL_H__ */
