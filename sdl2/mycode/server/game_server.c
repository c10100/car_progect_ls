/*
 * game_server.c - 联机贪吃蛇中转服务器 (UDP)
 *
 * 编译(PC): cmake -B build-server -DBUILD_CLIENT=OFF && cmake --build build-server
 * 运行: ./build-server/bin/game_server 9998
 *
 * 逻辑: 收到任一玩家的 game_state_t -> 转发给其他所有玩家。
 * UDP 无连接, 用 addr 表记住最近活跃的玩家 (超时自动剔除)。
 * TODO 进阶: 服务器权威模式 (食物由服务器生成/碰撞判定在服务器)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../include/task/task_game.h"

#define MAX_PEERS 8
#define PEER_TTL_S 10

typedef struct {
    struct sockaddr_in addr;
    time_t last_seen;
    int used;
} peer_t;

static peer_t g_peers[MAX_PEERS];

static int same_addr(const struct sockaddr_in *a, const struct sockaddr_in *b)
{
    return a->sin_addr.s_addr == b->sin_addr.s_addr &&
           a->sin_port == b->sin_port;
}

int main(int argc, char **argv)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in srv = {0}, cli;
    int port = argc > 1 ? atoi(argv[1]) : 9998;
    socklen_t len;
    game_state_t st;
    int reuse = 1;

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    srv.sin_family      = AF_INET;
    srv.sin_port        = htons(port);
    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
        perror("bind"); return 1;
    }
    printf("game relay server on udp %d\n", port);

    while (1) {
        len = sizeof(cli);
        if (recvfrom(fd, &st, sizeof(st), 0, (struct sockaddr *)&cli, &len)
                != sizeof(st))
            continue;

        /* 记录/刷新发送者 */
        int found = 0;
        time_t now = time(NULL);
        for (int i = 0; i < MAX_PEERS; i++) {
            if (g_peers[i].used && same_addr(&g_peers[i].addr, &cli)) {
                g_peers[i].last_seen = now;
                found = 1;
                break;
            }
        }
        if (!found)
            for (int i = 0; i < MAX_PEERS; i++)
                if (!g_peers[i].used || now - g_peers[i].last_seen > PEER_TTL_S) {
                    g_peers[i].addr = cli;
                    g_peers[i].last_seen = now;
                    g_peers[i].used = 1;
                    printf("peer joined %s:%d (player %d)\n",
                           inet_ntoa(cli.sin_addr), ntohs(cli.sin_port),
                           st.player_id);
                    break;
                }

        /* 转发给除发送者之外的所有玩家 */
        for (int i = 0; i < MAX_PEERS; i++)
            if (g_peers[i].used && !same_addr(&g_peers[i].addr, &cli) &&
                now - g_peers[i].last_seen <= PEER_TTL_S)
                sendto(fd, &st, sizeof(st), 0,
                       (struct sockaddr *)&g_peers[i].addr, sizeof(cli));
    }
    return 0;
}
