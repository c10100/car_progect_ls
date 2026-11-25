#ifndef __TASK_WEATHER_H__
#define __TASK_WEATHER_H__

/*
 * task_weather.h - 天气预报 (必写项: HTTP + cJSON)
 *
 * 流程: 后台线程定时 HTTP GET 天气API -> cJSON 解析 -> post(APP_MSG_WEATHER)
 *       -> ui_weather 页面订阅回调刷新界面
 */

#define CITY_LEN   32
#define WSTR_LEN   32

typedef struct {
    char city[CITY_LEN];      /* 城市 */
    char weather[WSTR_LEN];   /* 天气现象: 晴/多云/小雨... */
    int  temp;                /* 温度 ℃ */
    int  temp_min;
    int  temp_max;
    int  humidity;            /* 湿度 % */
    char update_time[20];     /* 更新时间 HH:MM */
} weather_data_t;

/* 启动天气线程 (main 里调用) */
int  task_weather_start(void);

/* 手动刷新(如用户点击) */
int  task_weather_refresh(void);

/* 解析函数单独导出, 方便在 PC 上单元测试 */
int  weather_parse_json(const char *json, weather_data_t *out);

#endif /* __TASK_WEATHER_H__ */
