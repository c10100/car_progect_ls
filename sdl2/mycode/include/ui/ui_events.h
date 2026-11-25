#ifndef __UI_EVENTS_H__
#define __UI_EVENTS_H__

/*
 * ui_events.h - 事件回调声明 (仿 SquareLine 的 ui_events)
 *
 * 实现在 src/ui/ui_events.c —— 这是 UI 与框架之间唯一的胶水层:
 *   登录/注册 -> task_chat (TCP 自定义协议)
 *   天气刷新 -> task_weather (HTTP + cJSON)
 * 换一个工程移植时, 只重写 ui_events.c 的函数体即可。
 */

#include "lvgl.h"

/* ---- 登录屏 ---- */
void ui_event_auth_tab_login(lv_event_t *e);
void ui_event_auth_tab_reg(lv_event_t *e);
void ui_event_auth_do_login(lv_event_t *e);
void ui_event_auth_do_reg(lv_event_t *e);
/* 登录/注册 ACK 到达 (ui.c 的 APP_MSG_LOGIN_ACK 订阅回调转发进来) */
void ui_auth_on_ack(int ok, const char *tip);

/* ---- Dock ---- */
void ui_event_dock_home(lv_event_t *e);
void ui_event_dock_music(lv_event_t *e);
void ui_event_dock_video(lv_event_t *e);
void ui_event_dock_gallery(lv_event_t *e);
void ui_event_dock_games(lv_event_t *e);
void ui_event_dock_nav(lv_event_t *e);
void ui_event_dock_weather(lv_event_t *e);

/* ---- 主页宫格 ---- */
void ui_event_open_music(lv_event_t *e);
void ui_event_open_video(lv_event_t *e);
void ui_event_open_gallery(lv_event_t *e);
void ui_event_open_games(lv_event_t *e);
void ui_event_open_nav(lv_event_t *e);
void ui_event_open_weather(lv_event_t *e);

/* ---- 音乐页 ---- */
void ui_event_music_toggle(lv_event_t *e);      /* 播放/暂停   */
void ui_event_music_prev(lv_event_t *e);
void ui_event_music_next(lv_event_t *e);
void ui_event_music_seek(lv_event_t *e);        /* 进度条拖动  */
void ui_event_music_vol_toggle(lv_event_t *e);  /* 音量图标    */
void ui_event_music_vol_change(lv_event_t *e);
void ui_event_music_cloak(lv_event_t *e);       /* 点空白收起弹层 */

/* ---- 视频页 ---- */
void ui_event_video_toggle(lv_event_t *e);
void ui_event_video_seek(lv_event_t *e);
void ui_event_video_speed_menu(lv_event_t *e);  /* 倍速按钮 */
void ui_event_video_speed_pick(lv_event_t *e);  /* 选倍速(用户数据=ratex10) */
void ui_event_video_vol_change(lv_event_t *e);
void ui_event_video_max(lv_event_t *e);         /* 最大化/还原 */
void ui_event_video_tap(lv_event_t *e);         /* 最大化时点画面: 呼出控制条 */
void ui_event_video_cloak(lv_event_t *e);

/* ---- 相册页 ---- */
void ui_event_gallery_open(lv_event_t *e);      /* 点图块(用户数据=索引) */
void ui_event_gallery_prev(lv_event_t *e);
void ui_event_gallery_next(lv_event_t *e);
void ui_event_gallery_exit(lv_event_t *e);

/* ---- 游戏页 ---- */
void ui_event_game_open(lv_event_t *e);         /* 用户数据=游戏号 */
void ui_event_game_exit(lv_event_t *e);
void ui_event_2048_key(lv_event_t *e);          /* 方向按钮(用户数据=方向) */
void ui_event_2048_reset(lv_event_t *e);
void ui_event_ttt_click(lv_event_t *e);         /* 井字棋格子 */
void ui_event_ttt_reset(lv_event_t *e);
void ui_event_gomo_click(lv_event_t *e);        /* 五子棋格子 */
void ui_event_gomo_reset(lv_event_t *e);
void ui_event_snake_key(lv_event_t *e);         /* 贪吃蛇方向 */
void ui_event_snake_start(lv_event_t *e);

/* ---- 导航页 ---- */
void ui_event_nav_start(lv_event_t *e);         /* 开始导航(小车动画) */
void ui_event_nav_zoom(lv_event_t *e);          /* 用户数据=+1/-1 */

/* ---- 天气页 ---- */
void ui_event_weather_refresh(lv_event_t *e);

#endif /* __UI_EVENTS_H__ */
