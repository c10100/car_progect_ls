#include "common.h"
#include "core/app_manager.h"
#include "core/config.h"
#include "net/net_utils.h"
#include "task/task_chat.h"

#define PROTO_BUF_SIZE 2048

static int       g_srv_fd = -1;         /* 与聊天服务器的 TCP 连接 */
static pthread_t g_tid;
static pthread_mutex_t g_send_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile int g_online = 0;

int chat_is_online(void) { return g_online; }

/* ---------- 内部: 统一发送一条协议消息 ---------- */
static int chat_send_msg(const proto_msg_t *msg)
{
    char buf[PROTO_BUF_SIZE];
    int  len;

    if (g_srv_fd < 0) {
        LOGW("not connected to server");
        return ERR;
    }
    pthread_mutex_lock(&g_send_lock);
    len = proto_pack(msg, buf, sizeof(buf));
    if (len > 0)
        len = tcp_send_all(g_srv_fd, buf, len);
    pthread_mutex_unlock(&g_send_lock);
    return len > 0 ? OK : ERR;
}

/* ---------- 业务接口实现 ---------- */
static int chat_connect_server(void)
{
    char ip[32];
    int  port;

    if (g_srv_fd >= 0) return OK;

    config_get_str(CFG_SRV_IP, "192.168.1.100", ip, sizeof(ip));
    port = config_get_int(CFG_SRV_PORT, 8888);

    g_srv_fd = tcp_connect(ip, port);
    if (g_srv_fd < 0) {
        app_manager_post(APP_MSG_NET_EVENT, "连接服务器失败", 0);
        return ERR;
    }
    LOGI("chat server connected: %s:%d", ip, port);
    return OK;
}

int chat_register(const char *account, const char *passwd,
                  const char *nickname, const char *ip, int port)
{
    proto_msg_t msg = {0};

    if (chat_connect_server() != OK) return ERR;

    msg.cmd = E_MSG_REGISTER;
    strncpy(msg.user.account,  account,  sizeof(msg.user.account)  - 1);
    strncpy(msg.user.passwd,   passwd,   sizeof(msg.user.passwd)   - 1);
    strncpy(msg.user.nickname, nickname, sizeof(msg.user.nickname) - 1);
    strncpy(msg.user.ip,       ip,       sizeof(msg.user.ip)       - 1);
    msg.user.port = port;
    return chat_send_msg(&msg);
}

int chat_login(const char *account, const char *passwd)
{
    proto_msg_t msg = {0};

    if (chat_connect_server() != OK) return ERR;

    msg.cmd = E_MSG_LOGIN;
    strncpy(msg.user.account, account, sizeof(msg.user.account) - 1);
    strncpy(msg.user.passwd,  passwd,  sizeof(msg.user.passwd)  - 1);
    return chat_send_msg(&msg);
}

int chat_logout(void)
{
    proto_msg_t msg = {0};
    msg.cmd = E_MSG_LOGOUT;
    config_get_str(CFG_ACCOUNT, "", msg.user.account, sizeof(msg.user.account));
    g_online = 0;
    return chat_send_msg(&msg);
}

int chat_get_online_list(void)
{
    proto_msg_t msg = {0};
    msg.cmd = E_MSG_GET_ONLINE;
    config_get_str(CFG_ACCOUNT, "", msg.user.account, sizeof(msg.user.account));
    return chat_send_msg(&msg);
}

int chat_send_text(const char *dst_account, const char *text)
{
    proto_msg_t msg = {0};
    msg.cmd = E_MSG_CHAT;
    config_get_str(CFG_ACCOUNT, "", msg.user.account, sizeof(msg.user.account));
    if (dst_account)
        strncpy(msg.dst_account, dst_account, sizeof(msg.dst_account) - 1);
    strncpy(msg.u.src_data, text, sizeof(msg.u.src_data) - 1);
    return chat_send_msg(&msg);
}

