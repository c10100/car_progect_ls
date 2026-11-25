#ifndef __CARPAD_APP_H__
#define __CARPAD_APP_H__

/*
 * carpad_app.h - 车机应用入口 (供外部工程 main.c 调用)
 *
 * sdl2 模拟器工程 (car_progect/sdl2) 与板级工程 (LVGL) 的 main.c
 * 都只需调用 carpad_app_init() 一次, 再在主循环里加 app_manager_poll().
 * mycode 内部不放 main(), 避免与外部工程入口冲突。
 */

#include "core/app_manager.h"

int carpad_app_init(void);   /* 初始化框架+界面+后台任务, 返回 0 成功 */

#endif /* __CARPAD_APP_H__ */
