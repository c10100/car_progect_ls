#include "common.h"
#include "core/config.h"

#define MAX_ENTRY 32
#define KEY_LEN   32
#define VAL_LEN   256

typedef struct {
    char key[KEY_LEN];
    char val[VAL_LEN];
} cfg_entry_t;

static cfg_entry_t g_cfg[MAX_ENTRY];
static int         g_cfg_cnt = 0;
static char        g_cfg_path[128];

/* ---------- 内部: 查找/插入 ---------- */
static cfg_entry_t *cfg_find(const char *key)
{
    for (int i = 0; i < g_cfg_cnt; i++)
        if (strcmp(g_cfg[i].key, key) == 0)
            return &g_cfg[i];
    return NULL;
}

static cfg_entry_t *cfg_get_slot(const char *key)
{
    cfg_entry_t *e = cfg_find(key);
    if (e) return e;
    if (g_cfg_cnt >= MAX_ENTRY) return NULL;
    e = &g_cfg[g_cfg_cnt++];
    memset(e, 0, sizeof(*e));
    strncpy(e->key, key, KEY_LEN - 1);
    return e;
}

/* ---------- 文件IO: 读入 ---------- */
int config_init(const char *path)
{
    FILE *fp;
    char  line[KEY_LEN + VAL_LEN + 4];

    strncpy(g_cfg_path, path ? path : CFG_PATH_DEFAULT, sizeof(g_cfg_path) - 1);

    fp = fopen(g_cfg_path, "r");
    if (!fp) {
        LOGW("config file not found: %s (use default)", g_cfg_path);
        return ERR;   /* 第一次运行没有配置文件, 属正常 */
    }

    while (fgets(line, sizeof(line), fp)) {
        char *eq, *p;
        /* 去掉换行和注释 */
        p = strchr(line, '#');
        if (p) *p = '\0';
        if (line[0] == '\n' || line[0] == '\0') continue;

        eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        p = eq + 1;
        p[strcspn(p, "\r\n")] = '\0';

        cfg_entry_t *e = cfg_get_slot(line);
        if (e) strncpy(e->val, p, VAL_LEN - 1);
    }
    fclose(fp);

    LOGI("config loaded from %s, %d entries", g_cfg_path, g_cfg_cnt);
    return OK;
}

/* ---------- 读接口 ---------- */
int config_get_str(const char *key, const char *def, char *out, int out_len)
{
    cfg_entry_t *e = cfg_find(key);
    strncpy(out, (e && e->val[0]) ? e->val : def, out_len - 1);
    out[out_len - 1] = '\0';
    return OK;
}

int config_get_int(const char *key, int def)
{
    cfg_entry_t *e = cfg_find(key);
    return (e && e->val[0]) ? atoi(e->val) : def;
}

/* ---------- 写接口(内存) ---------- */
int config_set_str(const char *key, const char *val)
{
    cfg_entry_t *e = cfg_get_slot(key);
    if (!e) return ERR;
    strncpy(e->val, val, VAL_LEN - 1);
    return OK;
}

int config_set_int(const char *key, int val)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", val);
    return config_set_str(key, buf);
}

/* ---------- 文件IO: 写回 ---------- */
int config_save(void)
{
    FILE *fp = fopen(g_cfg_path, "w");
    if (!fp) {
        LOGE("open %s for write failed: %s", g_cfg_path, strerror(errno));
        return ERR;
    }
    for (int i = 0; i < g_cfg_cnt; i++)
        fprintf(fp, "%s=%s\n", g_cfg[i].key, g_cfg[i].val);
    fclose(fp);
    LOGI("config saved to %s", g_cfg_path);
    return OK;
}
