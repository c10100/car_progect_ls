#include "common.h"
#include "ui/ui.h"
#include "ui/ui_internal.h"

/*
 * ui_games.c - 游戏中心: 2048 / 井字棋 / 五子棋(9路) / 贪吃蛇
 *   全部为本地双人或人机演示逻辑; 联机版可接 task_game (UDP) 扩展。
 */

static lv_obj_t *g_hub;
static lv_obj_t *g_wrap[UI_GAME_NUM];
static lv_obj_t *g_status[UI_GAME_NUM];      /* 每个游戏的提示行 */

static void wrap_open(int game)
{
    lv_obj_add_flag(g_hub, LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < UI_GAME_NUM; i++)
        lv_obj_add_flag(g_wrap[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_wrap[game], LV_OBJ_FLAG_HIDDEN);
}

void ui_games_open(int game)
{
    if (game < 0 || game >= UI_GAME_NUM) return;
    wrap_open(game);
}

void ui_games_exit(void)
{
    for (int i = 0; i < UI_GAME_NUM; i++)
        lv_obj_add_flag(g_wrap[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_hub, LV_OBJ_FLAG_HIDDEN);
}

/* 游戏层公共骨架: 返回内容容器 */
static lv_obj_t *wrap_create(lv_obj_t *parent, int game, const char *title)
{
    lv_obj_t *w = ui_panel_create(parent, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(w, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *bk = ui_btn_create(w, "返回", 0);
    lv_obj_set_pos(bk, 18, 14);
    lv_obj_add_event_cb(bk, ui_event_game_exit, LV_EVENT_CLICKED, NULL);
    lv_obj_t *tt = ui_label_create(w, title, UI_FONT_M, UI_COL_INK);
    lv_obj_align(tt, LV_ALIGN_TOP_LEFT, 90, 24);
    g_status[game] = ui_label_create(w, "", UI_FONT_S, UI_COL_MUTED);
    lv_obj_align(g_status[game], LV_ALIGN_TOP_RIGHT, -18, 24);
    return w;
}

/* ============================================================
 * 2048
 * ============================================================ */
static int g_grid[4][4];
static lv_obj_t *g_cell[4][4];
static lv_obj_t *g_board2048;

static const uint32_t VCOL[] = {
    0x2c3a4d, 0x34506b, 0x2f6f9f, 0x2f9fae, 0x35c48f,
    0xa3c94d, 0xd9a441, 0xe08641, 0xe06350, 0xfb7185, 0x7b6bff,
};
static uint32_t col_of(int v)
{
    int t = 0, x = v;
    while ((x >>= 1) > 1) t++;          /* 2->0 4->1 8->2 ... 2048->10 */
    if (t < 0) t = 0;
    if (t > 10) t = 10;
    return VCOL[t];
}

static void draw_2048(void)
{
    int empty = 0;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            int v = g_grid[r][c];
            if (!v) empty++;
            lv_obj_t *cell = g_cell[r][c];
            if (v) {
                lv_label_set_text_fmt(lv_obj_get_child(cell, 0), "%d", v);
                lv_obj_set_style_bg_color(cell, lv_color_hex(col_of(v)), 0);
            } else {
                lv_label_set_text(lv_obj_get_child(cell, 0), "");
                lv_obj_set_style_bg_color(cell, lv_color_hex(0x1a222e), 0);
            }
        }
    int best = 0;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (g_grid[r][c] > best) best = g_grid[r][c];
    if (!empty)
        lv_label_set_text(g_status[UI_GAME_2048], "没有空格了, 点重开");
    else if (best >= 2048)
        lv_label_set_text(g_status[UI_GAME_2048], "达成 2048 !");
    else
        lv_label_set_text_fmt(g_status[UI_GAME_2048], "滑动或按方向键合并");
}

static void spawn_2048(void)
{
    int empt[16], n = 0;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (!g_grid[r][c]) empt[n++] = r * 4 + c;
    if (!n) return;
    int pick = empt[rand() % n];
    g_grid[pick / 4][pick % 4] = (rand() % 10 == 0) ? 4 : 2;
}

/* 把一维行按方向压缩合并, moved 返回是否变化 */
static int merge_line(int *a)   /* a[0..3], 向索引0方向合并 */
{
    int b[4] = { 0 }, n = 0, moved = 0;   /* b 必须清零: 压缩后尾部若为栈垃圾会被写回棋盘 */
    for (int i = 0; i < 4; i++)
        if (a[i]) b[n++] = a[i];
    for (int i = 0; i < n - 1; i++)
        if (b[i] == b[i + 1]) {
            b[i] *= 2; b[i + 1] = 0; i++;
        }
    int c[4] = { 0 };
    n = 0;
    for (int i = 0; i < 4; i++)
        if (b[i]) c[n++] = b[i];
    for (int i = 0; i < 4; i++) {
        if (a[i] != c[i]) moved = 1;
        a[i] = c[i];
    }
    return moved;
}

void ui_2048_move(int dir)      /* 0上 1下 2左 3右 */
{
    int line[4], moved = 0;
    for (int k = 0; k < 4; k++) {
        for (int j = 0; j < 4; j++) {
            int r, c;
            if (dir == 0)      { r = j; c = k; }
            else if (dir == 1) { r = 3 - j; c = k; }
            else if (dir == 2) { r = k; c = j; }
            else               { r = k; c = 3 - j; }
            line[j] = g_grid[r][c];
        }
        moved |= merge_line(line);
        for (int j = 0; j < 4; j++) {
            int r, c;
            if (dir == 0)      { r = j; c = k; }
            else if (dir == 1) { r = 3 - j; c = k; }
            else if (dir == 2) { r = k; c = j; }
            else               { r = k; c = 3 - j; }
            g_grid[r][c] = line[j];
        }
    }
    if (moved) spawn_2048();
    draw_2048();
}

void ui_2048_reset(void)
{
    memset(g_grid, 0, sizeof(g_grid));
    spawn_2048();
    spawn_2048();
    draw_2048();
}

static void build_2048(lv_obj_t *parent)
{
    g_board2048 = ui_panel_create(parent, 284, 284);
    lv_obj_align(g_board2048, LV_ALIGN_TOP_MID, -70, 60);
    lv_obj_set_style_radius(g_board2048, 14, 0);
    lv_obj_set_style_bg_color(g_board2048, lv_color_hex(0x121a26), 0);
    lv_obj_set_style_bg_opa(g_board2048, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(g_board2048, lv_color_hex(UI_COL_RULE), 0);
    lv_obj_set_style_border_width(g_board2048, 1, 0);
    /* 手势滑动 */
    lv_obj_add_event_cb(g_board2048, ui_event_2048_key, LV_EVENT_GESTURE, NULL);

    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            lv_obj_t *cell = ui_panel_create(g_board2048, 64, 64);
            lv_obj_set_pos(cell, 6 + c * 70, 6 + r * 70);
            lv_obj_set_style_radius(cell, 10, 0);
            lv_obj_set_style_bg_color(cell, lv_color_hex(0x1a222e), 0);
            lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(cell, 0, 0);
            lv_obj_t *lb = ui_label_create(cell, "", UI_FONT_NUM, UI_COL_INK);
            lv_obj_center(lb);
            g_cell[r][c] = cell;
        }

    /* 方向键盘 */
    static const lv_coord_t dxdy[4][2] = {
        { 0, -46 }, { 0, 46 }, { -52, 0 }, { 52, 0 },   /* 上 下 左 右 */
    };
    static const char *arrows[4] = { LV_SYMBOL_UP, LV_SYMBOL_DOWN,
                                     LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT };
    for (int d = 0; d < 4; d++) {
        lv_obj_t *b = ui_icon_btn_create(parent, arrows[d], 44);
        lv_obj_align(b, LV_ALIGN_TOP_MID, 220 + dxdy[d][0], 166 + dxdy[d][1]);
        lv_obj_set_user_data(b, (void *)(intptr_t)d);
        lv_obj_add_event_cb(b, ui_event_2048_key, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_t *rs = ui_btn_create(parent, "重开", 0);
    lv_obj_align(rs, LV_ALIGN_TOP_MID, 220, 60);
    lv_obj_add_event_cb(rs, ui_event_2048_reset, LV_EVENT_CLICKED, NULL);
}

/* ============================================================
 * 井字棋
 * ============================================================ */
static int g_ttt[9], g_ttt_turn = 1;      /* 1=X 2=O */
static lv_obj_t *g_ttt_btn[9];

static const int WIN8[8][3] = {
    { 0, 1, 2 }, { 3, 4, 5 }, { 6, 7, 8 },
    { 0, 3, 6 }, { 1, 4, 7 }, { 2, 5, 8 },
    { 0, 4, 8 }, { 2, 4, 6 },
};

static int ttt_winner(void)
{
    for (int i = 0; i < 8; i++)
        if (g_ttt[WIN8[i][0]] && g_ttt[WIN8[i][0]] == g_ttt[WIN8[i][1]] &&
            g_ttt[WIN8[i][0]] == g_ttt[WIN8[i][2]])
            return g_ttt[WIN8[i][0]];
    return 0;
}

static void ttt_sync(void)
{
    int w = ttt_winner(), full = 1;
    for (int i = 0; i < 9; i++)
        if (!g_ttt[i]) full = 0;
    if (w)
        lv_label_set_text_fmt(g_status[UI_GAME_TTT], "%c 获胜! 点重开再来",
                              w == 1 ? 'X' : 'O');
    else if (full)
        lv_label_set_text(g_status[UI_GAME_TTT], "平局, 点重开再来");
    else
        lv_label_set_text_fmt(g_status[UI_GAME_TTT], "轮到 %c", g_ttt_turn == 1 ? 'X' : 'O');
}

void ui_ttt_click(int cell)
{
    if (cell < 0 || cell > 8 || g_ttt[cell] || ttt_winner()) return;
    g_ttt[cell] = g_ttt_turn;
    lv_label_set_text_fmt(lv_obj_get_child(g_ttt_btn[cell], 0), "%c",
                          g_ttt_turn == 1 ? 'X' : 'O');
    lv_obj_set_style_text_color(lv_obj_get_child(g_ttt_btn[cell], 0),
                                lv_color_hex(g_ttt_turn == 1 ? UI_COL_ACCENT : UI_COL_AMBER), 0);
    g_ttt_turn = 3 - g_ttt_turn;
    ttt_sync();
}

void ui_ttt_reset(void)
{
    memset(g_ttt, 0, sizeof(g_ttt));
    g_ttt_turn = 1;
    for (int i = 0; i < 9; i++)
        lv_label_set_text(lv_obj_get_child(g_ttt_btn[i], 0), "");
    ttt_sync();
}

static void build_ttt(lv_obj_t *parent)
{
    lv_obj_t *bd = ui_panel_create(parent, 296, 296);
    lv_obj_align(bd, LV_ALIGN_TOP_MID, -60, 60);
    lv_obj_set_style_bg_opa(bd, LV_OPA_TRANSP, 0);
    for (int i = 0; i < 9; i++) {
        lv_obj_t *b = ui_icon_btn_create(bd, "", 88);
        lv_obj_set_pos(b, (i % 3) * 104, (i / 3) * 104);
        lv_obj_set_style_radius(b, 14, 0);
        lv_obj_set_style_text_font(lv_obj_get_child(b, 0), &lv_font_montserrat_28, 0);
        g_ttt_btn[i] = b;
        lv_obj_set_user_data(b, (void *)(intptr_t)i);
        lv_obj_add_event_cb(b, ui_event_ttt_click, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_t *rs = ui_btn_create(parent, "重开", 0);
    lv_obj_align(rs, LV_ALIGN_TOP_MID, 150, 160);
    lv_obj_add_event_cb(rs, ui_event_ttt_reset, LV_EVENT_CLICKED, NULL);
}

/* ============================================================
 * 五子棋 9 路
 * ============================================================ */
static int g_gomo[81], g_gomo_turn = 1;   /* 1黑 2白 */
static lv_obj_t *g_gomo_btn[81];

static int gomo_win(int idx)
{
    static const int dirs[4][2] = { { 0, 1 }, { 1, 0 }, { 1, 1 }, { 1, -1 } };
    int r = idx / 9, c = idx % 9, me = g_gomo[idx];
    for (int d = 0; d < 4; d++) {
        int cnt = 1;
        for (int s = -1; s <= 1; s += 2)
            for (int k = 1; k < 5; k++) {
                int rr = r + dirs[d][0] * k * s, cc = c + dirs[d][1] * k * s;
                if (rr < 0 || rr > 8 || cc < 0 || cc > 8) break;
                if (g_gomo[rr * 9 + cc] != me) break;
                cnt++;
            }
        if (cnt >= 5) return 1;
    }
    return 0;
}

static void gomo_sync(void)
{
    lv_label_set_text_fmt(g_status[UI_GAME_GOMO], "轮到 %s",
                          g_gomo_turn == 1 ? "黑棋" : "白棋");
}

void ui_gomo_click(int cell)
{
    if (cell < 0 || cell > 80 || g_gomo[cell]) return;
    g_gomo[cell] = g_gomo_turn;
    lv_obj_set_style_bg_color(g_gomo_btn[cell],
        lv_color_hex(g_gomo_turn == 1 ? 0x11161d : 0xe9f0f8), 0);
    if (gomo_win(cell)) {
        lv_label_set_text_fmt(g_status[UI_GAME_GOMO], "%s 五连获胜! 点重开",
                              g_gomo_turn == 1 ? "黑棋" : "白棋");
        return;
    }
    g_gomo_turn = 3 - g_gomo_turn;
    gomo_sync();
}

void ui_gomo_reset(void)
{
    memset(g_gomo, 0, sizeof(g_gomo));
    g_gomo_turn = 1;
    for (int i = 0; i < 81; i++)
        lv_obj_set_style_bg_color(g_gomo_btn[i], lv_color_hex(0x223047), 0);
    gomo_sync();
}

static void build_gomo(lv_obj_t *parent)
{
    lv_obj_t *bd = ui_panel_create(parent, 330, 330);
    lv_obj_align(bd, LV_ALIGN_TOP_MID, -60, 56);
    lv_obj_set_style_bg_opa(bd, LV_OPA_TRANSP, 0);
    for (int i = 0; i < 81; i++) {
        lv_obj_t *b = lv_btn_create(bd);
        lv_obj_set_size(b, 34, 34);
        lv_obj_set_pos(b, (i % 9) * 33, (i / 9) * 33);
        lv_obj_set_style_radius(b, 6, 0);
        lv_obj_set_style_shadow_width(b, 0, 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x223047), 0);
        lv_obj_set_style_border_color(b, lv_color_hex(UI_COL_RULE), 0);
        lv_obj_set_style_border_width(b, 1, 0);
        g_gomo_btn[i] = b;
        lv_obj_set_user_data(b, (void *)(intptr_t)i);
        lv_obj_add_event_cb(b, ui_event_gomo_click, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_t *rs = ui_btn_create(parent, "重开", 0);
    lv_obj_align(rs, LV_ALIGN_TOP_MID, 150, 160);
    lv_obj_add_event_cb(rs, ui_event_gomo_reset, LV_EVENT_CLICKED, NULL);
}

/* ============================================================
 * 贪吃蛇 (canvas 24x16 格, 5px/格, 放大4倍显示)
 * ============================================================ */
#define SN_COLS 24
#define SN_ROWS 16
#define SN_CELL 5
#define SN_MAX  64

static lv_color_t sn_buf[SN_COLS * SN_ROWS * SN_CELL * SN_CELL];
static lv_obj_t *g_sn_canvas;
static int8_t sn_x[SN_MAX], sn_y[SN_MAX];
static int sn_len = 3, sn_dir = 3, sn_run = 0, sn_food_x, sn_food_y;
static lv_timer_t *sn_timer;

static void sn_draw(void)
{
    lv_canvas_fill_bg(g_sn_canvas, lv_color_hex(0x0c1420), LV_OPA_COVER);
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_opa = LV_OPA_COVER;
    d.border_width = 0;

    d.bg_color = lv_color_hex(UI_COL_RED);        /* 食物 */
    lv_canvas_draw_rect(g_sn_canvas, sn_food_x * SN_CELL, sn_food_y * SN_CELL,
                        SN_CELL, SN_CELL, &d);
    for (int i = 0; i < sn_len; i++) {
        d.bg_color = lv_color_hex(i == 0 ? UI_COL_AMBER : UI_COL_GREEN);
        lv_canvas_draw_rect(g_sn_canvas, sn_x[i] * SN_CELL, sn_y[i] * SN_CELL,
                            SN_CELL, SN_CELL, &d);
    }
    lv_obj_invalidate(g_sn_canvas);
}

static void sn_place_food(void)
{
    sn_food_x = rand() % SN_COLS;
    sn_food_y = rand() % SN_ROWS;
}

static void sn_reset(void)
{
    sn_len = 3;
    sn_dir = 3;
    for (int i = 0; i < sn_len; i++) { sn_x[i] = 8 - i; sn_y[i] = 8; }
    sn_place_food();
    sn_draw();
    lv_label_set_text(g_status[UI_GAME_SNAKE], "按开始, 方向键控制");
}

static void sn_step(void)
{
    /* 身体后移 */
    for (int i = sn_len - 1; i > 0; i--) { sn_x[i] = sn_x[i - 1]; sn_y[i] = sn_y[i - 1]; }
    static const int8_t dxdy[4][2] = { { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 } };
    sn_x[0] += dxdy[sn_dir][0];
    sn_y[0] += dxdy[sn_dir][1];

    if (sn_x[0] < 0 || sn_x[0] >= SN_COLS || sn_y[0] < 0 || sn_y[0] >= SN_ROWS) {
        sn_run = 0;
        lv_label_set_text(g_status[UI_GAME_SNAKE], "撞墙! 按开始重来");
        sn_draw();
        return;
    }
    for (int i = 1; i < sn_len; i++)
        if (sn_x[i] == sn_x[0] && sn_y[i] == sn_y[0]) {
            sn_run = 0;
            lv_label_set_text(g_status[UI_GAME_SNAKE], "咬到自己! 按开始重来");
            sn_draw();
            return;
        }
    if (sn_x[0] == sn_food_x && sn_y[0] == sn_food_y && sn_len < SN_MAX) {
        sn_len++;
        sn_place_food();
        lv_label_set_text_fmt(g_status[UI_GAME_SNAKE], "得分 %d", sn_len - 3);
    }
    sn_draw();
}

static void sn_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (sn_run) sn_step();
}

void ui_snake_dir(int dir)
{
    if (dir < 0 || dir > 3) return;
    sn_dir = dir;
}

void ui_snake_start(void)
{
    if (sn_run) {
        sn_run = 0;
        lv_label_set_text(g_status[UI_GAME_SNAKE], "已暂停, 按开始继续");
    } else {
        if (sn_len <= 3 || sn_x[0] < 0) sn_reset();
        sn_run = 1;
    }
}

static void build_snake(lv_obj_t *parent)
{
    g_sn_canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(g_sn_canvas, sn_buf, SN_COLS * SN_CELL, SN_ROWS * SN_CELL,
                         LV_IMG_CF_TRUE_COLOR);
    lv_img_set_pivot(g_sn_canvas, 0, 0);
    lv_img_set_zoom(g_sn_canvas, 1024);           /* 4 倍: 480x320 */
    lv_img_set_antialias(g_sn_canvas, false);
    lv_obj_align(g_sn_canvas, LV_ALIGN_TOP_MID, -90, 66);

    static const lv_coord_t dxdy[4][2] = {
        { 0, -46 }, { 0, 46 }, { -52, 0 }, { 52, 0 },
    };
    static const char *arrows[4] = { LV_SYMBOL_UP, LV_SYMBOL_DOWN,
                                     LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT };
    for (int d = 0; d < 4; d++) {
        lv_obj_t *b = ui_icon_btn_create(parent, arrows[d], 44);
        lv_obj_align(b, LV_ALIGN_TOP_MID, 240 + dxdy[d][0], 172 + dxdy[d][1]);
        lv_obj_set_user_data(b, (void *)(intptr_t)d);
        lv_obj_add_event_cb(b, ui_event_snake_key, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_t *st = ui_btn_create(parent, "开始", 1);
    lv_obj_align(st, LV_ALIGN_TOP_MID, 240, 60);
    lv_obj_add_event_cb(st, ui_event_snake_start, LV_EVENT_CLICKED, NULL);

    sn_timer = lv_timer_create(sn_timer_cb, 280, NULL);
    sn_reset();
}

/* ============================================================ */
void ui_games_create(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 0, 0);

    /* ---- 中心 hub ---- */
    g_hub = ui_panel_create(parent, LV_PCT(100), LV_PCT(100));
    lv_obj_t *hd = ui_label_create(g_hub, "游戏中心", UI_FONT_M, UI_COL_INK);
    lv_obj_align(hd, LV_ALIGN_TOP_LEFT, 18, 16);

    static const struct { const char *sym, *name, *desc; uint32_t col; } G[4] = {
        { LV_SYMBOL_SHUFFLE,  "2048",   "滑动合并, 冲击 2048",       UI_COL_AMBER  },
        { LV_SYMBOL_OK,       "井字棋", "双人对战 · 三连即胜",        UI_COL_ACCENT },
        { LV_SYMBOL_PLUS,     "五子棋", "9 路棋盘 · 五连获胜",        UI_COL_GREEN  },
        { LV_SYMBOL_RIGHT,    "贪吃蛇", "经典街机 · 吃食物变长",      UI_COL_RED    },
    };
    const int cw = 480, ch = 190, x0 = 18, y0 = 60;
    for (int i = 0; i < 4; i++) {
        lv_obj_t *c = lv_btn_create(g_hub);
        lv_obj_set_size(c, cw, ch);
        lv_obj_set_pos(c, x0 + (i % 2) * (cw + 12), y0 + (i / 2) * (ch + 12));
        lv_obj_set_style_radius(c, 20, 0);
        lv_obj_set_style_shadow_width(c, 0, 0);
        lv_obj_set_style_bg_color(c, lv_color_hex(UI_COL_CARD), 0);
        lv_obj_set_style_bg_color(c, lv_color_hex(UI_COL_CARD2), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(c, lv_color_hex(UI_COL_RULE), 0);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ic = ui_label_create(c, G[i].sym, &lv_font_montserrat_28, G[i].col);
        lv_obj_align(ic, LV_ALIGN_TOP_LEFT, 20, 18);
        lv_obj_t *nm = ui_label_create(c, G[i].name, UI_FONT_M, UI_COL_INK);
        lv_obj_align(nm, LV_ALIGN_TOP_LEFT, 70, 24);
        lv_obj_t *ds = ui_label_create(c, G[i].desc, UI_FONT_S, UI_COL_MUTED);
        lv_obj_align(ds, LV_ALIGN_BOTTOM_LEFT, 70, -22);

        lv_obj_set_user_data(c, (void *)(intptr_t)i);
        lv_obj_add_event_cb(c, ui_event_game_open, LV_EVENT_CLICKED, NULL);
    }

    /* ---- 四个游戏层 ---- */
    static const char *titles[UI_GAME_NUM] = {
        "2048", "井字棋", "五子棋", "贪吃蛇",
    };
    void (*builders[UI_GAME_NUM])(lv_obj_t *) = {
        build_2048, build_ttt, build_gomo, build_snake,
    };
    for (int i = 0; i < UI_GAME_NUM; i++) {
        g_wrap[i] = wrap_create(parent, i, titles[i]);
        builders[i](g_wrap[i]);
    }
    ui_2048_reset();
    ui_ttt_reset();
    ui_gomo_reset();
}
