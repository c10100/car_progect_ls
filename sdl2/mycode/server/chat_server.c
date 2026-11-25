/*
 * chat_server.c - 聊天室服务器 (加分项, 在 PC 或开发板上运行)
 *
 * 编译(PC): cmake -B build-server -DBUILD_CLIENT=OFF && cmake --build build-server
 * 运行: ./build-server/bin/chat_server 8888
 *
 * 核心点:
 *   - select() 多路复用管理所有客户端 (单线程, IO多路复用)
 *   - 用户数据持久化到 users.dat (文件IO, 对应图: 注册发所有信息)
 *   - 在线用户表: 数组 (进阶可改链表, 对应图: 获取服务器所有在线用户信息)
 *   - switch(recvBuf.cmd) 分发, 对应课程设计图右侧逻辑
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "../include/protocol.h"

#define MAX_CLIENTS  32
#define USER_DB      "users.dat"
#define LOG_TS() ({ time_t _t=time(NULL); struct tm _m; localtime_r(&_t,&_m); \
                    printf("[%02d:%02d:%02d] ", _m.tm_hour,_m.tm_min,_m.tm_sec); })

/* ---------------- 在线客户端表 ---------------- */
typedef struct {
    int          fd;                    /* -1 = 空位 */
    int          online;                /* 是否已登录 */
    user_info_t  user;                  /* 注册/登录信息 */
    struct sockaddr_in addr;            /* 对端地址 */
} client_t;

static client_t g_clients[MAX_CLIENTS];

/* ---------------- 用户数据库 (文件) ---------------- */
typedef struct {
    user_info_t u;
    int used;
} db_user_t;

static db_user_t g_db[256];
static int g_db_cnt = 0;

static void db_load(void)
{
    FILE *fp = fopen(USER_DB, "r");
    if (!fp) return;
    while (g_db_cnt < 256 &&
           fscanf(fp, "%31s %31s %31s %127s %63s %15s %d\n",
                  g_db[g_db_cnt].u.account, g_db[g_db_cnt].u.passwd,
                  g_db[g_db_cnt].u.nickname, g_db[g_db_cnt].u.signature,
                  g_db[g_db_cnt].u.avatar, g_db[g_db_cnt].u.ip,
                  &g_db[g_db_cnt].u.port) == 7) {
        g_db[g_db_cnt].used = 1;
        g_db_cnt++;
    }
    fclose(fp);
    printf("loaded %d users from %s\n", g_db_cnt, USER_DB);
}

static void db_save(void)
{
    FILE *fp = fopen(USER_DB, "w");
    if (!fp) { perror("save users"); return; }
    for (int i = 0; i < g_db_cnt; i++)
        fprintf(fp, "%s %s %s %s %s %s %d\n",
                g_db[i].u.account, g_db[i].u.passwd, g_db[i].u.nickname,
                g_db[i].u.signature, g_db[i].u.avatar, g_db[i].u.ip,
                g_db[i].u.port);
    fclose(fp);
}

static db_user_t *db_find(const char *account)
{
    for (int i = 0; i < g_db_cnt; i++)
        if (strcmp(g_db[i].u.account, account) == 0)
            return &g_db[i];
    return NULL;
}

/* ---------------- 发送辅助 ---------------- */
static int send_msg(int fd, const proto_msg_t *m)
{
    return send(fd, m, sizeof(*m), MSG_NOSIGNAL) == sizeof(*m) ? 0 : -1;
}

static client_t *find_by_account(const char *account)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (g_clients[i].fd > 0 && g_clients[i].online &&
            strcmp(g_clients[i].user.account, account) == 0)
            return &g_clients[i];
    return NULL;
}

