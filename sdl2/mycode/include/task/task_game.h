#ifndef __TASK_GAME_H__
#define __TASK_GAME_H__

/*
 * task_game.h - 联机贪吃蛇 (加分项: UDP)
 *
 * 为什么用 UDP: 游戏状态要求实时性, 丢一帧没关系, TCP 重传反而卡顿。
 * 架构: 经服务器中转 (game_server.c), 两块板子互发蛇的状态。
 *
 * 分工建议:
 *   - 蛇的移动/吃食物/死亡判定: 本地逻辑 (数据结构: 数组/链表实现蛇身)
 *   - 本模块只负责: 状态收发 + 对端状态投递给 UI
 */

#define GAME_MAX_BODY  64         /* 蛇身最大长度 */
#define GRID_W         30         /* 棋盘格数 */
#define GRID_H         20
#define GAME_UDP_PORT  9999       /* 本机 UDP 收包端口 */

/* 玩家ID: 1P / 2P */
#define GAME_PLAYER_1  1
#define GAME_PLAYER_2  2

typedef struct {
    int  player_id;               /* 谁的状态 */
    int  len;                     /* 蛇身长度 */
    int  body_x[GAME_MAX_BODY];   /* 蛇身坐标(格子) */
    int  body_y[GAME_MAX_BODY];
    int  food_x, food_y;          /* 食物位置(服务器/房主同步) */
    int  score;
    int  alive;                   /* 1存活 0死亡 */
} game_state_t;

int task_game_start(void);

/* 把本地蛇的状态发到服务器 (游戏主循环里每帧/每次移动调用) */
int game_send_state(const game_state_t *st);

/* 设置服务器地址(默认从 config 读) */
int game_set_server(const char *ip, int port);

#endif /* __TASK_GAME_H__ */
