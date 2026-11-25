#ifndef __TASK_CHAT_H__
#define __TASK_CHAT_H__

/*
 * task_chat.h - 聊天室客户端 (加分项: TCP + 自定义协议)
 *
 * 对应设计图流程:
 *   1. 注册: 所有信息(IP 端口号 账号 密码 昵称...)发给服务器
 *   2. 登录: 发账号+密码 -> 服务器验证 -> 回 ACK (1正确 0错误)
 *   3. 登录成功 -> 客户端请求在线列表 -> 好友列表 UI 显示
 *   4. 聊天 / 设置(改密码 改头像 改签名)
 *
 * 收消息: 接收线程常驻, 收到后 post 给 UI:
 *   APP_MSG_LOGIN_ACK   登录/注册应答
 *   APP_MSG_ONLINE_LIST 在线好友列表
 *   APP_MSG_CHAT_MSG    聊天消息
 */

#define ACCOUNT_LEN  32
#define CHAT_TXT_LEN 1024

#include "protocol.h"     /* user_info_t (在线列表条目用到) */

typedef struct {
    int  ok;                      /* ACK_OK / ACK_FAIL */
    char tip[64];                 /* 失败原因提示 */
} login_ack_t;

typedef struct {
    char from[ACCOUNT_LEN];       /* 发送者昵称 */
    char text[CHAT_TXT_LEN];      /* 消息内容 */
    int  is_self;                 /* 是否自己发的(回显) */
} chat_msg_t;

/* 在线列表条目: 服务器每用户发一条协议消息, idx=0 表示列表开始(UI先清空) */
typedef struct {
    user_info_t user;
    int         idx;
} online_list_item_t;

int task_chat_start(void);

/* ---------- 业务接口 (UI 层调用) ---------- */
int chat_register(const char *account, const char *passwd,
                  const char *nickname, const char *ip, int port);
int chat_login(const char *account, const char *passwd);
int chat_logout(void);

/* 请求服务器在线用户列表 -> 服务器回 E_MSG_ONLINE_LIST */
int chat_get_online_list(void);

/* 发聊天消息: dst 为空 = 群发 */
int chat_send_text(const char *dst_account, const char *text);

/* 设置类: 修改密码/头像/签名 (对应图中 E_SET_PASSWD / E_SET_PIC) */
int chat_set_passwd(const char *new_passwd);
int chat_set_pic(const char *avatar_path);
int chat_set_sign(const char *signature);

/* 当前连接状态: 1在线 0断开 */
int chat_is_online(void);

#endif /* __TASK_CHAT_H__ */
