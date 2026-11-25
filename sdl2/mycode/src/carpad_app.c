/*
 * carpad_app.c - 车机应用组装层 (唯一被外部 main.c 调用的入口)
 *
 * 初始化顺序与原 src/main.c 一致:
 *   config -> app_manager -> ui -> 后台任务线程
 * PC 模拟器 (sdl2 工程) 与板子 (LVGL 工程) 共用同一份业务代码:
 *   src/ui + src/core + src/task + include/
 */

#include "common.h"
#include "carpad_app.h"
#include "core/config.h"
#include "core/app_manager.h"
#include "task/task_weather.h"
#include "task/task_chat.h"
#include "task/task_game.h"
#include "ui/ui.h"

int carpad_app_init(void)
{
    /* 1. 配置文件: PC 模拟器写在当前目录, 板上用 /userdata/watch.cfg */
#ifdef CARPAD_PC_SIM
    config_init("carpad.cfg");
#else
    config_init(CFG_PATH_DEFAULT);
#endif

    /* 2. 框架 + 界面 (登录屏 + 系统屏) */
    app_manager_init();
    ui_init();

    /* 3. 后台业务线程 */
    task_weather_start();               /* 天气: http + cJSON */
    task_chat_start();                  /* 车友聊天室: TCP */
    task_game_start();                  /* 联机游戏: UDP */

    LOGI("carpad app started");
    return 0;
}
