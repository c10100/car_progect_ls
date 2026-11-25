#include "ui/ui.h"
#include "ui/ui_internal.h"

/*
 * ui_video.c - 视频播放页
 *   视频画面 + 底部控制条 (播放/进度/倍速菜单/音量/最大化)
 *   最大化: 视频画面铺满整屏 1024x600 (父对象临时移到 SysScreen),
 *           点画面呼出控制条, 3 秒自动隐藏
 *   板上移植: 把 ui_VideoBox 换成 lv_img + 视频解码器帧回调即可
 */

lv_obj_t *ui_VideoBox;
lv_obj_t *ui_VideoCtrlBar;
lv_obj_t *ui_VideoPlaySym;
lv_obj_t *ui_VideoSlider;
lv_obj_t *ui_VideoTimeLabel;
lv_obj_t *ui_VideoSpeedBtnTxt;
lv_obj_t *ui_VideoSpeedMenu;
lv_obj_t *ui_VideoVolSlider;
lv_obj_t *ui_VideoVolVal;

#define VID_DUR  180          /* 演示视频总长 3 分钟 */
static int g_pos = 0, g_playing = 0, g_rate = 10, g_vol = 80, g_max = 0;
static lv_obj_t *g_cloak;
static lv_timer_t *g_tick, *g_autohide;

