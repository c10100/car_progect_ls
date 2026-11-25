#include "common.h"
#include "ui/ui.h"
#include "core/app_manager.h"
#include "task/task_weather.h"
#include "task/task_chat.h"

/*
 * ui.c - 系统主屏: 顶栏 + 页面容器 + Dock, 以及全部数据入口
 *
 * 数据流 (与框架对齐):
 *   task_weather 线程 --post(APP_MSG_WEATHER)--> ui_weather_msg()
 *   task_chat    线程 --post(APP_MSG_LOGIN_ACK)--> ui_events/auth 处理
 *   时钟/车速      -> lv_timer 模拟 (TODO 车速接 CAN)
 */

lv_obj_t *ui_SysScreen;
lv_obj_t *ui_Topbar;
lv_obj_t *ui_TopNetLabel;
lv_obj_t *ui_TopWeatherLabel;
lv_obj_t *ui_TopSpeedLabel;
lv_obj_t *ui_TopClockLabel;
lv_obj_t *ui_PageHost;
lv_obj_t *ui_Pages[UI_PAGE_NUM];
lv_obj_t *ui_DockBtns[UI_PAGE_NUM];

static int g_cur_page = UI_PAGE_HOME;

/* ---------------- Dock ---------------- */
static void dock_btn(lv_obj_t *parent, int idx, const char *sym, const char *txt)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, 86, 48);
    lv_obj_set_style_radius(b, 12, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(UI_COL_CARD2), LV_STATE_CHECKED);

    lv_obj_t *ic = ui_label_create(b, sym, &lv_font_montserrat_16, UI_COL_FAINT);
    lv_obj_set_style_text_color(ic, lv_color_hex(UI_COL_ACCENT), LV_STATE_CHECKED);
    lv_obj_align(ic, LV_ALIGN_TOP_MID, 0, 3);
    lv_obj_t *lb = ui_label_create(b, txt, UI_FONT_S, UI_COL_FAINT);
    lv_obj_set_style_text_color(lb, lv_color_hex(UI_COL_ACCENT), LV_STATE_CHECKED);
    lv_obj_align(lb, LV_ALIGN_BOTTOM_MID, 0, 7);

    ui_DockBtns[idx] = b;
}

/* ---------------- 顶栏 ---------------- */
static void topbar_create(lv_obj_t *parent)
{
    ui_Topbar = ui_panel_create(parent, UI_HOR_RES, UI_TOPBAR_H);
    lv_obj_set_style_bg_color(ui_Topbar, lv_color_hex(0x0a1019), 0);
    lv_obj_set_style_bg_opa(ui_Topbar, LV_OPA_90, 0);
    lv_obj_set_style_border_width(ui_Topbar, 0, 0);
    lv_obj_set_style_border_side(ui_Topbar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(ui_Topbar, lv_color_hex(UI_COL_RULE), 0);

    lv_obj_t *dot = lv_obj_create(ui_Topbar);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(UI_COL_GREEN), 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_align(dot, LV_ALIGN_LEFT_MID, 18, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);

    ui_TopNetLabel = ui_label_create(ui_Topbar, "网络: --", UI_FONT_S, UI_COL_MUTED);
    lv_obj_align(ui_TopNetLabel, LV_ALIGN_LEFT_MID, 34, 0);

    ui_TopWeatherLabel = ui_label_create(ui_Topbar, "天气 --", UI_FONT_S, UI_COL_MUTED);
    lv_obj_align(ui_TopWeatherLabel, LV_ALIGN_LEFT_MID, 130, 0);

    ui_TopSpeedLabel = ui_label_create(ui_Topbar, "-- km/h", UI_FONT_S, UI_COL_ACCENT);
    lv_obj_align(ui_TopSpeedLabel, LV_ALIGN_RIGHT_MID, -80, 0);

    ui_TopClockLabel = ui_label_create(ui_Topbar, "--:--", UI_FONT_M, UI_COL_INK);
    lv_obj_align(ui_TopClockLabel, LV_ALIGN_RIGHT_MID, -18, 0);
}

/* ---------------- 定时器: 时钟 / 车速 ---------------- */
static void clock_timer_cb(lv_timer_t *t)
{
    (void)t;
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);

    lv_label_set_text_fmt(ui_TopClockLabel, "%02d:%02d", tm.tm_hour, tm.tm_min);
    lv_label_set_text_fmt(ui_HomeClockLabel, "%02d:%02d", tm.tm_hour, tm.tm_min);
    lv_label_set_text_fmt(ui_HomeDateLabel, "%d月%d日 星期%d",
                          tm.tm_mon + 1, tm.tm_mday, (tm.tm_wday + 6) % 7 + 1);
}

