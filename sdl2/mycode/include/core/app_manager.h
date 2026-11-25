#ifndef __APP_MANAGER_H__
#define __APP_MANAGER_H__

/*
 * app_manager.h - 应用管理器 (框架核心)
 *
 * 职责:
 *   1. 维护一个全局 UI 消息队列, 任何后台线程都能 app_manager_post()
 *   2. 主循环里 app_manager_poll() 取出消息, 按 type 分发给已订阅的 UI 回调
 *   3. 页面切换管理 (表盘/天气/聊天/游戏/AI)
 *
 * 数据流:
 *   [task_weather 线程] --post(WEATHER)--> [全局队列] --poll--> [ui_weather 回调刷UI]
 */

#include "core/msg_queue.h"

/* 消息类型: 每个功能一种 */
typedef enum {
    APP_MSG_WEATHER     = 0,   /* 天气数据到达       */
    APP_MSG_AI_REPLY    = 1,   /* AI 回复到达        */
    APP_MSG_CHAT_MSG    = 2,   /* 收到聊天消息       */
    APP_MSG_ONLINE_LIST = 3,   /* 在线好友列表更新   */
    APP_MSG_LOGIN_ACK   = 4,   /* 登录/注册应答      */
    APP_MSG_GAME_STATE  = 5,   /* 对端游戏状态       */
    APP_MSG_NET_EVENT   = 6,   /* 网络事件(断线等)   */
    APP_MSG_MAX,
} app_msg_type_e;

typedef struct {
    app_msg_type_e type;
    void          *data;    /* 指向该功能的结构体, 由 post 时 malloc */
    int            len;
} app_msg_t;

/* UI 订阅回调: 在 LVGL 主线程上下文中执行, 可安全调用 lv_xxx() */
typedef void (*app_msg_cb_t)(app_msg_t *msg);

int  app_manager_init(void);

/* 任意线程调用: 深拷贝 data 并入队 */
int  app_manager_post(app_msg_type_e type, const void *data, int len);

/* 主循环调用: 把队列里的消息分发给回调 (非阻塞) */
void app_manager_poll(void);

/* UI 页面订阅: 同一 type 后订阅覆盖先订阅 (只投递给当前显示页面) */
void app_manager_subscribe(app_msg_type_e type, app_msg_cb_t cb);

#endif /* __APP_MANAGER_H__ */