static void play_sync(void)
{
    lv_label_set_text(ui_VideoPlaySym,
                      g_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
}

static void time_sync(void)
{
    lv_label_set_text_fmt(ui_VideoTimeLabel, "%02d:%02d / %02d:%02d",
                          g_pos / 60, g_pos % 60, VID_DUR / 60, VID_DUR % 60);
    lv_slider_set_value(ui_VideoSlider, g_pos, LV_ANIM_OFF);
}

static void tick_cb(lv_timer_t *t)
{
    (void)t;
    if (!g_playing) return;
    g_pos += g_rate / 10;
    if (g_pos >= VID_DUR) {
        g_pos = VID_DUR;
        g_playing = 0;
        play_sync();
    }
    time_sync();
}

static void autohide_cb(lv_timer_t *t)
{
    (void)t;
    if (g_max) lv_obj_add_flag(ui_VideoCtrlBar, LV_OBJ_FLAG_HIDDEN);
    lv_timer_pause(g_autohide);
}

/* 最大化/还原: 画面铺满整屏 */
void ui_video_max(int on)
{
    if (on == g_max) return;
    g_max = on;
    if (on) {
        lv_obj_set_parent(ui_VideoBox, ui_SysScreen);
        lv_obj_set_parent(ui_VideoCtrlBar, ui_SysScreen);
        lv_obj_set_pos(ui_VideoBox, 0, 0);
        lv_obj_set_size(ui_VideoBox, UI_HOR_RES, UI_VER_RES);
        lv_obj_set_style_radius(ui_VideoBox, 0, 0);
        lv_obj_add_flag(ui_Topbar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_VideoCtrlBar, LV_OBJ_FLAG_HIDDEN);
        /* 隐藏 Dock (PageHost 之下的兄弟) */
        lv_obj_add_flag(ui_PageHost, LV_OBJ_FLAG_HIDDEN);
        lv_timer_reset(g_autohide);
        lv_timer_resume(g_autohide);
    } else {
        lv_obj_set_parent(ui_VideoBox, ui_Pages[UI_PAGE_VIDEO]);
        lv_obj_set_parent(ui_VideoCtrlBar, ui_Pages[UI_PAGE_VIDEO]);
        lv_obj_set_pos(ui_VideoBox, 18, 14);
        lv_obj_set_size(ui_VideoBox, 988, 400);
        lv_obj_set_style_radius(ui_VideoBox, 20, 0);
        lv_obj_clear_flag(ui_Topbar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_PageHost, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_VideoCtrlBar, LV_OBJ_FLAG_HIDDEN);
        lv_timer_pause(g_autohide);
    }
}

void ui_video_tap(void)
{
    if (!g_max) return;
    lv_obj_clear_flag(ui_VideoCtrlBar, LV_OBJ_FLAG_HIDDEN);
    lv_timer_reset(g_autohide);
    lv_timer_resume(g_autohide);
}

void ui_video_toggle(void)
{
    if (g_pos >= VID_DUR) g_pos = 0;
    g_playing = !g_playing;
    play_sync();
}

void ui_video_seek(int sec)
{
    if (sec < 0) sec = 0;
    if (sec > VID_DUR) sec = VID_DUR;
    g_pos = sec;
    time_sync();
}

void ui_video_speed_menu(int on)
{
    if (on) {
        lv_obj_move_foreground(g_cloak);
        lv_obj_clear_flag(g_cloak, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_VideoSpeedMenu, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ui_VideoSpeedMenu);
    } else {
        lv_obj_add_flag(g_cloak, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_VideoSpeedMenu, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_video_set_speed(int rate_x10)
{
    g_rate = rate_x10;
    lv_label_set_text_fmt(ui_VideoSpeedBtnTxt, "倍速 %d.%dx",
                          rate_x10 / 10, rate_x10 % 10);
    ui_video_speed_menu(0);
}

void ui_video_set_vol(int v)
{
    g_vol = v;
    lv_slider_set_value(ui_VideoVolSlider, v, LV_ANIM_OFF);
    lv_label_set_text_fmt(ui_VideoVolVal, "%d", v);
}

void ui_video_on_leave(void)
{
    ui_video_speed_menu(0);
    if (g_max) ui_video_max(0);
}

/* ---------- 创建 ---------- */
void ui_video_create(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 0, 0);

    /* 视频画面 */
    ui_VideoBox = ui_panel_create(parent, 988, 400);
    lv_obj_set_pos(ui_VideoBox, 18, 14);
    lv_obj_set_style_radius(ui_VideoBox, 20, 0);
    lv_obj_set_style_bg_color(ui_VideoBox, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_grad_color(ui_VideoBox, lv_color_hex(0x05070c), 0);
    lv_obj_set_style_bg_grad_dir(ui_VideoBox, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(ui_VideoBox, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ui_VideoBox, lv_color_hex(UI_COL_RULE), 0);
    lv_obj_t *poster = ui_label_create(ui_VideoBox, LV_SYMBOL_PLAY,
                                       &lv_font_montserrat_48, UI_COL_FAINT);
    lv_obj_center(poster);
    lv_obj_t *vname = ui_label_create(ui_VideoBox, "行车记录 · 周末山路.mp4",
                                      UI_FONT_S, UI_COL_FAINT);
    lv_obj_align(vname, LV_ALIGN_BOTTOM_LEFT, 16, -12);
    lv_obj_add_event_cb(ui_VideoBox, ui_event_video_tap, LV_EVENT_CLICKED, NULL);

    /* 倍速菜单 (先建在画面上层, 默认隐藏) */
    g_cloak = ui_cloak_create(parent);
    lv_obj_add_flag(g_cloak, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(g_cloak, ui_event_video_cloak, LV_EVENT_CLICKED, NULL);

    ui_VideoSpeedMenu = ui_panel_create(parent, 120, 168);
    lv_obj_align(ui_VideoSpeedMenu, LV_ALIGN_BOTTOM_RIGHT, -218, -70);
    lv_obj_set_style_bg_color(ui_VideoSpeedMenu, lv_color_hex(0x0e1622), 0);
    lv_obj_set_style_bg_opa(ui_VideoSpeedMenu, LV_OPA_90, 0);
    lv_obj_set_style_radius(ui_VideoSpeedMenu, 12, 0);
    lv_obj_set_style_border_color(ui_VideoSpeedMenu, lv_color_hex(UI_COL_RULE), 0);
    lv_obj_set_style_border_width(ui_VideoSpeedMenu, 1, 0);
    lv_obj_add_flag(ui_VideoSpeedMenu, LV_OBJ_FLAG_HIDDEN);
    static const int rates[4] = { 5, 10, 15, 20 };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *it = ui_btn_create(ui_VideoSpeedMenu, "", 0);
        lv_obj_set_size(it, 100, 32);
        lv_obj_align(it, LV_ALIGN_TOP_MID, 0, 6 + i * 38);
        lv_obj_t *lb = lv_obj_get_child(it, 0);
        lv_label_set_text_fmt(lb, "%d.%dx", rates[i] / 10, rates[i] % 10);
        lv_obj_set_user_data(it, (void *)(intptr_t)rates[i]);
        lv_obj_add_event_cb(it, ui_event_video_speed_pick, LV_EVENT_CLICKED, NULL);
    }

    /* 控制条 */
    ui_VideoCtrlBar = ui_panel_create(parent, 988, 52);
    lv_obj_align(ui_VideoCtrlBar, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_obj_set_style_bg_color(ui_VideoCtrlBar, lv_color_hex(UI_COL_CARD), 0);
    lv_obj_set_style_bg_opa(ui_VideoCtrlBar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ui_VideoCtrlBar, 14, 0);
    lv_obj_set_style_border_color(ui_VideoCtrlBar, lv_color_hex(UI_COL_RULE), 0);
    lv_obj_set_style_border_width(ui_VideoCtrlBar, 1, 0);

    lv_obj_t *vb = ui_icon_btn_create(ui_VideoCtrlBar, LV_SYMBOL_PLAY, 38);
    lv_obj_align(vb, LV_ALIGN_LEFT_MID, 8, 0);
    ui_VideoPlaySym = lv_obj_get_child(vb, 0);
    lv_obj_add_event_cb(vb, ui_event_video_toggle, LV_EVENT_CLICKED, NULL);

    ui_VideoTimeLabel = ui_label_create(ui_VideoCtrlBar, "00:00 / 03:00",
                                        UI_FONT_S, UI_COL_MUTED);
    lv_obj_align(ui_VideoTimeLabel, LV_ALIGN_LEFT_MID, 56, 0);

    ui_VideoSlider = lv_slider_create(ui_VideoCtrlBar);
    lv_obj_set_size(ui_VideoSlider, 400, 14);
    lv_obj_align(ui_VideoSlider, LV_ALIGN_LEFT_MID, 170, 0);
    lv_slider_set_range(ui_VideoSlider, 0, VID_DUR);
    lv_obj_set_style_bg_color(ui_VideoSlider, lv_color_hex(UI_COL_RULE), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_VideoSlider, lv_color_hex(UI_COL_ACCENT), LV_PART_INDICATOR);
    lv_obj_add_event_cb(ui_VideoSlider, ui_event_video_seek, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *spd = ui_btn_create(ui_VideoCtrlBar, "倍速 1.0x", 0);
    lv_obj_set_size(spd, 96, 34);
    lv_obj_align(spd, LV_ALIGN_RIGHT_MID, -258, 0);
    ui_VideoSpeedBtnTxt = lv_obj_get_child(spd, 0);
    lv_obj_add_event_cb(spd, ui_event_video_speed_menu, LV_EVENT_CLICKED, NULL);

    lv_obj_t *vsym = ui_label_create(ui_VideoCtrlBar, LV_SYMBOL_VOLUME_MAX,
                                     &lv_font_montserrat_16, UI_COL_MUTED);
    lv_obj_align(vsym, LV_ALIGN_RIGHT_MID, -204, 0);
    ui_VideoVolSlider = lv_slider_create(ui_VideoCtrlBar);
    lv_obj_set_size(ui_VideoVolSlider, 90, 12);
    lv_obj_align(ui_VideoVolSlider, LV_ALIGN_RIGHT_MID, -106, 0);
    lv_slider_set_range(ui_VideoVolSlider, 0, 100);
    lv_obj_set_style_bg_color(ui_VideoVolSlider, lv_color_hex(UI_COL_RULE), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_VideoVolSlider, lv_color_hex(UI_COL_ACCENT), LV_PART_INDICATOR);
    lv_obj_add_event_cb(ui_VideoVolSlider, ui_event_video_vol_change, LV_EVENT_VALUE_CHANGED, NULL);
    ui_VideoVolVal = ui_label_create(ui_VideoCtrlBar, "80", UI_FONT_S, UI_COL_MUTED);
    lv_obj_align(ui_VideoVolVal, LV_ALIGN_RIGHT_MID, -96, 0);

    lv_obj_t *mx = ui_btn_create(ui_VideoCtrlBar, "全屏", 0);
    lv_obj_set_size(mx, 64, 34);
    lv_obj_align(mx, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_add_event_cb(mx, ui_event_video_max, LV_EVENT_CLICKED, NULL);

    play_sync();
    time_sync();
    ui_video_set_vol(g_vol);
    g_tick = lv_timer_create(tick_cb, 1000, NULL);
    g_autohide = lv_timer_create(autohide_cb, 3000, NULL);
    lv_timer_pause(g_autohide);
}
