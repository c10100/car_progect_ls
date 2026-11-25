#ifndef __UI_HELPERS_H__
#define __UI_HELPERS_H__

/*
 * ui_helpers.h - 设计系统: 颜色 / 字体 / 通用控件工厂
 *
 * 对应 HTML 原型 carpad_ui.html 的设计变量(:root), 全部集中在这一处,
 * 换肤只改这里。仿 SquareLine Studio 的 ui_helpers 层: 只放纯 UI 工具,
 * 不含任何业务逻辑, 可整体拷贝进任意 LVGL v8 工程。
 */

#include "lvgl.h"

/* ---------------- 屏幕尺寸 (板子是 1024x600, 移植时按实际屏改) ---------------- */
#define UI_HOR_RES   1024
#define UI_VER_RES   600
#define UI_TOPBAR_H  44
#define UI_DOCK_H    62

/* ---------------- 颜色 (取自 HTML 原型 :root 变量) ---------------- */
#define UI_COL_BG      0x070b12   /* 全局深底 */
#define UI_COL_BG2     0x0e1622
#define UI_COL_CARD    0x10161f   /* 玻璃卡片(已与底色混合)   */
#define UI_COL_CARD2   0x182030   /* 玻璃卡片 hover/浅一层    */
#define UI_COL_RULE    0x2a3648   /* 描边 */
#define UI_COL_INK     0xe9f0f8   /* 主文字 */
#define UI_COL_MUTED   0x8ea0b5   /* 次文字 */
#define UI_COL_FAINT   0x5d6f85   /* 弱文字 */
#define UI_COL_ACCENT  0x2fd3f5   /* 青色主强调 */
#define UI_COL_ACC2    0x7b6bff   /* 紫色次强调 */
#define UI_COL_AMBER   0xffb648
#define UI_COL_GREEN   0x34d399
#define UI_COL_RED     0xfb7185

/* ---------------- 字体 ----------------
 * 中文: lv_font_cjk_12 / lv_font_cjk_16 —— lv_font_conv 从黑体生成,
 *       覆盖 UI 全部用到的汉字/符号, 文件在 src/ui/lv_font_cjk_1x.c。
 *       UI 里新增了显示为方框的文字时, 把该字加进 range 重新生成即可。
 * 英文/数字大字: montserrat (外部 lv_conf.h 已开启 12/14/16/28/48)。 */
LV_FONT_DECLARE(lv_font_cjk_12)
LV_FONT_DECLARE(lv_font_cjk_16)

#ifndef UI_FONT_S            /* 小号文字 (含中文, 黑体 12px) */
#define UI_FONT_S   &lv_font_cjk_12
#endif
#ifndef UI_FONT_M            /* 中文正文 (黑体 16px) */
#define UI_FONT_M   &lv_font_cjk_16
#endif
#ifndef UI_FONT_NUM          /* 数字中号 */
#define UI_FONT_NUM &lv_font_montserrat_28
#endif
#ifndef UI_FONT_XL           /* 时钟大数字 */
#define UI_FONT_XL  &lv_font_montserrat_48
#endif

/* ---------------- 通用工厂 ---------------- */
/* 容器: 无滚动/无边框, 透明底 */
lv_obj_t *ui_panel_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h);
/* 玻璃卡片: 圆角20 + 描边 + 深色底 */
lv_obj_t *ui_card_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h);
/* 标签: font 传 UI_FONT_xxx, color 传 UI_COL_xxx */
lv_obj_t *ui_label_create(lv_obj_t *parent, const char *txt,
                          const lv_font_t *font, uint32_t color);
/* 按钮: accent=1 青色实底, accent=0 玻璃描边(ghost) */
lv_obj_t *ui_btn_create(lv_obj_t *parent, const char *txt, int accent);
/* 图标按钮: 用 LV_SYMBOL_XXX 文本, 圆形/方块 */
lv_obj_t *ui_icon_btn_create(lv_obj_t *parent, const char *symbol, lv_coord_t size);
/* 模拟打遮罩: 用于"点空白关闭弹层" */
lv_obj_t *ui_cloak_create(lv_obj_t *parent);

/* 对齐小工具: 让 label 在父对象里水平居中于 y */
void ui_label_center(lv_obj_t *label, lv_coord_t y);

#endif /* __UI_HELPERS_H__ */