/* ---------------- 命令分发 (对应图: switch(recvBuf.cmd)) ---------------- */
static void handle_register(client_t *c, const proto_msg_t *m)
{
    proto_msg_t ack = {0};
    ack.cmd = E_MSG_REGISTER_ACK;

    if (db_find(m->user.account)) {
        ack.u.cmdData.src_data[0] = ACK_FAIL;
        strcat(ack.u.cmdData.src_data, "账号已存在");
    } else {
        g_db[g_db_cnt].u = m->user;
        g_db[g_db_cnt].used = 1;
        g_db_cnt++;
        db_save();
        ack.u.cmdData.src_data[0] = ACK_OK;
        LOG_TS(); printf("register ok: %s(%s)\n", m->user.account,
                         inet_ntoa(c->addr.sin_addr));
    }
    send_msg(c->fd, &ack);
}

static void broadcast_ntf(int online, const char *account)
{
    /* 有人上下线 -> 通知所有在线客户端刷新列表 */
    proto_msg_t ntf = {0};
    ntf.cmd = E_MSG_ONLINE_NTF;
    ntf.sub_cmd = online;
    strncpy(ntf.user.account, account, sizeof(ntf.user.account) - 1);
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (g_clients[i].fd > 0 && g_clients[i].online)
            send_msg(g_clients[i].fd, &ntf);
}

static void handle_login(client_t *c, const proto_msg_t *m)
{
    proto_msg_t ack = {0};
    ack.cmd = E_MSG_LOGIN_ACK;
    db_user_t *du = db_find(m->user.account);

    if (!du || strcmp(du->u.passwd, m->user.passwd) != 0) {
        ack.u.cmdData.src_data[0] = ACK_FAIL;
        strcat(ack.u.cmdData.src_data, "账号或密码错误");
        LOG_TS(); printf("login FAIL: %s\n", m->user.account);
    } else {
        c->online = 1;
        c->user = du->u;
        /* 记录客户端真实 IP/端口 (对应图: 服务器收集在线用户 IP 端口) */
        strncpy(c->user.ip, inet_ntoa(c->addr.sin_addr), sizeof(c->user.ip) - 1);
        c->user.port = ntohs(c->addr.sin_port);

        ack.u.cmdData.src_data[0] = ACK_OK;
        LOG_TS(); printf("login ok: %s @ %s:%d\n",
                         m->user.account, c->user.ip, c->user.port);
    }
    send_msg(c->fd, &ack);
    if (c->online) broadcast_ntf(1, c->user.account);
}

static void handle_get_online(client_t *c, const proto_msg_t *m)
{
    (void)m;
    int idx = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i].fd <= 0 || !g_clients[i].online) continue;
        proto_msg_t one = {0};
        one.cmd = E_MSG_ONLINE_LIST;
        one.sub_cmd = idx++;               /* 序号, 0 表示列表开始 */
        one.user = g_clients[i].user;
        send_msg(c->fd, &one);
    }
    LOG_TS(); printf("online list -> %s (%d users)\n", c->user.account, idx);
}

static void handle_chat(client_t *c, const proto_msg_t *m)
{
    proto_msg_t out = *m;
    strncpy(out.user.account,  c->user.account,  sizeof(out.user.account) - 1);
    strncpy(out.user.nickname, c->user.nickname, sizeof(out.user.nickname) - 1);

    if (m->dst_account[0] == '\0') {          /* 群发: 所有在线者 */
        for (int i = 0; i < MAX_CLIENTS; i++)
            if (g_clients[i].fd > 0 && g_clients[i].online)
                send_msg(g_clients[i].fd, &out);
        LOG_TS(); printf("chat[all] %s: %s\n", c->user.account, m->u.src_data);
    } else {                                  /* 私聊 */
        client_t *dst = find_by_account(m->dst_account);
        if (dst) {
            send_msg(dst->fd, &out);
            LOG_TS(); printf("chat[%s->%s]: %s\n",
                             c->user.account, m->dst_account, m->u.src_data);
        }
    }
}

