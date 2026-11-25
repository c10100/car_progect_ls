#ifndef __UI_H__
#define __UI_H__

/*
 * ui.h - CARPAD OS 界面总头文件 (仿 SquareLine Studio 导出结构)
 *
 * 结构对照 SquareLine 的导出:
 *   ui.h / ui.c          -> 屏幕与控件创建, ui_init() 总入口   (本文件 + src/ui/ui.c)
 *   ui_helpers.h/.c      -> 设计系统/控件工厂                  (可整体移植)
 *   ui_events.h/.c       -> 全部事件回调 (业务胶水, 换工程只改这里)
 *   ui_auth.c ui_home.c ui_music.c ... -> 每个页面一个文件, 均只做"创建+刷新"
 *
 * 移植到其它 LVGL v8 工程: 拷走 src/ui 与 include/ui 下全部文件, 满足
 *   1) lv_conf.h 打开 LV_FONT_SIMSUN_16_CJK 与 montserrat 12/28/48;
 *   2) 屏幕分辨率改 ui_helpers.h 的 UI_HOR_RES/UI_VER_RES;
 *   3) ui_events.c 里的框架调用换成新工程的业务接口。
 *
 * 线程原则: 除 ui_events/app_manager 回调外, 后台线程不得调用 lv_xxx,
 * 数据更新一律走 app_manager_post() -> ui.c 里注册的订阅回调。
 */

#include "lvgl.h"
#include "ui/ui_helpers.h"
#include "ui/ui_events.h"

/* ===================== 页面枚举 (Dock 顺序) ===================== */
typedef enum {
    UI_PAGE_HOME = 0,   /* 主页   */
    UI_PAGE_MUSIC,      /* 音乐   */
    UI_PAGE_VIDEO,      /* 视频   */
    UI_PAGE_GALLERY,    /* 相册   */
    UI_PAGE_GAMES,      /* 游戏   */
    UI_PAGE_NAV,        /* 导航   */
    UI_PAGE_WEATHER,    /* 天气   */
    UI_PAGE_NUM
} ui_page_e;

/* ===================== 全局控件 (跨文件引用的都放这) ===================== */
extern lv_obj_t *ui_AuthScreen;          /* 登录/注册屏 */
extern lv_obj_t *ui_SysScreen;           /* 系统主屏    */

/* 顶栏 */
extern lv_obj_t *ui_Topbar;
extern lv_obj_t *ui_TopNetLabel;
extern lv_obj_t *ui_TopWeatherLabel;
extern lv_obj_t *ui_TopSpeedLabel;
extern lv_obj_t *ui_TopClockLabel;

/* 页面容器与 Dock */
extern lv_obj_t *ui_PageHost;
extern lv_obj_t *ui_Pages[UI_PAGE_NUM];
extern lv_obj_t *ui_DockBtns[UI_PAGE_NUM];

/* 登录屏 (auth 回调需要) */
extern lv_obj_t *ui_LgAccountArea;
extern lv_obj_t *ui_LgPassArea;
extern lv_obj_t *ui_RgAccountArea;
extern lv_obj_t *ui_RgPassArea;
extern lv_obj_t *ui_RgNickArea;
extern lv_obj_t *ui_AuthMsgLabel;        /* 错误/提示行 */
extern lv_obj_t *ui_AuthTabLoginBtn;     /* 两个 tab    */
extern lv_obj_t *ui_AuthTabRegBtn;
extern lv_obj_t *ui_AuthLoginForm;       /* 登录表单容器 */
extern lv_obj_t *ui_AuthRegForm;         /* 注册表单容器 */
extern lv_obj_t *ui_OfflineSwitch;       /* 离线演示开关 */

/* 主页 */
extern lv_obj_t *ui_HomeClockLabel;
extern lv_obj_t *ui_HomeDateLabel;
extern lv_obj_t *ui_HomeWxTempLabel;
extern lv_obj_t *ui_HomeWxDescLabel;

/* 音乐页 */
extern lv_obj_t *ui_MusicCover;
extern lv_obj_t *ui_MusicTitleLabel;
extern lv_obj_t *ui_MusicArtistLabel;
extern lv_obj_t *ui_MusicSmallTitle;     /* 播放条左侧小字 */
extern lv_obj_t *ui_MusicSlider;
extern lv_obj_t *ui_MusicTimeCur;
extern lv_obj_t *ui_MusicTimeAll;
extern lv_obj_t *ui_MusicPlaySym;        /* 播放键上的符号label */
extern lv_obj_t *ui_MusicVolPop;         /* 音量弹层 */
extern lv_obj_t *ui_MusicVolSlider;
extern lv_obj_t *ui_MusicVolVal;

/* 视频页 */
extern lv_obj_t *ui_VideoBox;
extern lv_obj_t *ui_VideoCtrlBar;
extern lv_obj_t *ui_VideoPlaySym;
extern lv_obj_t *ui_VideoSlider;
extern lv_obj_t *ui_VideoTimeLabel;
extern lv_obj_t *ui_VideoSpeedBtnTxt;
extern lv_obj_t *ui_VideoSpeedMenu;
extern lv_obj_t *ui_VideoVolSlider;
extern lv_obj_t *ui_VideoVolVal;

/* 相册页 */
extern lv_obj_t *ui_GalleryViewer;       /* 全屏查看器 */

/* 天气页 */
extern lv_obj_t *ui_WxCityLabel;
extern lv_obj_t *ui_WxTempLabel;
extern lv_obj_t *ui_WxDescLabel;
extern lv_obj_t *ui_WxExtraLabel;

/* 导航页 */
extern lv_obj_t *ui_NavDistLabel;
extern lv_obj_t *ui_NavEtaLabel;

/* ===================== API ===================== */
void ui_init(void);                      /* main 里调用一次 */
void ui_show_page(ui_page_e page);       /* Dock 切页 */
void ui_enter_system(void);              /* 登录成功 -> 系统屏 */
void ui_back_to_auth(void);

/* 顶栏/主页数据接口 (app_manager 订阅回调里调用) */
void ui_set_net_text(const char *txt);
void ui_set_weather_brief(const char *txt);   /* 顶栏一行简况 */
void ui_set_weather_full(const char *city, const char *desc,
                         int temp, int humi, const char *uptime);

/* ===================== 各页面创建函数 (ui.c 依次调用) ===================== */
void ui_auth_create(void);
void ui_sys_create(void);
void ui_home_create(lv_obj_t *parent);
void ui_music_create(lv_obj_t *parent);
void ui_video_create(lv_obj_t *parent);
void ui_gallery_create(lv_obj_t *parent);
void ui_games_create(lv_obj_t *parent);
void ui_nav_create(lv_obj_t *parent);
void ui_weather_create(lv_obj_t *parent);

/* 页面离开时的清理钩子 (ui_show_page 调): 关弹层/退最大化 */
void ui_music_on_leave(void);
void ui_video_on_leave(void);

#endif /* __UI_H__ */
