#include "ui/ui_helpers.h"

/*
 * ui_helpers.c - 设计系统实现 (纯 UI 工具, 无业务)
 */

lv_obj_t *ui_panel_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

lv_obj_t *ui_card_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *o = ui_panel_create(parent, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(UI_COL_CARD), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(UI_COL_RULE), 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_radius(o, 20, 0);
    return o;
}

lv_obj_t *ui_label_create(lv_obj_t *parent, const char *txt,
                          const lv_font_t *font, uint32_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

lv_obj_t *ui_btn_create(lv_obj_t *parent, const char *txt, int accent)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, LV_SIZE_CONTENT, 36);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);

    if (accent) {
        /* 青紫渐变实底 + 深色文字 (对应 .btn) */
        lv_obj_set_style_bg_color(b, lv_color_hex(UI_COL_ACCENT), 0);
        lv_obj_set_style_bg_grad_color(b, lv_color_hex(UI_COL_ACC2), 0);
        lv_obj_set_style_bg_grad_dir(b, LV_GRAD_DIR_HOR, 0);
        lv_obj_t *l = ui_label_create(b, txt, UI_FONT_M, 0x04252b);
        lv_obj_center(l);
    } else {
        /* ghost: 玻璃底 + 描边 + 灰字 (对应 .ghost) */
        lv_obj_set_style_bg_color(b, lv_color_hex(UI_COL_CARD), 0);
        lv_obj_set_style_border_color(b, lv_color_hex(UI_COL_RULE), 0);
        lv_obj_set_style_border_width(b, 1, 0);
        lv_obj_t *l = ui_label_create(b, txt, UI_FONT_M, UI_COL_MUTED);
        lv_obj_center(l);
    }
    return b;
}

lv_obj_t *ui_icon_btn_create(lv_obj_t *parent, const char *symbol, lv_coord_t size)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, size, size);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(UI_COL_CARD2), 0);
    lv_obj_set_style_border_color(b, lv_color_hex(UI_COL_RULE), 0);
    lv_obj_set_style_border_width(b, 1, 0);

    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, symbol);
    lv_obj_set_style_text_color(l, lv_color_hex(UI_COL_INK), 0);
    lv_obj_center(l);
    return b;
}

lv_obj_t *ui_cloak_create(lv_obj_t *parent)
{
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_remove_style_all(c);            /* 全透明 */
    lv_obj_set_size(c, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(c, 0, 0);
    /* 可点击: 点它就当"点空白" */
    return c;
}

void ui_label_center(lv_obj_t *label, lv_coord_t y)
{
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, y);
}
