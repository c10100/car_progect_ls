#include "ui/ui.h"

/*
 * ui_weather.c - 天气页 (数据来自 task_weather: HTTP + cJSON)
 *   当前天气大卡 + 逐小时条 + 未来 5 天
 *   数据更新入口: ui.c 的 ui_set_weather_full() (订阅 APP_MSG_WEATHER)
 *   逐小时/5天为演示占位, 可扩展协议解析后填入
 */

lv_obj_t *ui_WxCityLabel;
lv_obj_t *ui_WxTempLabel;
lv_obj_t *ui_WxDescLabel;
lv_obj_t *ui_WxExtraLabel;

void ui_weather_create(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 0, 0);

    /* ============ 当前天气大卡 ============ */
    lv_obj_t *cur = ui_card_create(parent, 614, 220);
    lv_obj_set_pos(cur, 18, 14);
    lv_obj_set_style_bg_grad_color(cur, lv_color_hex(UI_COL_BG2), 0);
    lv_obj_set_style_bg_grad_dir(cur, LV_GRAD_DIR_VER, 0);

    lv_obj_t *cap = ui_label_create(cur, "当前天气", UI_FONT_S, UI_COL_FAINT);
    lv_obj_align(cap, LV_ALIGN_TOP_LEFT, 18, 14);
    ui_WxCityLabel = ui_label_create(cur, "等待数据...", UI_FONT_M, UI_COL_INK);
    lv_obj_align(ui_WxCityLabel, LV_ALIGN_TOP_LEFT, 18, 38);
    ui_WxTempLabel = ui_label_create(cur, "--°", UI_FONT_XL, UI_COL_INK);
    lv_obj_align(ui_WxTempLabel, LV_ALIGN_LEFT_MID, 18, 10);

    lv_obj_t *right = ui_panel_create(cur, 240, 120);
    lv_obj_align(right, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
    ui_WxDescLabel = ui_label_create(right, "--", UI_FONT_M, UI_COL_MUTED);
    lv_obj_align(ui_WxDescLabel, LV_ALIGN_TOP_LEFT, 0, 0);
    ui_WxExtraLabel = ui_label_create(right, "湿度 --%   更新 --:--",
                                      UI_FONT_S, UI_COL_MUTED);
    lv_obj_align(ui_WxExtraLabel, LV_ALIGN_TOP_LEFT, 0, 30);
    lv_obj_t *hint = ui_label_create(right, "数据: OpenWeather API", UI_FONT_S, UI_COL_FAINT);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_t *rf = ui_btn_create(cur, "刷新", 0);
    lv_obj_align(rf, LV_ALIGN_BOTTOM_RIGHT, -14, -12);
    lv_obj_add_event_cb(rf, ui_event_weather_refresh, LV_EVENT_CLICKED, NULL);

    /* ============ 逐小时 (演示占位) ============ */
    const char *hours[4][2] = {
        { "现在", "--°" }, { "2时后", "--°" }, { "4时后", "--°" }, { "6时后", "--°" },
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *c = ui_card_create(parent, 85, 220);
        lv_obj_set_pos(c, 644 + i * 97, 14);
        lv_obj_t *h = ui_label_create(c, hours[i][0], UI_FONT_S, UI_COL_FAINT);
        lv_obj_align(h, LV_ALIGN_TOP_MID, 0, 18);
        lv_obj_t *t = ui_label_create(c, hours[i][1], UI_FONT_NUM, UI_COL_INK);
        lv_obj_align(t, LV_ALIGN_CENTER, 0, 0);
    }

    /* ============ 未来 5 天 (演示占位) ============ */
    const char *days[5] = { "明天", "后天", "周三", "周四", "周五" };
    for (int i = 0; i < 5; i++) {
        lv_obj_t *c = ui_card_create(parent, 184, 190);
        lv_obj_set_pos(c, 18 + i * 196, 246);
        lv_obj_t *d = ui_label_create(c, days[i], UI_FONT_S, UI_COL_FAINT);
        lv_obj_align(d, LV_ALIGN_TOP_MID, 0, 16);
        lv_obj_t *ic = ui_label_create(c, LV_SYMBOL_REFRESH, &lv_font_montserrat_28,
                                       i % 2 ? UI_COL_ACCENT : UI_COL_AMBER);
        lv_obj_align(ic, LV_ALIGN_TOP_MID, 0, 60);
        lv_obj_t *rg = ui_label_create(c, "18° / 26°", UI_FONT_S, UI_COL_MUTED);
        lv_obj_align(rg, LV_ALIGN_BOTTOM_MID, 0, -20);
    }
}
