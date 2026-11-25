#include "ui/ui.h"

/*
 * ui_home.c - 主页: 时钟/天气/快捷状态 三张 hero 卡 + 3x2 应用宫格
 * (对应原型 .hero + .grid, 1024x600 下无滚动一屏放下)
 */

lv_obj_t *ui_HomeClockLabel;
lv_obj_t *ui_HomeDateLabel;
lv_obj_t *ui_HomeWxTempLabel;
lv_obj_t *ui_HomeWxDescLabel;

/* 应用宫格卡片 */
static lv_obj_t *app_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                          lv_coord_t w, lv_coord_t h,
                          const char *sym, const char *name,
                          const char *en, lv_event_cb_t cb, uint32_t glow)
{
    lv_obj_t *c = lv_btn_create(parent);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, w, h);
    lv_obj_set_style_radius(c, 20, 0);
    lv_obj_set_style_shadow_width(c, 0, 0);
    lv_obj_set_style_bg_color(c, lv_color_hex(UI_COL_CARD), 0);
    lv_obj_set_style_bg_color(c, lv_color_hex(UI_COL_CARD2), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(c, lv_color_hex(UI_COL_RULE), 0);
    lv_obj_set_style_border_color(c, lv_color_hex(glow), LV_STATE_FOCUSED);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ic = ui_label_create(c, sym, &lv_font_montserrat_28, glow);
    lv_obj_align(ic, LV_ALIGN_TOP_LEFT, 18, 16);
    lv_obj_t *nm = ui_label_create(c, name, UI_FONT_M, UI_COL_INK);
    lv_obj_align(nm, LV_ALIGN_BOTTOM_LEFT, 18, -34);
    lv_obj_t *eln = ui_label_create(c, en, UI_FONT_S, UI_COL_FAINT);
    lv_obj_align(eln, LV_ALIGN_BOTTOM_LEFT, 18, -14);

    /* 右上角光晕点 (对应 .app::after) */
    lv_obj_t *halo = lv_obj_create(c);
    lv_obj_set_size(halo, 46, 46);
    lv_obj_set_style_radius(halo, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(halo, lv_color_hex(glow), 0);
    lv_obj_set_style_bg_opa(halo, LV_OPA_30, 0);
    lv_obj_set_style_border_width(halo, 0, 0);
    lv_obj_align(halo, LV_ALIGN_TOP_RIGHT, -6, 6);
    lv_obj_clear_flag(halo, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(c, cb, LV_EVENT_CLICKED, NULL);
    return c;
}

void ui_home_create(lv_obj_t *parent)
{
    /* 页面内边距 14/18, 内容区 988x466 */
    lv_obj_set_style_pad_left(parent, 18, 0);
    lv_obj_set_style_pad_right(parent, 18, 0);
    lv_obj_set_style_pad_top(parent, 14, 0);

    /* ============ hero 三卡 ============ */
    /* 时钟 */
    lv_obj_t *clock_card = ui_card_create(parent, 380, 150);
    lv_obj_set_pos(clock_card, 0, 0);
    ui_HomeClockLabel = ui_label_create(clock_card, "--:--", UI_FONT_XL, UI_COL_INK);
    lv_obj_align(ui_HomeClockLabel, LV_ALIGN_TOP_LEFT, 18, 26);
    ui_HomeDateLabel = ui_label_create(clock_card, "--月--日", UI_FONT_M, UI_COL_MUTED);
    lv_obj_align(ui_HomeDateLabel, LV_ALIGN_BOTTOM_LEFT, 20, -22);

    /* 天气简况 */
    lv_obj_t *wx_card = ui_card_create(parent, 275, 150);
    lv_obj_set_pos(wx_card, 392, 0);
    lv_obj_t *wxi = ui_label_create(wx_card, LV_SYMBOL_SETTINGS, &lv_font_montserrat_28, UI_COL_AMBER);
    lv_obj_align(wxi, LV_ALIGN_TOP_LEFT, 18, 16);
    ui_HomeWxTempLabel = ui_label_create(wx_card, "--°", UI_FONT_NUM, UI_COL_INK);
    lv_obj_align(ui_HomeWxTempLabel, LV_ALIGN_BOTTOM_LEFT, 18, -30);
    ui_HomeWxDescLabel = ui_label_create(wx_card, "等待天气数据...", UI_FONT_S, UI_COL_MUTED);
    lv_obj_align(ui_HomeWxDescLabel, LV_ALIGN_BOTTOM_LEFT, 18, -8);

    /* 快捷状态 */
    lv_obj_t *qk = ui_card_create(parent, 297, 150);
    lv_obj_set_pos(qk, 691, 0);
    const char *rows[][2] = {
        { LV_SYMBOL_BLUETOOTH, "蓝牙  已连接 · 小米15" },
        { LV_SYMBOL_AUDIO,     "媒体  正在播放 · 车载FM" },
        { LV_SYMBOL_WIFI,      "网络  4G 在线" },
        { LV_SYMBOL_BATTERY_FULL, "电量  82% · 市电已接" },
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *ic = ui_label_create(qk, rows[i][0], &lv_font_montserrat_14, UI_COL_ACCENT);
        lv_obj_align(ic, LV_ALIGN_TOP_LEFT, 18, 16 + i * 31);
        lv_obj_t *tx = ui_label_create(qk, rows[i][1], UI_FONT_S, UI_COL_MUTED);
        lv_obj_align(tx, LV_ALIGN_TOP_LEFT, 42, 17 + i * 31);
    }

    /* ============ 3x2 应用宫格 ============ */
    const struct {
        const char *sym, *name, *en; lv_event_cb_t cb; uint32_t glow;
    } apps[6] = {
        { LV_SYMBOL_AUDIO,  "音乐", "MUSIC",   ui_event_open_music,   UI_COL_ACCENT },
        { LV_SYMBOL_VIDEO,  "视频", "VIDEO",   ui_event_open_video,   UI_COL_ACC2   },
        { LV_SYMBOL_IMAGE,  "相册", "GALLERY", ui_event_open_gallery, UI_COL_AMBER  },
        { LV_SYMBOL_BARS,   "游戏", "GAMES",   ui_event_open_games,   UI_COL_GREEN  },
        { LV_SYMBOL_GPS,    "导航", "NAV",     ui_event_open_nav,     UI_COL_RED    },
        { LV_SYMBOL_REFRESH,"天气", "WEATHER", ui_event_open_weather, UI_COL_ACCENT },
    };
    const int cw = 321, ch = 146, gap = 12;
    for (int i = 0; i < 6; i++) {
        app_card(parent, (i % 3) * (cw + gap), 162 + (i / 3) * (ch + gap),
                 cw, ch, apps[i].sym, apps[i].name, apps[i].en,
                 apps[i].cb, apps[i].glow);
    }
}
