/*
 * main.c - 车载娱乐平板主程序入口
 *
 * 代码组织 (自底向上):
 *   第1层 core  : 消息队列 / 配置 / 应用管理器        (数据结构+系统编程+文件IO)
 *   第2层 net   : TCP/UDP 封装 + HTTP客户端 + 协议    (网络编程)
 *   第3层 task  : 天气/AI/聊天/游戏 后台线程          (业务逻辑)
 *   第4层 ui    : LVGL 页面 (主页状态栏 + 应用)       (图形界面)
 *
 * 线程模型:
 *   [LVGL主线程]  lv_timer_handler() + app_manager_poll()
 *   [天气线程]    定时 HTTP GET -> cJSON 解析 -> post(APP_MSG_WEATHER)
 *   [聊天线程]    TCP 收消息 -> post(APP_MSG_CHAT_MSG)
 *   [游戏线程]    UDP 收发对端状态 -> post(APP_MSG_GAME_STATE)
 *   (AI 助手为按需请求: UI 发问 -> 工作线程 HTTP POST -> post 回来)
 */

#include "lvgl.h"           /* LVGL 主头文件, 路径在 CMakeLists.txt 指定 */
#include "common.h"
#include "core/config.h"
#include "core/app_manager.h"
#include "task/task_weather.h"
#include "task/task_chat.h"
#include "task/task_game.h"
#include "ui/ui.h"

/* 板级移植层接口: LCD初始化/触摸驱动/LVGL显示注册, 由你们已有的移植工程提供.
 * 如果你们的工程是 main 里直接 lv_init() + display_init(), 对应改这里即可. */
extern void lv_port_init(void);   /* TODO: 接你们板子的 LVGL 移植层 */

int main(void)
{
    /* 屏蔽 SIGPIPE: 向已断开的 socket 写入时默认会杀死进程 */
    signal(SIGPIPE, SIG_IGN);

    /* 1. 板级 + LVGL 初始化 (显示/触摸) */
    lv_port_init();

    /* 2. 框架初始化 */
    config_init(CFG_PATH_DEFAULT);      /* 文件IO: 读账号/城市/服务器IP */
    app_manager_init();                 /* 线程间消息队列 */

    /* 3. 创建 CARPAD OS 界面 (登录屏 + 系统屏7页面, 仿SquareLine结构) */
    ui_init();

    /* 4. 启动后台业务线程 */
    task_weather_start();               /* 天气: http + cJSON, 必写 */
    task_chat_start();                  /* 车友聊天室: TCP 注册/登录, 加分 */
    task_game_start();                  /* 联机游戏: UDP, 加分 (可在游戏页扩展联机) */

    /* 5. 主循环: LVGL 渲染 + 分发后台消息 */
    LOGI("carpad started");
    while (1) {
        lv_timer_handler();             /* LVGL 任务处理, 5ms 一次 */
        usleep(5000);
        app_manager_poll();             /* 取后台消息 -> 刷新 UI (主线程内, 安全) */
    }

    return 0;
}
