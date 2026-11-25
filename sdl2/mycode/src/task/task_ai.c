#include "common.h"
#include "core/app_manager.h"
#include "core/config.h"
#include "net/http_client.h"
#include "task/task_ai.h"
#include "cJSON.h"

/*
 * 以 OpenAI 兼容接口为例 (DeepSeek / 智谱 / 通义 都兼容此格式):
 *   POST {endpoint}/chat/completions
 *   请求: {"model":"...","messages":[{"role":"user","content":"你好"}]}
 *   响应: {"choices":[{"message":{"role":"assistant","content":"..."}}]}
 *
 * 注意: 这类接口基本都是 https, 二选一:
 *   方案A(推荐): 用老师提供的中转 http 地址, 把 AI_API_URL 改成中转地址
 *   方案B(选做): http_client.c 里加 openssl, 把 tcp_connect 换 SSL_connect
 */
#define AI_API_URL  "http://your-middle-server:8080/v1/chat/completions" /* TODO: 换成实际地址 */
#define AI_MODEL    "deepseek-chat"                                       /* TODO: 按接口要求改 */

typedef struct {
    char question[AI_Q_LEN];
} ai_req_t;

/*
 * 组请求 JSON:
 * {"model":"deepseek-chat","messages":[{"role":"user","content":"问题"}]}
 */
static char *ai_build_request(const char *question)
{
    cJSON *root, *msgs, *m;
    char *str;

    root  = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", AI_MODEL);

    msgs = cJSON_AddArrayToObject(root, "messages");
    m = cJSON_CreateObject();
    cJSON_AddStringToObject(m, "role", "user");
    cJSON_AddStringToObject(m, "content", question);
    cJSON_AddItemToArray(msgs, m);

    str = cJSON_PrintUnformatted(root);   /* 堆上, 调用者 free */
    cJSON_Delete(root);
    return str;
}

int ai_parse_reply(const char *json, char *out, int out_len)
{
    cJSON *root, *choices, *c0, *msg;

    root = cJSON_Parse(json);
    if (!root) return ERR;

    choices = cJSON_GetObjectItem(root, "choices");
    c0 = cJSON_IsArray(choices) ? cJSON_GetArrayItem(choices, 0) : NULL;
    msg = c0 ? cJSON_GetObjectItem(c0, "message") : NULL;
    cJSON *content = msg ? cJSON_GetObjectItem(msg, "content") : NULL;

    if (!cJSON_IsString(content)) { cJSON_Delete(root); return ERR; }
    strncpy(out, content->valuestring, out_len - 1);
    out[out_len - 1] = '\0';

    cJSON_Delete(root);
    return OK;
}

static int ai_request_once(const char *question, ai_data_t *out)
{
    char *req_json;
    http_resp_t resp;
    int ret = ERR;

    memset(out, 0, sizeof(*out));
    strncpy(out->question, question, AI_Q_LEN - 1);

    req_json = ai_build_request(question);
    if (!req_json) return ERR;

    if (http_post_json(AI_API_URL, req_json, &resp) == OK && resp.body) {
        if (ai_parse_reply(resp.body, out->reply, AI_R_LEN) == OK) {
            out->ok = 1;
            ret = OK;
        }
    }
    http_resp_free(&resp);
    free(req_json);
    return ret;
}

static void *ai_worker(void *arg)
{
    ai_req_t *req = arg;
    ai_data_t data;

    ai_request_once(req->question, &data);

    /* 不管成败都投递, UI 负责展示失败提示 */
    app_manager_post(APP_MSG_AI_REPLY, &data, sizeof(data));
    free(req);
    return NULL;
}

int ai_chat_async(const char *question)
{
    ai_req_t *req = malloc(sizeof(ai_req_t));
    pthread_t tid;

    if (!req) return ERR;
    strncpy(req->question, question, AI_Q_LEN - 1);

    /* 每次提问起一个临时线程(分离), 不阻塞 UI */
    if (pthread_create(&tid, NULL, ai_worker, req) != 0) {
        free(req);
        return ERR;
    }
    pthread_detach(tid);
    return OK;
}

int ai_chat_sync(const char *question, char *reply, int reply_len)
{
    ai_data_t data;
    if (ai_request_once(question, &data) != OK) return ERR;
    strncpy(reply, data.reply, reply_len - 1);
    reply[reply_len - 1] = '\0';
    return OK;
}
