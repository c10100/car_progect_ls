#include "common.h"
#include "net/net_utils.h"
#include <fcntl.h>
#include <netdb.h>

/* ==================== TCP ==================== */

int tcp_connect(const char *ip, int port)
{
    int fd;
    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        LOGE("socket: %s", strerror(errno));
        return ERR;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        /* 支持域名: 天气/AI 的 API 常给域名 */
        struct hostent *he = gethostbyname(ip);
        if (!he) {
            LOGE("resolve %s failed", ip);
            close(fd);
            return ERR;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOGE("connect %s:%d: %s", ip, port, strerror(errno));
        close(fd);
        return ERR;
    }

    LOGD("tcp connected %s:%d (fd=%d)", ip, port, fd);
    return fd;
}

int tcp_send_all(int fd, const void *buf, int len)
{
    const char *p = buf;
    int sent = 0;

    while (sent < len) {
        int n = send(fd, p + sent, len - sent, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            LOGE("send: %s", strerror(errno));
            return ERR;
        }
        sent += n;
    }
    return sent;
}

int tcp_recv_all(int fd, void *buf, int len)
{
    char  *p = buf;
    int    got = 0;

    /* 核心: TCP 是字节流, 必须 recv 到收满一条完整消息为止 */
    while (got < len) {
        int n = recv(fd, p + got, len - got, 0);
        if (n == 0) {                       /* 对端关闭 */
            LOGD("peer closed");
            return 0;
        }
        if (n < 0) {
            if (errno == EINTR) continue;
            LOGE("recv: %s", strerror(errno));
            return ERR;
        }
        got += n;
    }
    return got;
}

void net_close(int fd)
{
    if (fd >= 0) close(fd);
}

/* ==================== UDP ==================== */

int udp_create(int port)
{
    int fd;
    struct sockaddr_in addr;
    int reuse = 1;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        LOGE("udp socket: %s", strerror(errno));
        return ERR;
    }
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOGE("udp bind %d: %s", port, strerror(errno));
        close(fd);
        return ERR;
    }
    return fd;
}

int udp_send_to(int fd, const char *ip, int port, const void *buf, int len)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    return sendto(fd, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
}

int net_set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return ERR;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
