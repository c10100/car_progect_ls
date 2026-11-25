#include "common.h"
#include "core/msg_queue.h"
#include <sys/time.h>

int msg_queue_init(msg_queue_t *q, int capacity)
{
    if (!q || capacity <= 0) return ERR;
    q->buf = calloc(capacity, sizeof(void *));
    if (!q->buf) return ERR;
    q->cap   = capacity;
    q->head  = 0;
    q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    return OK;
}

void msg_queue_destroy(msg_queue_t *q)
{
    if (!q) return;
    /* 释放残留消息 */
    while (q->count > 0) {
        void *m = msg_queue_pop(q, 0);
        free(m);
    }
    free(q->buf);
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
}

int msg_queue_push(msg_queue_t *q, void *msg)
{
    if (!q || !msg) return ERR;

    pthread_mutex_lock(&q->lock);
    if (q->count == q->cap) {
        /* 队列满: 覆盖最旧的一条, 保证最新数据(如天气)优先展示 */
        free(q->buf[q->head]);
        q->head = (q->head + 1) % q->cap;
        q->count--;
        LOGW("queue full, drop oldest msg");
    }
    q->buf[(q->head + q->count) % q->cap] = msg;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
    return OK;
}

void *msg_queue_pop(msg_queue_t *q, int timeout_ms)
{
    void *msg = NULL;
    struct timespec ts;

    pthread_mutex_lock(&q->lock);
    if (q->count == 0 && timeout_ms != 0) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec  += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec  += 1;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&q->not_empty, &q->lock, &ts);
    }
    if (q->count > 0) {
        msg = q->buf[q->head];
        q->head = (q->head + 1) % q->cap;
        q->count--;
    }
    pthread_mutex_unlock(&q->lock);
    return msg;
}

int msg_queue_count(msg_queue_t *q)
{
    int c;
    pthread_mutex_lock(&q->lock);
    c = q->count;
    pthread_mutex_unlock(&q->lock);
    return c;
}
