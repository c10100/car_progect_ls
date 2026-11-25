#include "common.h"
#include "core/app_manager.h"
#include "core/config.h"
#include "net/net_utils.h"
#include "task/task_game.h"

static int       g_udp_fd = -1;
static char      g_srv_ip[32];
static int       g_srv_port = 9998;
static pthread_t g_tid;

int game_set_server(const char *ip, int port)
{
    strncpy(g_srv_ip, ip, sizeof(g_srv_ip) - 1);
    g_srv_port = port;
    return OK;
}

/*
 * 发本地蛇状态到服务器。
 * 教学进阶 TODO:
 *   - 直接发整个 game_state_t 简单但流量大, 优化为只发 {player_id, 头坐标, 方向, 长度}
 *   - 服务器以固定节奏广播, 客户端做插值平滑
 */
int game_send_state(const game_state_t *st)
{
    if (g_udp_fd < 0) return ERR;
    return udp_send_to(g_udp_fd, g_srv_ip, g_srv_port, st, sizeof(*st));
}

/* 接收线程: 收对端(经服务器中转)的蛇状态 -> 投递 UI */
static void *game_recv_thread(void *arg)
{
    game_state_t st;
    struct sockaddr_in peer;
    socklen_t peer_len;
    int n;

    (void)arg;
    while (1) {
        if (g_udp_fd < 0) { usleep(500 * 1000); continue; }

        peer_len = sizeof(peer);
        n = recvfrom(g_udp_fd, &st, sizeof(st), 0,
                     (struct sockaddr *)&peer, &peer_len);
        if (n != sizeof(st)) continue;          /* 丢残包, UDP 丢包属正常 */

        app_manager_post(APP_MSG_GAME_STATE, &st, sizeof(st));
    }
    return NULL;
}

int task_game_start(void)
{
    char ip[32];

    config_get_str(CFG_SRV_IP, "192.168.1.100", ip, sizeof(ip));
    game_set_server(ip, 9998);

    g_udp_fd = udp_create(GAME_UDP_PORT);
    if (g_udp_fd < 0) {
        LOGE("game udp create failed");
        return ERR;
    }

    if (pthread_create(&g_tid, NULL, game_recv_thread, NULL) != 0) {
        LOGE("create game thread failed");
        return ERR;
    }
    pthread_detach(g_tid);
    LOGI("game thread started (udp %d <-> %s:9998)", GAME_UDP_PORT, g_srv_ip);
    return OK;
}