static void speed_timer_cb(lv_timer_t *t)
{
    (void)t;
    static int speed = 45;
    speed += (rand() % 7) - 3;
    if (speed < 0)   speed = 0;
    if (speed > 120) speed = 120;
    lv_label_set_text_fmt(ui_TopSpeedLabel, "%d km/h", speed);
}

/* ---------------- 订阅回调: 天气 ---------------- */
static void ui_weather_msg(app_msg_t *msg)
{
    weather_data_t *w = (weather_data_t *)msg->data;
    char brief[64];
    snprintf(brief, sizeof(brief), "%s %s %d°C", w->city, w->weather, w->temp);
    ui_set_weather_full(w->city, w->weather, w->temp, w->humidity, w->update_time);
    ui_set_weather_brief(brief);
}

/* ---------------- 订阅回调: 登录/注册 ACK ---------------- */
static void ui_login_ack_msg(app_msg_t *msg)
{
    login_ack_t *ack = (login_ack_t *)msg->data;
    ui_auth_on_ack(ack ? ack->ok : 0, ack ? ack->tip : NULL);
}

/* ---------------- 页面切换 ---------------- */
static void dock_sync(void)
{
    for (int i = 0; i < UI_PAGE_NUM; i++) {
        if (i == g_cur_page) lv_obj_add_state(ui_DockBtns[i], LV_STATE_CHECKED);
        else                 lv_obj_clear_state(ui_DockBtns[i], LV_STATE_CHECKED);
    }
}

void ui_show_page(ui_page_e page)
{
    if (page < 0 || page >= UI_PAGE_NUM) return;

    /* 离开旧页: 收弹层/退最大化 (对应原型 go() 的处理) */
    ui_music_on_leave();
    ui_video_on_leave();

    for (int i = 0; i < UI_PAGE_NUM; i++) {
        if (i == (int)page) lv_obj_clear_flag(ui_Pages[i], LV_OBJ_FLAG_HIDDEN);
        else                lv_obj_add_flag(ui_Pages[i], LV_OBJ_FLAG_HIDDEN);
    }
    g_cur_page = page;
    dock_sync();
}

void ui_set_net_text(const char *txt)
{
    if (ui_TopNetLabel && txt) lv_label_set_text(ui_TopNetLabel, txt);
}

void ui_set_weather_brief(const char *txt)
{
    if (ui_TopWeatherLabel && txt) lv_label_set_text(ui_TopWeatherLabel, txt);
}

void ui_set_weather_full(const char *city, const char *desc,
                         int temp, int humi, const char *uptime)
{
    if (ui_WxCityLabel)  lv_label_set_text(ui_WxCityLabel, city);
    if (ui_WxDescLabel)  lv_label_set_text(ui_WxDescLabel, desc);
    if (ui_WxExtraLabel) lv_label_set_text_fmt(ui_WxExtraLabel, "湿度 %d%%   更新 %s", humi, uptime);
    if (ui_WxTempLabel)  lv_label_set_text_fmt(ui_WxTempLabel, "%d°", temp);
    if (ui_HomeWxTempLabel) lv_label_set_text_fmt(ui_HomeWxTempLabel, "%d°", temp);
    if (ui_HomeWxDescLabel) lv_label_set_text_fmt(ui_HomeWxDescLabel, "%s · %s", city, desc);
}

/* ---------------- 进入/返回 ---------------- */
void ui_enter_system(void)
{
    lv_scr_load(ui_SysScreen);
}

