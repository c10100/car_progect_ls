#include "common.h"
#include "net/net_utils.h"
#include "net/http_client.h"

#define RECV_BUF_SIZE   8192
#define HTTP_TIMEOUT_S  10

/*
 * 解析 "http://host[:port]/path" -> host / port / path
 * 返回 OK / ERR
 */
static int parse_url(const char *url, char *host, int host_len,
                     int *port, char *path, int path_len)
{
    const char *p, *slash, *colon;

    if (strncmp(url, "http://", 7) != 0) {
        LOGE("only http:// supported, got: %s", url);
        return ERR;
    }
    p = url + 7;                              /* 跳过 http:// */

    slash = strchr(p, '/');
    colon = strchr(p, ':');

    if (colon && (!slash || colon < slash)) { /* 有端口号 */
        int n = colon - p;
        if (n >= host_len) return ERR;
        memcpy(host, p, n); host[n] = '\0';
        *port = atoi(colon + 1);
    } else {                                  /* 默认 80 */
        int n = slash ? slash - p : (int)strlen(p);
        if (n >= host_len) return ERR;
        memcpy(host, p, n); host[n] = '\0';
        *port = 80;
    }

    if (slash)
        strncpy(path, slash, path_len - 1);
    else
        strncpy(path, "/", path_len - 1);

    return OK;
}

/* 从响应头里找 Content-Length */
static int get_content_length(const char *hdr)
{
    char *p = strcasestr(hdr, "Content-Length:");
    if (!p) return -1;
    return atoi(p + strlen("Content-Length:"));
}

int http_request(const char *method, const char *url,
                 const char **headers, int hdr_cnt,
                 const char *body,
                 http_resp_t *resp)
{
    char host[128], path[512], req[4096];
    int  port, fd, total = 0, content_len = -1;
    char *raw = NULL, *hdr_end;
    int  raw_cap = RECV_BUF_SIZE;

    memset(resp, 0, sizeof(*resp));
    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) != OK)
        return ERR;

    /* 1. 拼请求报文 (体现 HTTP 协议格式) */
    int n = snprintf(req, sizeof(req),
            "%s %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: carpad/1.0\r\n"
            "Accept: */*\r\n"
            "Connection: close\r\n",      /* 用短连接: 读完即关, 免去保活 */
            method, path, host);
    for (int i = 0; i < hdr_cnt && headers; i++)
        n += snprintf(req + n, sizeof(req) - n, "%s\r\n", headers[i]);
    if (body)
        n += snprintf(req + n, sizeof(req) - n,
                      "Content-Type: application/json\r\n"
                      "Content-Length: %d\r\n\r\n%s",
                      (int)strlen(body), body);
    else
        n += snprintf(req + n, sizeof(req) - n, "\r\n");

    /* 2. 连接 + 发送 */
    fd = tcp_connect(host, port);
    if (fd < 0) return ERR;
    if (tcp_send_all(fd, req, n) < 0) { net_close(fd); return ERR; }
    LOGD(">> %s %s:%d%s", method, host, port, path);

    /* 3. 收响应 (缓冲区不够就翻倍扩容) */
    raw = malloc(raw_cap);
    if (!raw) { net_close(fd); return ERR; }

    struct timeval tv = { HTTP_TIMEOUT_S, 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (1) {
        if (total == raw_cap) {
            raw_cap *= 2;
            char *tmp = realloc(raw, raw_cap);
            if (!tmp) break;
            raw = tmp;
        }
        int r = recv(fd, raw + total, raw_cap - total, 0);
        if (r <= 0) break;
        total += r;

        /* 已拿到头且算出 Content-Length 且收满 -> 提前结束 */
        raw[total] = '\0';
        hdr_end = strstr(raw, "\r\n\r\n");
        if (hdr_end && content_len < 0) {
            *hdr_end = '\0';
            content_len = get_content_length(raw);
            *hdr_end = '\r';
            if (content_len >= 0 &&
                total >= (int)(hdr_end - raw) + 4 + content_len)
                break;
        }
    }
    net_close(fd);
    raw[total] = '\0';

    /* 4. 分离 状态行/响应头 与 响应体 */
    hdr_end = strstr(raw, "\r\n\r\n");
    if (!hdr_end) { free(raw); LOGE("bad http response"); return ERR; }
    *hdr_end = '\0';
    resp->body = strdup(hdr_end + 4);
    resp->body_len = total - (int)(hdr_end - raw) - 4;

    resp->status_code = (raw[9] - '0') * 100 + (raw[10] - '0') * 10 + (raw[11] - '0');
    LOGD("<< HTTP %d, body %d bytes", resp->status_code, resp->body_len);

    free(raw);
    return (resp->status_code == 200) ? OK : ERR;
}

int http_get(const char *url, http_resp_t *resp)
{
    return http_request("GET", url, NULL, 0, NULL, resp);
}

int http_post_json(const char *url, const char *json_body, http_resp_t *resp)
{
    const char *hdrs[] = { "Authorization: Bearer", /* 占位: 需要鉴权时在 url 处带 key 或自行扩展 */
                           "Accept: application/json" };
    return http_request("POST", url, hdrs, 2, json_body, resp);
}

void http_resp_free(http_resp_t *resp)
{
    if (resp && resp->body) {
        free(resp->body);
        resp->body = NULL;
    }
}
