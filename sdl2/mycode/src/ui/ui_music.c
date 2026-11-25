#include <stdio.h>
#include "ui/ui.h"
#include "ui/ui_internal.h"

/*
 * ui_music.c - 音频播放页 (网易云"播放器模式"布局)
 *   上: 大封面 + 歌名/歌手 (居中)
 *   下: 与上方同一图层的播放条: 进度细线 + 左信息 + 中控制 + 右音量
 *   音量点击弹出调节条 (弹层 + 点空白关闭)
 */

lv_obj_t *ui_MusicCover;
lv_obj_t *ui_MusicTitleLabel;
lv_obj_t *ui_MusicArtistLabel;
lv_obj_t *ui_MusicSmallTitle;
lv_obj_t *ui_MusicSlider;
lv_obj_t *ui_MusicTimeCur;
lv_obj_t *ui_MusicTimeAll;
lv_obj_t *ui_MusicPlaySym;
lv_obj_t *ui_MusicVolPop;
lv_obj_t *ui_MusicVolSlider;
lv_obj_t *ui_MusicVolVal;

typedef struct { const char *title; const char *artist; int dur; } song_t;
static const song_t SONGS[] = {
    { "夜曲",           "周杰伦",  226 },
    { "海阔天空",       "Beyond",  326 },
    { "Highway Star",   "Deep Purple", 308 },
    { "Working Man",    "Rush",    447 },
};
#define SONG_N  ((int)(sizeof(SONGS) / sizeof(SONGS[0])))

static int g_idx = 0, g_pos = 0, g_playing = 0, g_vol = 62;
static lv_obj_t *g_cloak;
static const uint32_t COVER_COLS[SONG_N] = {
    UI_COL_ACC2, UI_COL_AMBER, UI_COL_GREEN, UI_COL_RED
};

static void fmt_time(char *buf, int s)
{
    snprintf(buf, 9, "%02d:%02d", s / 60, s % 60);
}

static void song_load(void)
{
    const song_t *s = &SONGS[g_idx];
    lv_label_set_text(ui_MusicTitleLabel, s->title);
    lv_label_set_text(ui_MusicArtistLabel, s->artist);
    lv_label_set_text_fmt(ui_MusicSmallTitle, "%s - %s", s->title, s->artist);
    char t[9];
    fmt_time(t, s->dur);
    lv_label_set_text(ui_MusicTimeAll, t);
    lv_slider_set_range(ui_MusicSlider, 0, s->dur);
    /* 封面换色 (板上替换为真实专辑图 lv_img) */
    lv_obj_set_style_bg_color(ui_MusicCover, lv_color_hex(COVER_COLS[g_idx]), 0);
    g_pos = 0;
    lv_slider_set_value(ui_MusicSlider, 0, LV_ANIM_OFF);
    fmt_time(t, 0);
    lv_label_set_text(ui_MusicTimeCur, t);
}

