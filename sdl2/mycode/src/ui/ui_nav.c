#include "common.h"
#include "ui/ui.h"
#include "ui/ui_internal.h"

/*
 * ui_nav.c - 导航页 (演示)
 *   左: 地图 (路网 + 路线 + 小车动画 + 缩放)
 *   右: 终点卡 / 路况步骤 / ETA / 开始导航
 *   板上移植: 地图换瓦片图 lv_img, 路线仍用 lv_line 叠加
 */

lv_obj_t *ui_NavDistLabel;
lv_obj_t *ui_NavEtaLabel;

#define NAV_PTS 8
/* 地图坐标系的原始路线 (600x438 内) */
static const lv_point_t ROUTE0[NAV_PTS] = {
    { 40, 380 }, { 150, 380 }, { 150, 240 }, { 300, 240 },
    { 300, 120 }, { 460, 120 }, { 460, 60 }, { 560, 60 },
};
static lv_point_t g_scaled[NAV_PTS];
static float g_zoomf = 1.0f;

static lv_obj_t *g_line, *g_line_ghost, *g_car;
static int g_car_idx = 0;
static lv_timer_t *g_drive;

static void route_rescale(void)
{
    const float cx = 300, cy = 219;
    for (int i = 0; i < NAV_PTS; i++) {
        g_scaled[i].x = (lv_coord_t)(cx + (ROUTE0[i].x - cx) * g_zoomf);
        g_scaled[i].y = (lv_coord_t)(cy + (ROUTE0[i].y - cy) * g_zoomf);
    }
    if (g_line)     lv_line_set_points(g_line, g_scaled, NAV_PTS);
    if (g_line_ghost) lv_line_set_points(g_line_ghost, g_scaled, NAV_PTS);
    if (g_car) {
        lv_obj_set_pos(g_car, g_scaled[g_car_idx].x - 7, g_scaled[g_car_idx].y - 7);
    }
}

static void drive_cb(lv_timer_t *t)
{
    (void)t;
    if (g_car_idx < NAV_PTS - 1) {
        g_car_idx++;
        lv_obj_set_pos(g_car, g_scaled[g_car_idx].x - 7, g_scaled[g_car_idx].y - 7);
        int total = 12400;                       /* 12.4 km 演示 */
        int left = total - total * g_car_idx / (NAV_PTS - 1);
        lv_label_set_text_fmt(ui_NavDistLabel, "剩余 %d.%d km",
                              left / 1000, (left % 1000) / 100);
        lv_label_set_text_fmt(ui_NavEtaLabel, "%d 分钟", 3 + (NAV_PTS - 1 - g_car_idx) * 2);
    } else {
        lv_label_set_text(ui_NavEtaLabel, "已到达");
        lv_timer_pause(g_drive);
    }
}

void ui_nav_start(void)
{
    g_car_idx = 0;
    lv_obj_set_pos(g_car, g_scaled[0].x - 7, g_scaled[0].y - 7);
    lv_timer_resume(g_drive);
    lv_timer_reset(g_drive);
}

void ui_nav_zoom(int step)
{
    g_zoomf += step * 0.2f;
    if (g_zoomf < 0.6f) g_zoomf = 0.6f;
    if (g_zoomf > 1.8f) g_zoomf = 1.8f;
    route_rescale();
}