void ui_back_to_auth(void)
{
    lv_scr_load(ui_AuthScreen);
}

/* ---------------- 系统屏创建 ---------------- */
void ui_sys_create(void)
{
    ui_SysScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_SysScreen, lv_color_hex(UI_COL_BG), 0);
    lv_obj_clear_flag(ui_SysScreen, LV_OBJ_FLAG_SCROLLABLE);

    /* 页面容器: 顶栏之下、Dock 之上 */
    ui_PageHost = ui_panel_create(ui_SysScreen, UI_HOR_RES,
                                  UI_VER_RES - UI_TOPBAR_H - UI_DOCK_H);
    lv_obj_set_pos(ui_PageHost, 0, UI_TOPBAR_H);
    lv_obj_set_style_pad_all(ui_PageHost, 0, 0);

    /* 7 个页面: 不显示的先隐藏 */
    const struct { void (*create)(lv_obj_t *); } makers[UI_PAGE_NUM] = {
        { ui_home_create }, { ui_music_create }, { ui_video_create },
        { ui_gallery_create }, { ui_games_create }, { ui_nav_create },
        { ui_weather_create },
    };
    for (int i = 0; i < UI_PAGE_NUM; i++) {
        ui_Pages[i] = ui_panel_create(ui_PageHost, LV_PCT(100), LV_PCT(100));
        makers[i].create(ui_Pages[i]);
        if (i != UI_PAGE_HOME) lv_obj_add_flag(ui_Pages[i], LV_OBJ_FLAG_HIDDEN);
    }

    topbar_create(ui_SysScreen);

    /* Dock */
    lv_obj_t *dock = ui_panel_create(ui_SysScreen, UI_HOR_RES, UI_DOCK_H);
    lv_obj_align(dock, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(dock, lv_color_hex(0x0a1019), 0);
    lv_obj_set_style_bg_opa(dock, LV_OPA_90, 0);
    lv_obj_set_style_border_width(dock, 0, 0);
    lv_obj_set_style_border_side(dock, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(dock, lv_color_hex(UI_COL_RULE), 0);
    lv_obj_set_flex_flow(dock, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dock, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dock, 6, 0);

    static const lv_event_cb_t dock_cbs[UI_PAGE_NUM] = {
        ui_event_dock_home, ui_event_dock_music, ui_event_dock_video,
        ui_event_dock_gallery, ui_event_dock_games, ui_event_dock_nav,
        ui_event_dock_weather,
    };
    static const char *dock_syms[UI_PAGE_NUM] = {
        LV_SYMBOL_HOME, LV_SYMBOL_AUDIO, LV_SYMBOL_VIDEO,
        LV_SYMBOL_IMAGE, LV_SYMBOL_BARS, LV_SYMBOL_GPS, LV_SYMBOL_REFRESH,
    };
    static const char *dock_txts[UI_PAGE_NUM] = {
        "主页", "音乐", "视频", "相册", "游戏", "导航", "天气",
    };
    for (int i = 0; i < UI_PAGE_NUM; i++) {
        dock_btn(dock, i, dock_syms[i], dock_txts[i]);
        lv_obj_add_event_cb(ui_DockBtns[i], dock_cbs[i], LV_EVENT_CLICKED, NULL);
    }
    dock_sync();

    /* 定时器 + 订阅 */
    lv_timer_create(clock_timer_cb, 1000, NULL);
    lv_timer_create(speed_timer_cb, 2000, NULL);
    clock_timer_cb(NULL);                       /* 立即刷一次, 避免开局显示 --:-- */
    app_manager_subscribe(APP_MSG_WEATHER, ui_weather_msg);
    app_manager_subscribe(APP_MSG_LOGIN_ACK, ui_login_ack_msg);
}

/* ---------------- 总入口 ---------------- */
void ui_init(void)
{
    ui_auth_create();      /* 先建登录屏 */
    ui_sys_create();       /* 再建系统屏 (隐藏页不占渲染) */
    lv_scr_load(ui_AuthScreen);
}
