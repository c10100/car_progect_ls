#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

/*
 * http_client.h - 轻量 HTTP 客户端 (裸 socket 实现, 体现网络编程)
 *
 * 用途:
 *   1. 天气预报:  HTTP GET  -> 返回 JSON -> cJSON 解析
 *   2. AI 聊天:   HTTP POST(JSON body) -> 返回 JSON -> cJSON 解析
 *
 * 限制:
 *   仅支持 http:// 。若接口强制 https(如 DeepSeek/智谱), 有两种方案:
 *     a) 用学校/老师提供的中转 http 服务器 (推荐, 教学常见)
 *     b) 在 net 层加 openssl/mbedtls, 把 tcp_connect 换成 SSL_connect (TODO选做)
 */

#include <stddef.h>

typedef struct {
    int     status_code;      /* 200 / 404 / 500 ... */
    char   *body;             /* 响应体(malloc, 调用者负责 free) */
    int     body_len;
} http_resp_t;

/*
 * 通用请求:
 *   method       "GET" / "POST"
 *   url          "http://host:port/path?query"  (host 支持域名)
 *   headers      附加头数组, 如 {"Content-Type: application/json"} 可为 NULL
 *   body         POST 体, GET 传 NULL
 *   resp         出参, 成功后 resp->body 需 free
 * 返回: OK / ERR
 */
int http_request(const char *method, const char *url,
                 const char **headers, int hdr_cnt,
                 const char *body,
                 http_resp_t *resp);

/* 便捷封装 */
int http_get (const char *url, http_resp_t *resp);
int http_post_json(const char *url, const char *json_body, http_resp_t *resp);

/* 释放响应体 */
void http_resp_free(http_resp_t *resp);

#endif /* __HTTP_CLIENT_H__ */
