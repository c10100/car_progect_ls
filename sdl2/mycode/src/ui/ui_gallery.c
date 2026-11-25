#include "ui/ui.h"
#include "ui/ui_internal.h"

/*
 * ui_gallery.c - 相册页
 *   网格缩略图 (4x2) + 全屏查看器 (图片铺满, 左右切换, 点中间/右上退出)
 *   板上移植: 缩略图与查看器换成 lv_img + lv_img_set_src(文件路径),
 *   铺满用 lv_img_set_zoom / img_wfy, 逻辑不变
 */

lv_obj_t *ui_GalleryViewer;

typedef struct { const char *name; uint32_t ca, cb; } photo_t;
static const photo_t PHOTOS[] = {
    { "海边落日",   0x2b5876, 0x4e4376 },
    { "山路十八弯", 0x134e5e, 0x71b280 },
    { "城市夜灯",   0x232526, 0x414345 },
    { "草原公路",   0x3e8e7e, 0x8bc34a },
    { "雪山湖泊",   0x1c92d2, 0xf2fcfe },
    { "峡谷星空",   0x0f2027, 0x2c5364 },
    { "红叶栈道",   0x870000, 0x190a05 },
    { "沙漠越野",   0xc79b5e, 0x6b4423 },
};
#define PHOTO_N ((int)(sizeof(PHOTOS) / sizeof(PHOTOS[0])))

static int g_view_idx = 0;
static lv_obj_t *g_view_canvas;      /* 查看器里"铺满"的照片区 */
static lv_obj_t *g_view_name;

static void view_load(void)
{
    const photo_t *p = &PHOTOS[g_view_idx];
    lv_obj_set_style_bg_color(g_view_canvas, lv_color_hex(p->ca), 0);
    lv_obj_set_style_bg_grad_color(g_view_canvas, lv_color_hex(p->cb), 0);
    lv_label_set_text_fmt(g_view_name, "%s   %d/%d",
                          p->name, g_view_idx + 1, PHOTO_N);
}

void ui_gallery_open(int idx)
{
    if (idx < 0 || idx >= PHOTO_N) return;
    g_view_idx = idx;
    view_load();
    lv_obj_move_foreground(ui_GalleryViewer);   /* viewer 建于顶栏/Dock之前, 打开时提到最上层 */
    lv_obj_clear_flag(ui_GalleryViewer, LV_OBJ_FLAG_HIDDEN);
}

void ui_gallery_nav(int step)
{
    g_view_idx = (g_view_idx + step + PHOTO_N) % PHOTO_N;
    view_load();
}

void ui_gallery_exit(void)
{
    lv_obj_add_flag(ui_GalleryViewer, LV_OBJ_FLAG_HIDDEN);
}

/* ---------- 创建 ---------- */
void ui_gallery_create(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 0, 0);

    /* 头部 */
    lv_obj_t *hd = ui_panel_create(parent, LV_PCT(100), 36);
    lv_obj_set_pos(hd, 18, 14);
    lv_obj_t *t = ui_label_create(hd, "相册", UI_FONT_M, UI_COL_INK);
    lv_obj_align(t, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t *cnt = ui_label_create(hd, "8 张 · 点击放大 · 左右切换 · 点中间退出",
                                    UI_FONT_S, UI_COL_FAINT);
    lv_obj_align(cnt, LV_ALIGN_LEFT_MID, 60, 0);

    /* 4x2 缩略图网格 */
    const int cw = 238, ch = 196, gx = 12, gy = 12, x0 = 18, y0 = 62;
    for (int i = 0; i < PHOTO_N; i++) {
        lv_obj_t *tile = lv_btn_create(parent);
        lv_obj_set_size(tile, cw, ch);
        lv_obj_set_pos(tile, x0 + (i % 4) * (cw + gx), y0 + (i / 4) * (ch + gy));
        lv_obj_set_style_radius(tile, 16, 0);
        lv_obj_set_style_shadow_width(tile, 0, 0);
        lv_obj_set_style_bg_color(tile, lv_color_hex(PHOTOS[i].ca), 0);
        lv_obj_set_style_bg_grad_color(tile, lv_color_hex(PHOTOS[i].cb), 0);
        lv_obj_set_style_bg_grad_dir(tile, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_border_color(tile, lv_color_hex(UI_COL_RULE), 0);
        lv_obj_set_style_border_width(tile, 1, 0);
        lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *nm = ui_label_create(tile, PHOTOS[i].name, UI_FONT_S, UI_COL_INK);
        lv_obj_align(nm, LV_ALIGN_BOTTOM_LEFT, 12, -12);
        lv_obj_set_style_shadow_width(nm, 8, 0);
        lv_obj_set_style_shadow_color(nm, lv_color_hex(0x000000), 0);

        lv_obj_set_user_data(tile, (void *)(intptr_t)i);
        lv_obj_add_event_cb(tile, ui_event_gallery_open, LV_EVENT_CLICKED, NULL);
    }

    /* ============ 全屏查看器: 挂在系统屏上, 盖住顶栏/Dock ============ */
    ui_GalleryViewer = ui_panel_create(ui_SysScreen, UI_HOR_RES, UI_VER_RES);
    lv_obj_set_pos(ui_GalleryViewer, 0, 0);
    lv_obj_set_style_bg_color(ui_GalleryViewer, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ui_GalleryViewer, LV_OPA_COVER, 0);   /* 不透明: 盖住顶栏/Dock, 底部名称不与Dock重叠 */
    lv_obj_add_flag(ui_GalleryViewer, LV_OBJ_FLAG_HIDDEN);

    /* 照片区: 铺满 (板上换 lv_img) */
    g_view_canvas = ui_panel_create(ui_GalleryViewer, UI_HOR_RES, UI_VER_RES - 40);
    lv_obj_set_pos(g_view_canvas, 0, 0);
    lv_obj_set_style_bg_opa(g_view_canvas, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(g_view_canvas, 0, 0);
    lv_obj_set_style_border_width(g_view_canvas, 0, 0);
    /* 点中间退出 */
    lv_obj_t *mid = lv_obj_create(g_view_canvas);
    lv_obj_set_size(mid, 300, 220);
    lv_obj_center(mid);
    lv_obj_remove_style_all(mid);
    lv_obj_add_event_cb(mid, ui_event_gallery_exit, LV_EVENT_CLICKED, NULL);

    /* 左右切换 */
    lv_obj_t *pv = ui_icon_btn_create(ui_GalleryViewer, LV_SYMBOL_LEFT, 56);
    lv_obj_align(pv, LV_ALIGN_LEFT_MID, 18, 0);
    lv_obj_add_event_cb(pv, ui_event_gallery_prev, LV_EVENT_CLICKED, NULL);
    lv_obj_t *nx = ui_icon_btn_create(ui_GalleryViewer, LV_SYMBOL_RIGHT, 56);
    lv_obj_align(nx, LV_ALIGN_RIGHT_MID, -18, 0);
    lv_obj_add_event_cb(nx, ui_event_gallery_next, LV_EVENT_CLICKED, NULL);

    /* 退出 + 名称 */
    lv_obj_t *ex = ui_icon_btn_create(ui_GalleryViewer, LV_SYMBOL_CLOSE, 44);
    lv_obj_align(ex, LV_ALIGN_TOP_RIGHT, -14, 10);
    lv_obj_add_event_cb(ex, ui_event_gallery_exit, LV_EVENT_CLICKED, NULL);

    g_view_name = ui_label_create(ui_GalleryViewer, "", UI_FONT_M, UI_COL_INK);
    lv_obj_align(g_view_name, LV_ALIGN_BOTTOM_MID, 0, -8);
}
