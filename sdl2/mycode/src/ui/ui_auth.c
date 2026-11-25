#include "ui/ui.h"

/*
 * ui_auth.c - 登录/注册屏 (对应原型 .auth)
 *   左: 品牌区 (渐变 + LOGO + 宣传语 + 特性列表)
 *   右: 登录/注册双 Tab 表单
 * 事件在 ui_events.c: 走 task_chat 的注册/登录协议
 */

lv_obj_t *ui_AuthScreen;
lv_obj_t *ui_LgAccountArea;
lv_obj_t *ui_LgPassArea;
lv_obj_t *ui_RgAccountArea;
lv_obj_t *ui_RgPassArea;
lv_obj_t *ui_RgNickArea;
lv_obj_t *ui_AuthMsgLabel;
lv_obj_t *ui_AuthTabLoginBtn;
lv_obj_t *ui_AuthTabRegBtn;
lv_obj_t *ui_AuthLoginForm;
lv_obj_t *ui_AuthRegForm;
lv_obj_t *ui_OfflineSwitch;

/* 一个输入框: 标题 + 文本域, y 为在表单内的纵向偏移, 返回文本域指针 */
static lv_obj_t *field_create(lv_obj_t *parent, lv_coord_t y, const char *title,
                              const char *placeholder, int is_pass)
{
    lv_obj_t *box = ui_panel_create(parent, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_pos(box, 0, y);          /* LVGL 子控件默认叠在 (0,0), 必须手动排布 */

    lv_obj_t *cap = ui_label_create(box, title, UI_FONT_S, UI_COL_MUTED);
    lv_obj_align(cap, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *ta = lv_textarea_create(box);
    lv_obj_set_size(ta, LV_PCT(100), 42);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 20);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, placeholder);
    lv_obj_set_style_bg_color(ta, lv_color_hex(UI_COL_BG2), 0);
    lv_obj_set_style_border_color(ta, lv_color_hex(UI_COL_RULE), 0);
    lv_obj_set_style_radius(ta, 10, 0);
    lv_obj_set_style_text_font(ta, UI_FONT_M, 0);
    if (is_pass) lv_textarea_set_password_mode(ta, true);
    return ta;
}

/* 顶部 Tab 按钮 */
static lv_obj_t *tab_create(lv_obj_t *parent, const char *txt, lv_coord_t x)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, 198, 36);
    lv_obj_set_pos(b, x, 0);
    lv_obj_set_style_radius(b, 9, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(UI_COL_BG2), 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(UI_COL_CARD2), LV_STATE_CHECKED);
    lv_obj_t *l = ui_label_create(b, txt, UI_FONT_M, UI_COL_MUTED);
    lv_obj_set_style_text_color(l, lv_color_hex(UI_COL_INK), LV_STATE_CHECKED);
    lv_obj_center(l);
    return b;
}

