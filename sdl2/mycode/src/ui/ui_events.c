#include "common.h"
#include "ui/ui.h"
#include "ui/ui_internal.h"
#include "core/config.h"
#include "protocol.h"
#include "task/task_chat.h"
#include "task/task_weather.h"

/*
 * ui_events.c - 事件胶水层 (仿 SquareLine 的 ui_events)
 *
 * 这里是 UI 与框架之间唯一的耦合点:
 *   登录/注册 -> task_chat (TCP + 自定义协议)
 *   天气刷新  -> task_weather (HTTP + cJSON)
 * 移植到别的工程时, 只需要重写本文件的函数体。
 */

/* ============ 登录/注册 ============ */

/* 等待服务器 ACK 的类型: 0 无 1 登录 2 注册 (在 do_login/do_reg 里设置) */
static int g_auth_wait = 0;

void ui_auth_on_ack(int ok, const char *tip)
{
    char acc[ACCOUNT_LEN] = "";
    config_get_str(CFG_ACCOUNT, "", acc, sizeof(acc));

    if (g_auth_wait == 1) {                       /* 登录应答 */
        if (ok) {
            lv_label_set_text(ui_AuthMsgLabel, "");
            LOGI("login ok: %s", acc);
            ui_enter_system();
        } else {
            lv_label_set_text(ui_AuthMsgLabel, tip ? tip : "账号或密码错误");
        }
    } else if (g_auth_wait == 2) {                /* 注册应答 */
        if (ok) {
            lv_label_set_text(ui_AuthMsgLabel, "注册成功, 请登录");
            /* 切回登录 Tab */
            lv_event_send(ui_AuthTabLoginBtn, LV_EVENT_CLICKED, NULL);
        } else {
            lv_label_set_text(ui_AuthMsgLabel, tip ? tip : "注册失败 (账号已存在?)");
        }
    }
    g_auth_wait = 0;
}

static int auth_check(lv_obj_t *acc, lv_obj_t *pas, lv_obj_t *nick)
{
    if (strlen(lv_textarea_get_text(acc)) == 0 ||
        strlen(lv_textarea_get_text(pas)) == 0) {
        lv_label_set_text(ui_AuthMsgLabel, "账号和密码不能为空");
        return 0;
    }
    if (nick && strlen(lv_textarea_get_text(nick)) == 0) {
        lv_label_set_text(ui_AuthMsgLabel, "昵称不能为空");
        return 0;
    }
    lv_label_set_text(ui_AuthMsgLabel, "连接服务器中...");
    return 1;
}

void ui_event_auth_tab_login(lv_event_t *e)
{
    (void)e;
    lv_obj_add_state(ui_AuthTabLoginBtn, LV_STATE_CHECKED);
    lv_obj_clear_state(ui_AuthTabRegBtn, LV_STATE_CHECKED);
    lv_obj_clear_flag(ui_AuthLoginForm, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_AuthRegForm, LV_OBJ_FLAG_HIDDEN);
}

void ui_event_auth_tab_reg(lv_event_t *e)
{
    (void)e;
    lv_obj_add_state(ui_AuthTabRegBtn, LV_STATE_CHECKED);
    lv_obj_clear_state(ui_AuthTabLoginBtn, LV_STATE_CHECKED);
    lv_obj_clear_flag(ui_AuthRegForm, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_AuthLoginForm, LV_OBJ_FLAG_HIDDEN);
}

void ui_event_auth_do_login(lv_event_t *e)
{
    (void)e;
    /* 离线演示: 板上没起聊天服务器时直接进系统看界面 */
    if (lv_obj_has_state(ui_OfflineSwitch, LV_STATE_CHECKED)) {
        LOGI("offline demo mode -> enter system");
        ui_enter_system();
        return;
    }
    if (!auth_check(ui_LgAccountArea, ui_LgPassArea, NULL)) return;

    const char *acc = lv_textarea_get_text(ui_LgAccountArea);
    const char *pwd = lv_textarea_get_text(ui_LgPassArea);
    config_set_str(CFG_ACCOUNT, acc);
    config_set_str(CFG_PASSWD, pwd);
    config_save();

    g_auth_wait = 1;
    if (chat_login(acc, pwd) != OK) {
        g_auth_wait = 0;
        lv_label_set_text(ui_AuthMsgLabel, "无法连接服务器 (试试离线演示)");
    }
}

void ui_event_auth_do_reg(lv_event_t *e)
{
    (void)e;
    if (!auth_check(ui_RgAccountArea, ui_RgPassArea, ui_RgNickArea)) return;

    char ip[32] = "127.0.0.1";
    config_get_str(CFG_SRV_IP, "127.0.0.1", ip, sizeof(ip));
    int port = config_get_int(CFG_SRV_PORT, 8888);

    g_auth_wait = 2;
    if (chat_register(lv_textarea_get_text(ui_RgAccountArea),
                      lv_textarea_get_text(ui_RgPassArea),
                      lv_textarea_get_text(ui_RgNickArea),
                      ip, port) != OK) {
        g_auth_wait = 0;
        lv_label_set_text(ui_AuthMsgLabel, "无法连接服务器");
    }
}

/* ============ Dock / 主页宫格 ============ */
void ui_event_dock_home(lv_event_t *e)    { (void)e; ui_show_page(UI_PAGE_HOME); }
void ui_event_dock_music(lv_event_t *e)   { (void)e; ui_show_page(UI_PAGE_MUSIC); }
void ui_event_dock_video(lv_event_t *e)   { (void)e; ui_show_page(UI_PAGE_VIDEO); }
void ui_event_dock_gallery(lv_event_t *e) { (void)e; ui_show_page(UI_PAGE_GALLERY); }
void ui_event_dock_games(lv_event_t *e)   { (void)e; ui_show_page(UI_PAGE_GAMES); }
void ui_event_dock_nav(lv_event_t *e)     { (void)e; ui_show_page(UI_PAGE_NAV); }
void ui_event_dock_weather(lv_event_t *e) { (void)e; ui_show_page(UI_PAGE_WEATHER); }

