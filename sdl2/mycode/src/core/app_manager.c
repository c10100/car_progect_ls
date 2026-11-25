#include "common.h"
#include "core/app_manager.h"

#define UI_QUEUE_CAP 16

static msg_queue_t  g_ui_queue;
static app_msg_cb_t g_cbs[APP_MSG_MAX];

int app_manager_init(void)
{
    memset(g_cbs, 0, sizeof(g_cbs));
    return msg_queue_init(&g_ui_queue, UI_QUEUE_CAP);
}

int app_manager_post(app_msg_type_e type, const void *data, int len)
{
    app_msg_t *m;

    if (type < 0 || type >= APP_MSG_MAX) return ERR;

    m = malloc(sizeof(app_msg_t));
    if (!m) return ERR;
    m->type = type;

    if (data && len > 0) {
        m->data = malloc(len);
        if (!m->data) { free(m); return ERR; }
        memcpy(m->data, data, len);
        m->len = len;
    } else {
        m->data = NULL;
        m->len  = 0;
    }

    return msg_queue_push(&g_ui_queue, m);
}

void app_manager_subscribe(app_msg_type_e type, app_msg_cb_t cb)
{
    if (type < 0 || type >= APP_MSG_MAX) return;
    g_cbs[type] = cb;   /* 后订阅覆盖先订阅: 只给当前页面 */
}

void app_manager_poll(void)
{
    app_msg_t *m;

    while ((m = msg_queue_pop(&g_ui_queue, 0)) != NULL) {
        if (g_cbs[m->type])
            g_cbs[m->type](m);       /* 回调里安全操作 LVGL */
        else
            LOGD("msg type=%d no subscriber, dropped", m->type);
        free(m->data);
        free(m);
    }
}