static int chat_set_common(int set_cmd, const char *val)
{
    proto_msg_t msg = {0};
    msg.cmd = E_MSG_SET;
    msg.sub_cmd = set_cmd;
    config_get_str(CFG_ACCOUNT, "", msg.user.account, sizeof(msg.user.account));
    strncpy(msg.u.cmdData.src_data, val, sizeof(msg.u.cmdData.src_data) - 1);
    return chat_send_msg(&msg);
}

int chat_set_passwd(const char *new_passwd) { return chat_set_common(E_SET_PASSWD, new_passwd); }
int chat_set_pic(const char *avatar_path)   { return chat_set_common(E_SET_PIC,    avatar_path); }
int chat_set_sign(const char *signature)    { return chat_set_common(E_SET_SIGN,   signature);   }

/* ---------- 接收线程 ---------- */
/*
 * 对应图中服务器逻辑的客户端镜像:
 *   switch(recvBuf.cmd)
 *     LOGIN_ACK     -> post(APP_MSG_LOGIN_ACK)
 *     ONLINE_LIST   -> post(APP_MSG_ONLINE_LIST)
 *     CHAT          -> post(APP_MSG_CHAT_MSG)
 *     ...
 */
static void handle_one_msg(const proto_msg_t *m)
{
    LOGD("recv cmd=%s from=%s", proto_cmd_str(m->cmd), m->user.nickname);

    switch (m->cmd) {
    case E_MSG_REGISTER_ACK:
    case E_MSG_LOGIN_ACK: {
        login_ack_t ack;
        ack.ok = (m->u.cmdData.src_data[0] == ACK_OK);
        strncpy(ack.tip, ack.ok ? "成功" : m->u.cmdData.src_data + 1,
                sizeof(ack.tip) - 1);
        if (m->cmd == E_MSG_LOGIN_ACK && ack.ok) {
            g_online = 1;
            chat_get_online_list();      /* 登录成功 -> 立刻请求在线列表(对应流程3) */
        }
        app_manager_post(APP_MSG_LOGIN_ACK, &ack, sizeof(ack));
        break;
    }
    case E_MSG_ONLINE_LIST: {
        /* 服务器每个在线用户发一条, sub_cmd = 序号(0起) */
        online_list_item_t item;
        item.user = m->user;
        item.idx  = m->sub_cmd;
        app_manager_post(APP_MSG_ONLINE_LIST, &item, sizeof(item));
        break;
    }

    case E_MSG_ONLINE_NTF:
        chat_get_online_list();          /* 有人上下线 -> 刷新列表 */
        break;

    case E_MSG_CHAT: {
        chat_msg_t cm;
        strncpy(cm.from, m->user.nickname, sizeof(cm.from) - 1);
        strncpy(cm.text, m->u.src_data,   sizeof(cm.text) - 1);
        cm.is_self = 0;
        app_manager_post(APP_MSG_CHAT_MSG, &cm, sizeof(cm));
        break;
    }

    default:
        break;
    }
}

static void *chat_recv_thread(void *arg)
{
    char buf[PROTO_BUF_SIZE];
    proto_msg_t msg;
    int n;

    (void)arg;
    while (1) {
        if (g_srv_fd < 0) { usleep(500 * 1000); continue; }

        /* 定长协议消息: 收满 sizeof(proto_msg_t) 才算一条 */
        n = tcp_recv_all(g_srv_fd, buf, sizeof(proto_msg_t));
        if (n <= 0) {
            LOGW("server disconnected, fd=%d", g_srv_fd);
            net_close(g_srv_fd);
            g_srv_fd = -1;
            g_online = 0;
            app_manager_post(APP_MSG_NET_EVENT, "服务器断开", 0);
            sleep(2);                    /* 断线重连节奏 */
            continue;
        }

        if (proto_unpack(buf, n, &msg) == OK)
            handle_one_msg(&msg);
    }
    return NULL;
}

int task_chat_start(void)
{
    if (pthread_create(&g_tid, NULL, chat_recv_thread, NULL) != 0) {
        LOGE("create chat thread failed");
        return ERR;
    }
    pthread_detach(g_tid);
    LOGI("chat thread started");
    return OK;
}
