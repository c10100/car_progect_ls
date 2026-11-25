#ifndef __NET_UTILS_H__
#define __NET_UTILS_H__

/*
 * net_utils.h - socket 封装 (网络编程基础件)
 * 聊天室(TCP) / 联机游戏(UDP) / HTTP 都基于这里的函数。
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* ---------- TCP 客户端 ---------- */
/* 连接服务器(阻塞), 成功返回 sockfd */
int  tcp_connect(const char *ip, int port);

/* 发全部: 处理 send 半包, 成功返回发送字节数 */
int  tcp_send_all(int fd, const void *buf, int len);

/* 收整条定长消息: 循环 recv 直到收满 len 字节(TCP粘包/半包处理核心) */
int  tcp_recv_all(int fd, void *buf, int len);

/* 关闭 */
void net_close(int fd);

/* ---------- UDP ---------- */
/* 创建 UDP socket 并绑定到 port(0=系统分配), 用于收数据 */
int  udp_create(int port);

/* 发送到指定 ip:port */
int  udp_send_to(int fd, const char *ip, int port, const void *buf, int len);

/* 设置 socket 非阻塞 (游戏线程轮询用) */
int  net_set_nonblock(int fd);

#endif /* __NET_UTILS_H__ */
