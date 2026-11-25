#ifndef __MSG_QUEUE_H__
#define __MSG_QUEUE_H__

/*
 * msg_queue.h - 线程安全循环队列 (数据结构 + 系统编程)
 *
 * 用途: 后台线程(网络收数据) -> 主线程(LVGL刷新) 之间的数据搬运。
 * 原则: LVGL 绝不允许被子线程直接调用, 子线程只往队列塞数据,
 *       主循环 lv_timer_handler() 之后轮询队列再刷新 UI。
 *
 * 实现: 环形缓冲区 + pthread_mutex + pthread_cond
 */

#include <pthread.h>

typedef struct {
    void          **buf;        /* 存放消息指针的环形数组 */
    int             cap;        /* 容量 */
    int             head;       /* 队头下标(出队位置) */
    int             count;      /* 当前元素个数 */
    pthread_mutex_t lock;       /* 互斥锁 */
    pthread_cond_t  not_empty;  /* 条件变量: 通知消费者 */
} msg_queue_t;

/* 初始化/销毁 */
int msg_queue_init(msg_queue_t *q, int capacity);
void msg_queue_destroy(msg_queue_t *q);

/* 入队: 队列满时丢弃最旧消息(实时性优先), msg 的内存由队列接管 */
int msg_queue_push(msg_queue_t *q, void *msg);

/* 出队: timeout_ms < 0 阻塞等待; >= 0 超时返回 NULL */
void *msg_queue_pop(msg_queue_t *q, int timeout_ms);

/* 非阻塞探测是否有消息 */
int msg_queue_count(msg_queue_t *q);

#endif /* __MSG_QUEUE_H__ */