void ui_auth_create(void)
{
    /* ============ 屏幕 ============ */
    ui_AuthScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_AuthScreen, lv_color_hex(UI_COL_BG), 0);
    lv_obj_clear_flag(ui_AuthScreen, LV_OBJ_FLAG_SCROLLABLE);

    /* ============ 左侧品牌区 ============ */
    lv_obj_t *brand = lv_obj_create(ui_AuthScreen);
    lv_obj_set_size(brand, 563, UI_VER_RES);
    lv_obj_set_pos(brand, 0, 0);
    lv_obj_set_style_bg_color(brand, lv_color_hex(0x0d1725), 0);
    lv_obj_set_style_bg_grad_color(brand, lv_color_hex(0x0a111b), 0);
    lv_obj_set_style_bg_grad_dir(brand, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(brand, 0, 0);
    lv_obj_set_style_radius(brand, 0, 0);
    lv_obj_clear_flag(brand, LV_OBJ_FLAG_SCROLLABLE);

    /* LOGO 行 */
    lv_obj_t *mark = lv_obj_create(brand);
    lv_obj_set_size(mark, 52, 52);
    lv_obj_set_pos(mark, 36, 38);
    lv_obj_set_style_radius(mark, 15, 0);
    lv_obj_set_style_bg_color(mark, lv_color_hex(UI_COL_ACCENT), 0);
    lv_obj_set_style_bg_grad_color(mark, lv_color_hex(UI_COL_ACC2), 0);
    lv_obj_set_style_bg_grad_dir(mark, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_border_width(mark, 0, 0);
    lv_obj_clear_flag(mark, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *mk = lv_label_create(mark);
    lv_label_set_text(mk, LV_SYMBOL_DRIVE);
    lv_obj_set_style_text_color(mk, lv_color_hex(0x04252b), 0);
    lv_obj_center(mk);

    lv_obj_t *h1 = ui_label_create(brand, "CARPAD OS", UI_FONT_NUM, UI_COL_INK);
    lv_obj_align(h1, LV_ALIGN_TOP_LEFT, 100, 38);
    lv_obj_t *sub = ui_label_create(brand, "CAR  ENTERTAINMENT  PAD",
                                    UI_FONT_S, UI_COL_MUTED);
    lv_obj_align(sub, LV_ALIGN_TOP_LEFT, 101, 70);

    /* 宣传语 */
    lv_obj_t *tag1 = ui_label_create(brand, "DRIVE • ENTERTAIN • CONNECT",
                                     UI_FONT_NUM, UI_COL_INK);
    lv_obj_align(tag1, LV_ALIGN_TOP_LEFT, 36, 180);
    lv_obj_t *tag2 = ui_label_create(brand, "把一整块屏, 交给旅途",
                                     UI_FONT_M, UI_COL_MUTED);
    lv_obj_align(tag2, LV_ALIGN_TOP_LEFT, 36, 218);

    /* 特性列表 */
    const char *feats[] = {
        "音频播放    播放器模式 · 进度/音量/切歌",
        "视频相册    画中画控制 · 全屏浏览",
        "本地游戏    2048 / 井字棋 / 五子棋 / 贪吃蛇",
        "导航天气    HTTP+cJSON 在线天气 · 路况模拟",
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *it = ui_label_create(brand, feats[i], UI_FONT_M, UI_COL_MUTED);
        lv_obj_align(it, LV_ALIGN_TOP_LEFT, 36, 310 + i * 34);
    }

    /* ============ 右侧表单区 ============ */
    lv_obj_t *form = ui_panel_create(ui_AuthScreen, 461, UI_VER_RES);
    lv_obj_align(form, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(form, lv_color_hex(UI_COL_BG), 0);
    lv_obj_set_style_bg_opa(form, LV_OPA_COVER, 0);

    /* Tab 切换 */
    lv_obj_t *tabs = ui_panel_create(form, 401, 46);
    lv_obj_align(tabs, LV_ALIGN_TOP_MID, 0, 46);
    lv_obj_set_style_bg_color(tabs, lv_color_hex(UI_COL_BG2), 0);
    lv_obj_set_style_bg_opa(tabs, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(tabs, 12, 0);
    lv_obj_set_style_pad_all(tabs, 5, 0);

    ui_AuthTabLoginBtn = tab_create(tabs, "登  录", 0);
    ui_AuthTabRegBtn   = tab_create(tabs, "注  册", 198);
    lv_obj_add_state(ui_AuthTabLoginBtn, LV_STATE_CHECKED);
    lv_obj_add_event_cb(ui_AuthTabLoginBtn, ui_event_auth_tab_login, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_AuthTabRegBtn,   ui_event_auth_tab_reg,   LV_EVENT_CLICKED, NULL);

    /* 提示行 (登录/注册结果, ui_events 里设置文本) */
    ui_AuthMsgLabel = ui_label_create(form, "", UI_FONT_S, UI_COL_RED);
    lv_obj_align(ui_AuthMsgLabel, LV_ALIGN_TOP_MID, 0, 104);

    /* ---- 登录表单 ---- */
    ui_AuthLoginForm = ui_panel_create(form, 401, 290);
    lv_obj_align(ui_AuthLoginForm, LV_ALIGN_TOP_MID, 0, 130);
    ui_LgAccountArea = field_create(ui_AuthLoginForm, 0, "账号", "手机号 / 昵称", 0);
    ui_LgPassArea    = field_create(ui_AuthLoginForm, 78, "密码", "请输入密码", 1);

    lv_obj_t *offrow = ui_panel_create(ui_AuthLoginForm, LV_PCT(100), 26);
    lv_obj_align(offrow, LV_ALIGN_BOTTOM_LEFT, 0, -64);
    ui_OfflineSwitch = lv_switch_create(offrow);
    lv_obj_set_size(ui_OfflineSwitch, 44, 24);
    lv_obj_align(ui_OfflineSwitch, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t *offlb = ui_label_create(offrow, "离线演示 (无服务器直接进入)",
                                      UI_FONT_S, UI_COL_FAINT);
    lv_obj_align_to(offlb, ui_OfflineSwitch, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_obj_t *login_btn = ui_btn_create(ui_AuthLoginForm, "登  录", 1);
    lv_obj_set_size(login_btn, LV_PCT(100), 46);
    lv_obj_align(login_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(login_btn, ui_event_auth_do_login, LV_EVENT_CLICKED, NULL);

    /* ---- 注册表单 ---- */
    ui_AuthRegForm = ui_panel_create(form, 401, 290);
    lv_obj_align(ui_AuthRegForm, LV_ALIGN_TOP_MID, 0, 130);
    lv_obj_add_flag(ui_AuthRegForm, LV_OBJ_FLAG_HIDDEN);
    ui_RgAccountArea = field_create(ui_AuthRegForm, 0, "账号", "设置登录账号", 0);
    ui_RgPassArea    = field_create(ui_AuthRegForm, 78, "密码", "设置密码", 1);
    ui_RgNickArea    = field_create(ui_AuthRegForm, 156, "昵称", "车机端显示的昵称", 0);

    lv_obj_t *reg_btn = ui_btn_create(ui_AuthRegForm, "注  册", 1);
    lv_obj_set_size(reg_btn, LV_PCT(100), 46);
    lv_obj_align(reg_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(reg_btn, ui_event_auth_do_reg, LV_EVENT_CLICKED, NULL);

    /* 底部说明 */
    lv_obj_t *foot = ui_label_create(form,
        "登录走 TCP 自定义协议 (task_chat), 服务器 IP/端口 在配置文件里改",
        UI_FONT_S, UI_COL_FAINT);
    lv_obj_align(foot, LV_ALIGN_BOTTOM_MID, 0, -18);
    lv_obj_set_style_text_align(foot, LV_TEXT_ALIGN_CENTER, 0);
}
