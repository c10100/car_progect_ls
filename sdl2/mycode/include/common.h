#ifndef __COMMON_H__
#define __COMMON_H__

/*
 * common.h - 工程公共头文件
 * 项目: 车载娱乐平板 (RK3568 + LVGL)
 * 包含: 日志宏 / 常用系统头文件
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

/* MinGW 没有 localtime_r, 提供等价 shim (Linux/板端不受影响) */
#ifdef _WIN32
static inline struct tm *localtime_r(const time_t *t, struct tm *r)
{
    return localtime_s(r, t) ? NULL : r;
}
#endif

/* ---------------- 日志宏 (服务器信息提示 / log 要求) ---------------- */
#define LOG_TAG  "carpad"

#define LOG(level, fmt, ...) \
    do { \
        time_t _t = time(NULL); \
        struct tm _tm; \
        localtime_r(&_t, &_tm); \
        printf("[%c][%s][%02d:%02d:%02d][%s:%d] " fmt "\n", \
               level, LOG_TAG, _tm.tm_hour, _tm.tm_min, _tm.tm_sec, \
               __func__, __LINE__, ##__VA_ARGS__); \
        fflush(stdout); \
    } while (0)

#define LOGD(fmt, ...) LOG('D', fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG('I', fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG('W', fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOG('E', fmt, ##__VA_ARGS__)

/* ---------------- 通用返回值 ---------------- */
#define OK    0
#define ERR  -1

/* ---------------- 简单工具 ---------------- */
#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))

#endif /* __COMMON_H__ */