/* 地图元素: 一条横/竖马路 */
static lv_obj_t *road(lv_obj_t *map, lv_coord_t x, lv_coord_t y,
                      lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *r = lv_obj_create(map);
    lv_obj_set_pos(r, x, y);
    lv_obj_set_size(r, w, h);
    lv_obj_set_style_bg_color(r, lv_color_hex(0x1a2636), 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(r, 0, 0);
    lv_obj_set_style_radius(r, 2, 0);
    lv_obj_clear_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    return r;
}

void ui_nav_create(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 0, 0);

    /* ============ 左: 地图 ============ */
    lv_obj_t *map = ui_panel_create(parent, 610, 466);
    lv_obj_set_pos(map, 18, 14);
    lv_obj_set_style_radius(map, 20, 0);
    lv_obj_set_style_bg_color(map, lv_color_hex(0x0b1220), 0);
    lv_obj_set_style_bg_opa(map, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(map, lv_color_hex(UI_COL_RULE), 0);
    lv_obj_set_style_border_width(map, 1, 0);

    /* 路网 (装饰) */
    road(map, 0, 100, 610, 14);
    road(map, 0, 260, 610, 14);
    road(map, 100, 0, 14, 466);
    road(map, 360, 0, 14, 466);
    road(map, 500, 0, 14, 466);

    /* 备选路线 (灰) + 推荐路线 (青) */
    static lv_style_t style_line, style_ghost;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 6);
    lv_style_set_line_color(&style_line, lv_color_hex(UI_COL_ACCENT));
    lv_style_set_line_rounded(&style_line, true);
    lv_style_init(&style_ghost);
    lv_style_set_line_width(&style_ghost, 3);
    lv_style_set_line_color(&style_ghost, lv_color_hex(0x44536a));
    lv_style_set_line_rounded(&style_ghost, true);

    g_line_ghost = lv_line_create(map);
    lv_obj_add_style(g_line_ghost, &style_ghost, 0);
    g_line = lv_line_create(map);
    lv_obj_add_style(g_line, &style_line, 0);

    /* 起点/终点 */
    lv_obj_t *pa = ui_panel_create(map, 14, 14);
    lv_obj_set_style_radius(pa, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(pa, lv_color_hex(UI_COL_GREEN), 0);
    lv_obj_set_style_bg_opa(pa, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pa, 0, 0);
    lv_obj_t *pb = ui_panel_create(map, 14, 14);
    lv_obj_set_style_radius(pb, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(pb, lv_color_hex(UI_COL_RED), 0);
    lv_obj_set_style_bg_opa(pb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pb, 0, 0);

    /* 小车 */
    g_car = ui_panel_create(map, 14, 14);
    lv_obj_set_style_radius(g_car, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(g_car, lv_color_hex(UI_COL_AMBER), 0);
    lv_obj_set_style_bg_opa(g_car, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_car, 0, 0);
    lv_obj_t *cl = lv_label_create(g_car);
    lv_label_set_text(cl, "");
    lv_obj_center(cl);

    /* 缩放按钮 */
    lv_obj_t *zin = ui_icon_btn_create(map, LV_SYMBOL_PLUS, 40);
    lv_obj_align(zin, LV_ALIGN_BOTTOM_RIGHT, -14, -14);
    lv_obj_set_user_data(zin, (void *)(intptr_t)1);
    lv_obj_add_event_cb(zin, ui_event_nav_zoom, LV_EVENT_CLICKED, NULL);
    lv_obj_t *zout = ui_icon_btn_create(map, LV_SYMBOL_MINUS, 40);
    lv_obj_align(zout, LV_ALIGN_BOTTOM_RIGHT, -62, -14);
    lv_obj_set_user_data(zout, (void *)(intptr_t)-1);
    lv_obj_add_event_cb(zout, ui_event_nav_zoom, LV_EVENT_CLICKED, NULL);

    /* 线与端点定位 (缩放后) */
    route_rescale();
    lv_obj_set_pos(pa, g_scaled[0].x - 7, g_scaled[0].y - 7);
    lv_obj_set_pos(pb, g_scaled[NAV_PTS - 1].x - 7, g_scaled[NAV_PTS - 1].y - 7);

    /* ============ 右: 信息面板 ============ */
    const lv_coord_t px = 646;
    lv_obj_t *dest = ui_card_create(parent, 342, 92);
    lv_obj_set_pos(dest, px, 14);
    lv_obj_t *d1 = ui_label_create(dest, "目的地", UI_FONT_S, UI_COL_FAINT);
    lv_obj_align(d1, LV_ALIGN_TOP_LEFT, 16, 12);
    lv_obj_t *d2 = ui_label_create(dest, "科技园 P2 停车场", UI_FONT_M, UI_COL_INK);
    lv_obj_align(d2, LV_ALIGN_TOP_LEFT, 16, 36);
    lv_obj_t *d3 = ui_label_create(dest, "途经 6 个红绿灯 · 畅通", UI_FONT_S, UI_COL_MUTED);
    lv_obj_align(d3, LV_ALIGN_BOTTOM_LEFT, 16, -12);

    lv_obj_t *steps = ui_card_create(parent, 342, 210);
    lv_obj_set_pos(steps, px, 118);
    const char *stp[] = {
        "沿当前位置向北行驶 800 米",
        "右转进入创业大道, 行驶 2.3 km",
        "靠左直行进入南坪快速, 行驶 6.1 km",
        "出快速后右转, 行驶 500 米到达",
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *dot = ui_panel_create(steps, 10, 10);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, lv_color_hex(UI_COL_ACCENT), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_pos(dot, 16, 22 + i * 46);
        lv_obj_t *tx = ui_label_create(steps, stp[i], UI_FONT_S, UI_COL_MUTED);
        lv_obj_align(tx, LV_ALIGN_TOP_LEFT, 38, 18 + i * 46);
    }

    lv_obj_t *eta = ui_card_create(parent, 342, 84);
    lv_obj_set_pos(eta, px, 340);
    lv_obj_t *e1 = ui_label_create(eta, "预计用时", UI_FONT_S, UI_COL_FAINT);
    lv_obj_align(e1, LV_ALIGN_TOP_LEFT, 16, 12);
    ui_NavEtaLabel = ui_label_create(eta, "18 分钟", UI_FONT_NUM, UI_COL_ACCENT);
    lv_obj_align(ui_NavEtaLabel, LV_ALIGN_LEFT_MID, 16, 12);
    ui_NavDistLabel = ui_label_create(eta, "剩余 12.4 km", UI_FONT_M, UI_COL_INK);
    lv_obj_align(ui_NavDistLabel, LV_ALIGN_BOTTOM_RIGHT, -16, -14);

    lv_obj_t *go = ui_btn_create(parent, "开始导航", 1);
    lv_obj_set_size(go, 342, 44);
    lv_obj_set_pos(go, px, 436);
    lv_obj_add_event_cb(go, ui_event_nav_start, LV_EVENT_CLICKED, NULL);

    g_drive = lv_timer_create(drive_cb, 700, NULL);
    lv_timer_pause(g_drive);
}