void ui_event_open_music(lv_event_t *e)   { (void)e; ui_show_page(UI_PAGE_MUSIC); }
void ui_event_open_video(lv_event_t *e)   { (void)e; ui_show_page(UI_PAGE_VIDEO); }
void ui_event_open_gallery(lv_event_t *e) { (void)e; ui_show_page(UI_PAGE_GALLERY); }
void ui_event_open_games(lv_event_t *e)   { (void)e; ui_show_page(UI_PAGE_GAMES); }
void ui_event_open_nav(lv_event_t *e)     { (void)e; ui_show_page(UI_PAGE_NAV); }
void ui_event_open_weather(lv_event_t *e) { (void)e; ui_show_page(UI_PAGE_WEATHER); }

/* ============ 音乐页 ============ */
void ui_event_music_toggle(lv_event_t *e) { (void)e; ui_music_toggle(); }
void ui_event_music_prev(lv_event_t *e)   { (void)e; ui_music_prev(); }
void ui_event_music_next(lv_event_t *e)   { (void)e; ui_music_next(); }

void ui_event_music_seek(lv_event_t *e)
{
    ui_music_seek(lv_slider_get_value(lv_event_get_target(e)));
}

void ui_event_music_vol_toggle(lv_event_t *e)
{
    (void)e;
    ui_music_vol_pop(lv_obj_has_flag(ui_MusicVolPop, LV_OBJ_FLAG_HIDDEN) ? 1 : 0);
}

void ui_event_music_vol_change(lv_event_t *e)
{
    ui_music_set_vol(lv_slider_get_value(lv_event_get_target(e)));
}

void ui_event_music_cloak(lv_event_t *e)  { (void)e; ui_music_vol_pop(0); }

/* ============ 视频页 ============ */
void ui_event_video_toggle(lv_event_t *e) { (void)e; ui_video_toggle(); }

void ui_event_video_seek(lv_event_t *e)
{
    ui_video_seek(lv_slider_get_value(lv_event_get_target(e)));
}

void ui_event_video_speed_menu(lv_event_t *e)
{
    (void)e;
    ui_video_speed_menu(lv_obj_has_flag(ui_VideoSpeedMenu, LV_OBJ_FLAG_HIDDEN) ? 1 : 0);
}

void ui_event_video_speed_pick(lv_event_t *e)
{
    ui_video_set_speed((int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e)));
}

void ui_event_video_vol_change(lv_event_t *e)
{
    ui_video_set_vol(lv_slider_get_value(lv_event_get_target(e)));
}

void ui_event_video_max(lv_event_t *e)
{
    (void)e;
    /* 已最大化 -> 还原; 未最大化 -> 最大化 */
    int is_max = (lv_obj_get_parent(ui_VideoBox) == ui_SysScreen);
    ui_video_max(!is_max);
}

void ui_event_video_tap(lv_event_t *e)   { (void)e; ui_video_tap(); }
void ui_event_video_cloak(lv_event_t *e) { (void)e; ui_video_speed_menu(0); }

/* ============ 相册页 ============ */
void ui_event_gallery_open(lv_event_t *e)
{
    ui_gallery_open((int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e)));
}

void ui_event_gallery_prev(lv_event_t *e) { (void)e; ui_gallery_nav(-1); }
void ui_event_gallery_next(lv_event_t *e) { (void)e; ui_gallery_nav(1); }
void ui_event_gallery_exit(lv_event_t *e) { (void)e; ui_gallery_exit(); }

/* ============ 游戏页 ============ */
void ui_event_game_open(lv_event_t *e)
{
    ui_games_open((int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e)));
}

void ui_event_game_exit(lv_event_t *e) { (void)e; ui_games_exit(); }

/* 方向: 手势 -> dir, 按钮的 user_data 也是 dir (0上1下2左3右) */
static int gesture_dir(lv_event_t *e)
{
    lv_dir_t d = lv_indev_get_gesture_dir(lv_indev_get_act());
    switch (d) {
    case LV_DIR_TOP:    return 0;
    case LV_DIR_BOTTOM: return 1;
    case LV_DIR_LEFT:   return 2;
    case LV_DIR_RIGHT:  return 3;
    default:            return -1;
    }
}

void ui_event_2048_key(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE)
        ui_2048_move(gesture_dir(e));
    else
        ui_2048_move((int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e)));
}

void ui_event_2048_reset(lv_event_t *e) { (void)e; ui_2048_reset(); }

void ui_event_ttt_click(lv_event_t *e)
{
    ui_ttt_click((int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e)));
}

void ui_event_ttt_reset(lv_event_t *e) { (void)e; ui_ttt_reset(); }

void ui_event_gomo_click(lv_event_t *e)
{
    ui_gomo_click((int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e)));
}

void ui_event_gomo_reset(lv_event_t *e) { (void)e; ui_gomo_reset(); }

void ui_event_snake_key(lv_event_t *e)
{
    ui_snake_dir((int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e)));
}

void ui_event_snake_start(lv_event_t *e) { (void)e; ui_snake_start(); }

/* ============ 导航页 ============ */
void ui_event_nav_start(lv_event_t *e) { (void)e; ui_nav_start(); }

void ui_event_nav_zoom(lv_event_t *e)
{
    ui_nav_zoom((int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e)));
}

/* ============ 天气页 ============ */
void ui_event_weather_refresh(lv_event_t *e)
{
    (void)e;
    task_weather_refresh();
}
