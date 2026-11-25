#ifndef __UI_INTERNAL_H__
#define __UI_INTERNAL_H__

/*
 * ui_internal.h - 页面内部动作接口 (仅 ui_events.c 使用)
 *
 * 页面文件 (ui_music.c 等) 保存各自的播放状态/棋盘数据,
 * ui_events.c 只做"事件 -> 动作"翻译, 不直接碰页面内部静态变量。
 */

#include "lvgl.h"

/* ---- 音乐页 ---- */
void ui_music_toggle(void);            /* 播放/暂停       */
void ui_music_prev(void);
void ui_music_next(void);
void ui_music_seek(int sec);           /* 拖进度条        */
void ui_music_vol_pop(int on);         /* 音量弹层开关    */
void ui_music_set_vol(int v);          /* 0~100           */

/* ---- 视频页 ---- */
void ui_video_toggle(void);
void ui_video_seek(int sec);           /* 拖进度条        */
void ui_video_speed_menu(int on);      /* 倍速菜单开关    */
void ui_video_set_speed(int rate_x10); /* 10=1.0x 20=2.0x */
void ui_video_set_vol(int v);
void ui_video_max(int on);             /* 最大化/还原     */
void ui_video_tap(void);               /* 最大化时点画面: 呼出控制条 */

/* ---- 相册页 ---- */
void ui_gallery_open(int idx);
void ui_gallery_nav(int step);         /* +1 下一张 / -1 上一张 */
void ui_gallery_exit(void);

/* ---- 游戏页 ---- */
enum { UI_GAME_2048 = 0, UI_GAME_TTT, UI_GAME_GOMO, UI_GAME_SNAKE, UI_GAME_NUM };
void ui_games_open(int game);
void ui_games_exit(void);
void ui_2048_move(int dir);            /* 0上 1下 2左 3右 */
void ui_2048_reset(void);
void ui_ttt_click(int cell);           /* 0~8             */
void ui_ttt_reset(void);
void ui_gomo_click(int cell);          /* 0~80 (9x9)      */
void ui_gomo_reset(void);
void ui_snake_dir(int dir);            /* 0上 1下 2左 3右 */
void ui_snake_start(void);             /* 开始/暂停       */

/* ---- 导航页 ---- */
void ui_nav_start(void);               /* 小车沿线动画    */
void ui_nav_zoom(int step);            /* +1 放大 / -1 缩小 */

#endif /* __UI_INTERNAL_H__ */