static void handle_set(client_t *c, const proto_msg_t *m)
{
    db_user_t *du = db_find(c->user.account);
    if (!du) return;

    switch (m->sub_cmd) {
    case E_SET_PASSWD:                        /* 更新客户端的密码(对应图) */
        strncpy(du->u.passwd, m->u.cmdData.src_data, sizeof(du->u.passwd) - 1);
        break;
    case E_SET_PIC:                           /* 更新客户端的头像(对应图) */
        strncpy(du->u.avatar, m->u.cmdData.src_data, sizeof(du->u.avatar) - 1);
        break;
    case E_SET_SIGN:
        strncpy(du->u.signature, m->u.cmdData.src_data, sizeof(du->u.signature) - 1);
        break;
    default:
        return;
    }
    db_save();
    LOG_TS(); printf("set[%d] %s ok\n", m->sub_cmd, c->user.account);
}

static void dispatch(client_t *c, const proto_msg_t *m)
{
    LOG_TS(); printf("cmd=%s from=%s\n", proto_cmd_str(m->cmd),
                     m->user.account[0] ? m->user.account : "?");
    switch (m->cmd) {
    case E_MSG_REGISTER:    handle_register(c, m);    break;
    case E_MSG_LOGIN:       handle_login(c, m);       break;
    case E_MSG_LOGOUT:
        c->online = 0;
        broadcast_ntf(0, c->user.account);
        break;
    case E_MSG_GET_ONLINE:  handle_get_online(c, m);  break;
    case E_MSG_CHAT:        handle_chat(c, m);        break;
    case E_MSG_SET:         handle_set(c, m);         break;
    default:
        LOG_TS(); printf("unknown cmd %d\n", m->cmd);
    }
}

/* ---------------- 主循环: select ---------------- */
int main(int argc, char **argv)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in srv = {0};
    int port = argc > 1 ? atoi(argv[1]) : 8888;
    int reuse = 1;

    signal(SIGPIPE, SIG_IGN);
    memset(g_clients, 0, sizeof(g_clients));
    for (int i = 0; i < MAX_CLIENTS; i++) g_clients[i].fd = -1;
    db_load();

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    srv.sin_family      = AF_INET;
    srv.sin_port        = htons(port);
    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listen_fd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
        perror("bind"); return 1;
    }
    listen(listen_fd, 16);
    printf("chat server listening on %d\n", port);

    while (1) {
        fd_set rset;
        int maxfd = listen_fd;
        FD_ZERO(&rset);
        FD_SET(listen_fd, &rset);
        for (int i = 0; i < MAX_CLIENTS; i++)
            if (g_clients[i].fd > 0) {
                FD_SET(g_clients[i].fd, &rset);
                if (g_clients[i].fd > maxfd) maxfd = g_clients[i].fd;
            }

        if (select(maxfd + 1, &rset, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("select"); break;
        }

        /* 新连接 */
        if (FD_ISSET(listen_fd, &rset)) {
            struct sockaddr_in cli;
            socklen_t len = sizeof(cli);
            int fd = accept(listen_fd, (struct sockaddr *)&cli, &len);
            if (fd >= 0) {
                for (int i = 0; i < MAX_CLIENTS; i++)
                    if (g_clients[i].fd < 0) {
                        g_clients[i].fd = fd;
                        g_clients[i].addr = cli;
                        g_clients[i].online = 0;
                        printf("client #%d connected %s:%d\n",
                               i, inet_ntoa(cli.sin_addr), ntohs(cli.sin_port));
                        break;
                    }
            }
        }

        /* 已连接客户端的数据 */
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = g_clients[i].fd;
            if (fd <= 0 || !FD_ISSET(fd, &rset)) continue;

            proto_msg_t msg;
            ssize_t n = recv(fd, &msg, sizeof(msg), 0);
            if (n <= 0) {                       /* 断开 */
                printf("client #%d (%s) disconnected\n", i, g_clients[i].user.account);
                if (g_clients[i].online) broadcast_ntf(0, g_clients[i].user.account);
                close(fd);
                g_clients[i].fd = -1;
                continue;
            }
            if (n == sizeof(msg))               /* 完整一条 */
                dispatch(&g_clients[i], &msg);
        }
    }
    return 0;
}