static void play_sync(void)
{
    lv_label_set_text(ui_MusicPlaySym,
                      g_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
}

static void tick_cb(lv_timer_t *t)
{
    (void)t;
    if (!g_playing) return;
    if (++g_pos >= SONGS[g_idx].dur) {          /* 播完自动下一首 */
        g_idx = (g_idx + 1) % SONG_N;
        song_load();
        return;
    }
    lv_slider_set_value(ui_MusicSlider, g_pos, LV_ANIM_OFF);
    char buf[9];
    fmt_time(buf, g_pos);
    lv_label_set_text(ui_MusicTimeCur, buf);
}

/* ---------- 内部动作 (ui_events 调用) ---------- */
void ui_music_toggle(void)
{
    g_playing = !g_playing;
    play_sync();
}

void ui_music_prev(void)
{
    g_idx = (g_idx + SONG_N - 1) % SONG_N;
    song_load();
}

void ui_music_next(void)
{
    g_idx = (g_idx + 1) % SONG_N;
    song_load();
}

void ui_music_seek(int sec)
{
    if (sec < 0) sec = 0;
    if (sec > SONGS[g_idx].dur) sec = SONGS[g_idx].dur;
    g_pos = sec;
    char t[9];
    fmt_time(t, sec);
    lv_label_set_text(ui_MusicTimeCur, t);
}

void ui_music_vol_pop(int on)
{
    if (!ui_MusicVolPop) return;
    if (on) {
        lv_obj_move_foreground(g_cloak);
        lv_obj_clear_flag(g_cloak, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_MusicVolPop, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ui_MusicVolPop);
    } else {
        lv_obj_add_flag(g_cloak, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_MusicVolPop, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_music_set_vol(int v)
{
    g_vol = v;
    lv_slider_set_value(ui_MusicVolSlider, v, LV_ANIM_OFF);
    lv_label_set_text_fmt(ui_MusicVolVal, "%d", v);
}

/* ---------- 创建 ---------- */
void ui_music_create(lv_obj_t *parent)
{
    /* 暖棕渐变背景 (对应 #page-music) */
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x3c2b1e), 0);
    lv_obj_set_style_bg_grad_color(parent, lv_color_hex(0x19110c), 0);
    lv_obj_set_style_bg_grad_dir(parent, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(parent, 0, 0);

    /* ============ 上半: 大封面 ============ */
    ui_MusicCover = ui_panel_create(parent, 220, 220);
    lv_obj_align(ui_MusicCover, LV_ALIGN_TOP_MID, 0, 26);
    lv_obj_set_style_radius(ui_MusicCover, 20, 0);
    lv_obj_set_style_bg_opa(ui_MusicCover, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_dir(ui_MusicCover, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_grad_color(ui_MusicCover, lv_color_hex(UI_COL_BG), 0);
    lv_obj_set_style_border_width(ui_MusicCover, 0, 0);
    lv_obj_set_style_shadow_width(ui_MusicCover, 60, 0);
    lv_obj_set_style_shadow_color(ui_MusicCover, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(ui_MusicCover, LV_OPA_50, 0);
    lv_obj_t *vinyl = ui_label_create(ui_MusicCover, LV_SYMBOL_AUDIO,
                                      &lv_font_montserrat_48, UI_COL_INK);
    lv_obj_center(vinyl);

    ui_MusicTitleLabel = ui_label_create(parent, "--", UI_FONT_M, UI_COL_INK);
    lv_obj_align(ui_MusicTitleLabel, LV_ALIGN_TOP_MID, 0, 258);
    ui_MusicArtistLabel = ui_label_create(parent, "--", UI_FONT_S, UI_COL_MUTED);
    lv_obj_align(ui_MusicArtistLabel, LV_ALIGN_TOP_MID, 0, 284);

    /* ============ 下半: 播放条 (与上方同图层, 只留顶部进度线) ============ */
    lv_obj_t *bar = ui_panel_create(parent, LV_PCT(100), 88);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(UI_COL_RULE), 0);

    /* 进度: 细长 slider, 渐变指示条 */
    ui_MusicSlider = lv_slider_create(bar);
    lv_obj_set_size(ui_MusicSlider, LV_PCT(100), 16);
    lv_obj_align(ui_MusicSlider, LV_ALIGN_TOP_MID, 0, -8);
    lv_obj_set_style_bg_opa(ui_MusicSlider, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(ui_MusicSlider, 5, LV_PART_KNOB);
    lv_obj_set_style_bg_color(ui_MusicSlider, lv_color_hex(UI_COL_RULE), LV_PART_MAIN);
    lv_obj_set_style_radius(ui_MusicSlider, 3, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_MusicSlider, lv_color_hex(UI_COL_ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(ui_MusicSlider, lv_color_hex(UI_COL_AMBER), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(ui_MusicSlider, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ui_MusicSlider, lv_color_hex(UI_COL_INK), LV_PART_KNOB);
    lv_obj_add_event_cb(ui_MusicSlider, ui_event_music_seek, LV_EVENT_VALUE_CHANGED, NULL);

    /* 左: 小封面 + 歌名 */
    lv_obj_t *mini = ui_panel_create(bar, 54, 54);
    lv_obj_align(mini, LV_ALIGN_LEFT_MID, 10, 8);
    lv_obj_set_style_radius(mini, 12, 0);
    lv_obj_set_style_bg_color(mini, lv_color_hex(UI_COL_CARD2), 0);
    lv_obj_set_style_bg_opa(mini, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(mini, 0, 0);
    lv_obj_t *mi = ui_label_create(mini, LV_SYMBOL_AUDIO, &lv_font_montserrat_16, UI_COL_MUTED);
    lv_obj_center(mi);

    lv_obj_t *mt = ui_label_create(bar, "--", UI_FONT_S, UI_COL_INK);
    lv_obj_align(mt, LV_ALIGN_LEFT_MID, 76, 0);
    ui_MusicSmallTitle = mt;

    /* 中: 时间 + 控制 */
    ui_MusicTimeCur = ui_label_create(bar, "00:00", UI_FONT_S, UI_COL_MUTED);
    lv_obj_align(ui_MusicTimeCur, LV_ALIGN_BOTTOM_MID, -480, -65);

    lv_obj_t *btn_prev = ui_icon_btn_create(bar, LV_SYMBOL_PREV, 46);
    lv_obj_align(btn_prev, LV_ALIGN_BOTTOM_MID, -80, -15);
    lv_obj_add_event_cb(btn_prev, ui_event_music_prev, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_play = ui_icon_btn_create(bar, LV_SYMBOL_PLAY, 62);
    lv_obj_align(btn_play, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn_play, lv_color_hex(UI_COL_ACCENT), 0);
    lv_obj_set_style_bg_grad_color(btn_play, lv_color_hex(UI_COL_ACC2), 0);
    lv_obj_set_style_bg_grad_dir(btn_play, LV_GRAD_DIR_HOR, 0);
    ui_MusicPlaySym = lv_obj_get_child(btn_play, 0);
    lv_obj_add_event_cb(btn_play, ui_event_music_toggle, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_next = ui_icon_btn_create(bar, LV_SYMBOL_NEXT, 46);
    lv_obj_align(btn_next, LV_ALIGN_BOTTOM_MID, 80, -15);
    lv_obj_add_event_cb(btn_next, ui_event_music_next, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_loop = ui_icon_btn_create(bar, LV_SYMBOL_LOOP, 38);
    lv_obj_align(btn_loop, LV_ALIGN_BOTTOM_MID, 150, -18);
    lv_obj_add_event_cb(btn_loop, ui_event_music_next, LV_EVENT_CLICKED, NULL);

    ui_MusicTimeAll = ui_label_create(bar, "00:00", UI_FONT_S, UI_COL_MUTED);
    lv_obj_align(ui_MusicTimeAll, LV_ALIGN_BOTTOM_MID, 480, -65);

    /* 右: 音量 + 弹层 */
    lv_obj_t *btn_vol = ui_icon_btn_create(bar, LV_SYMBOL_VOLUME_MAX, 38);
    lv_obj_align(btn_vol, LV_ALIGN_RIGHT_MID, -66, 8);
    lv_obj_add_event_cb(btn_vol, ui_event_music_vol_toggle, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_more = ui_icon_btn_create(bar, LV_SYMBOL_BARS, 38);
    lv_obj_align(btn_more, LV_ALIGN_RIGHT_MID, -16, 8);
    lv_obj_add_event_cb(btn_more, ui_event_music_vol_toggle, LV_EVENT_CLICKED, NULL);

    /* 点空白关闭的遮罩 (先建, 压在内容上、弹层下) */
    g_cloak = ui_cloak_create(parent);
    lv_obj_add_flag(g_cloak, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(g_cloak, ui_event_music_cloak, LV_EVENT_CLICKED, NULL);

    ui_MusicVolPop = ui_panel_create(parent, 232, 100);
    lv_obj_align(ui_MusicVolPop, LV_ALIGN_BOTTOM_RIGHT, -12, -104);
    lv_obj_set_style_bg_color(ui_MusicVolPop, lv_color_hex(0x160f0b), 0);
    lv_obj_set_style_bg_opa(ui_MusicVolPop, LV_OPA_90, 0);
    lv_obj_set_style_border_color(ui_MusicVolPop, lv_color_hex(UI_COL_RULE), 0);
    lv_obj_set_style_border_width(ui_MusicVolPop, 1, 0);
    lv_obj_set_style_radius(ui_MusicVolPop, 14, 0);
    lv_obj_set_style_shadow_width(ui_MusicVolPop, 40, 0);
    lv_obj_add_flag(ui_MusicVolPop, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *vic = ui_label_create(ui_MusicVolPop, LV_SYMBOL_VOLUME_MAX,
                                    &lv_font_montserrat_16, UI_COL_INK);
    lv_obj_align(vic, LV_ALIGN_LEFT_MID, 14, 0);
    ui_MusicVolSlider = lv_slider_create(ui_MusicVolPop);
    lv_obj_set_size(ui_MusicVolSlider, 130, 12);
    lv_obj_align(ui_MusicVolSlider, LV_ALIGN_LEFT_MID, 44, 0);
    lv_slider_set_range(ui_MusicVolSlider, 0, 100);
    lv_obj_set_style_bg_color(ui_MusicVolSlider, lv_color_hex(UI_COL_RULE), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_MusicVolSlider, lv_color_hex(UI_COL_ACCENT), LV_PART_INDICATOR);
    lv_obj_add_event_cb(ui_MusicVolSlider, ui_event_music_vol_change, LV_EVENT_VALUE_CHANGED, NULL);
    ui_MusicVolVal = ui_label_create(ui_MusicVolPop, "62", UI_FONT_S, UI_COL_INK);
    lv_obj_align(ui_MusicVolVal, LV_ALIGN_RIGHT_MID, -10, 0);

    song_load();
    play_sync();
    ui_music_set_vol(g_vol);
    lv_timer_create(tick_cb, 1000, NULL);
}

void ui_music_on_leave(void)
{
    ui_music_vol_pop(0);
}
