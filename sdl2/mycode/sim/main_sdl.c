/*
 * sim/main_sdl.c - PC 端 SDL2 模拟器入口 (WSL / Windows 均可)
 *
 * 与板上 main.c 的差别:
 *   1. 显示/触摸换成 lv_drivers 的 SDL 窗口 (1024x600, 鼠标=触摸, 键盘=输入)
 *   2. 不编译 task_chat/task_weather (BSD socket 网络任务), 用桩函数代替:
 *        登录/注册 -> 直接回登录 ACK
 *        天气刷新  -> 回一份演示数据
 *   3. 配置文件写在运行目录 carpad.cfg (板上为 /userdata/watch.cfg)
 *
 * 业务层 (src/ui + src/core) 与板上完全同一份代码 —— 这就是模拟器移植的意义。
 */

#define SDL_MAIN_HANDLED            /* 不用 SDL2main 的 main/WinMain 包装 */

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include <SDL2/SDL.h>               /* lv_drivers/sdl.h 不带 SDL2 头, 自己包含 */
#include "lvgl.h"
#include "sdl/sdl.h"                /* lv_drivers: SDL 窗口 + 鼠标 + 键盘 */

#include "common.h"
#include "core/config.h"
#include "core/app_manager.h"
#include "task/task_chat.h"         /* login_ack_t / chat_msg_t */
#include "task/task_weather.h"      /* weather_data_t */
#include "ui/ui.h"

/* 外部 lv_conf.h 开了 LV_TICK_CUSTOM, LVGL 要这里提供毫秒时钟 */
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
#ifdef _WIN32
    uint64_t now_ms = (uint64_t)GetTickCount64();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t now_ms = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
    if (start_ms == 0) start_ms = now_ms;
    return (uint32_t)(now_ms - start_ms);
}

/* ============ task 桩: 函数签名与真实 task_chat/task_weather 一致 ============ */

int chat_login(const char *acc, const char *pwd)
{
    (void)acc; (void)pwd;
    login_ack_t ack = { 1, "" };                    /* 模拟服务器: 密码正确 */
    app_manager_post(APP_MSG_LOGIN_ACK, &ack, sizeof(ack));
    return OK;
}

int chat_register(const char *acc, const char *pwd, const char *nick,
                  const char *ip, int port)
{
    (void)acc; (void)pwd; (void)nick; (void)ip; (void)port;
    login_ack_t ack = { 1, "" };                    /* 模拟服务器: 注册成功 */
    app_manager_post(APP_MSG_LOGIN_ACK, &ack, sizeof(ack));
    return OK;
}

int task_weather_refresh(void)
{
    weather_data_t w = { 0 };
    strcpy(w.city, "深圳");
    strcpy(w.weather, "多云");
    w.temp = 28; w.temp_min = 24; w.temp_max = 31;
    w.humidity = 62;
    strcpy(w.update_time, "12:00");
    app_manager_post(APP_MSG_WEATHER, &w, sizeof(w));
    return OK;
}

/* ============ 主入口 ============ */

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    /* 1. LVGL + SDL 窗口 (sdl_init 内部自建事件泵定时器) */
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s (WSL 需 WSLg 支持, 检查 DISPLAY)\n", SDL_GetError());
        return 1;
    }
    lv_init();
    sdl_init();

    /* 显示驱动注册: sdl_init 只建窗口, LVGL 的 disp driver 要自己挂 (lv_drivers v8.3 用法) */
    static lv_disp_draw_buf_t disp_buf;
    static lv_color_t disp_fb[SDL_HOR_RES * SDL_VER_RES];
    static lv_disp_drv_t disp_drv;
    lv_disp_draw_buf_init(&disp_buf, disp_fb, NULL, SDL_HOR_RES * SDL_VER_RES);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = SDL_HOR_RES;
    disp_drv.ver_res  = SDL_VER_RES;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = sdl_display_flush;
    lv_disp_drv_register(&disp_drv);

    /* 输入设备: 鼠标=触摸 / 键盘 / 滚轮=编码器 */
    lv_indev_drv_t indev;
    lv_indev_drv_init(&indev);
    indev.type    = LV_INDEV_TYPE_POINTER;
    indev.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&indev);

    lv_indev_drv_init(&indev);
    indev.type    = LV_INDEV_TYPE_KEYPAD;
    indev.read_cb = sdl_keyboard_read;
    lv_indev_drv_register(&indev);

    lv_indev_drv_init(&indev);
    indev.type    = LV_INDEV_TYPE_ENCODER;
    indev.read_cb = sdl_mousewheel_read;
    lv_indev_drv_register(&indev);

    /* 2. 框架初始化 (与板上 main.c 顺序一致) */
    config_init("carpad.cfg");
    app_manager_init();

    /* 3. 界面: 登录屏 + 系统屏 */
    ui_init();

    LOGI("carpad_sim started (SDL2 %dx%d)", SDL_HOR_RES, SDL_VER_RES);

    /* 4. 主循环: LVGL 渲染 + 后台消息分发
     * 注意: 外部 lv_conf.h 开了 LV_TICK_CUSTOM(custom_tick_get), 不需要 lv_tick_inc */
    while (1) {
        lv_timer_handler();
        SDL_Delay(5);
        app_manager_poll();
    }
    return 0;
}
