#include "common.h"
#include "core/app_manager.h"
#include "core/config.h"
#include "net/http_client.h"
#include "task/task_weather.h"
#include "cJSON.h"

#define WEATHER_PERIOD_MIN  30   /* 自动刷新周期(分钟) */

static pthread_t g_tid;
static volatile int g_refresh_flag = 0;

/*
 * cJSON 解析天气 JSON
 *
 * 以 OpenWeatherMap 免费接口为例 (支持 http):
 *   http://api.openweathermap.org/data/2.5/weather?q=Shenzhen&appid=你的KEY&units=metric&lang=zh_cn
 *   {
 *     "name": "Shenzhen",
 *     "weather": [ {"main": "Clouds", "description": "多云"} ],
 *     "main": { "temp": 25.3, "temp_min": 22, "temp_max": 28, "humidity": 70 }
 *   }
 *
 * 若老师提供其他接口(心知天气等), 只需改这个函数里的 key 路径, 框架不变.
 */
int weather_parse_json(const char *json, weather_data_t *out)
{
    cJSON *root, *weather_arr, *weather0, *main_obj;
    cJSON *item;

    root = cJSON_Parse(json);
    if (!root) {
        LOGE("cJSON_Parse failed: %s", cJSON_GetErrorPtr() ? cJSON_GetErrorPtr() : "(null)");
        return ERR;
    }

    memset(out, 0, sizeof(*out));

    /* 城市名 */
    item = cJSON_GetObjectItem(root, "name");
    if (cJSON_IsString(item))
        strncpy(out->city, item->valuestring, CITY_LEN - 1);

    /* 天气现象: weather 是数组, 取第一个 */
    weather_arr = cJSON_GetObjectItem(root, "weather");
    weather0 = cJSON_IsArray(weather_arr) ? cJSON_GetArrayItem(weather_arr, 0) : NULL;
    if (weather0) {
        item = cJSON_GetObjectItem(weather0, "description");
        if (cJSON_IsString(item))
            strncpy(out->weather, item->valuestring, WSTR_LEN - 1);
    }

    /* 温湿度: main 对象 */
    main_obj = cJSON_GetObjectItem(root, "main");
    if (cJSON_IsObject(main_obj)) {
        item = cJSON_GetObjectItem(main_obj, "temp");
        if (cJSON_IsNumber(item)) out->temp = (int)(item->valuedouble + 0.5);
        item = cJSON_GetObjectItem(main_obj, "temp_min");
        if (cJSON_IsNumber(item)) out->temp_min = (int)(item->valuedouble + 0.5);
        item = cJSON_GetObjectItem(main_obj, "temp_max");
        if (cJSON_IsNumber(item)) out->temp_max = (int)(item->valuedouble + 0.5);
        item = cJSON_GetObjectItem(main_obj, "humidity");
        if (cJSON_IsNumber(item)) out->humidity = item->valueint;
    }

    cJSON_Delete(root);
    return OK;
}

/* 抓取一次天气并投递到 UI 队列 */
static int weather_fetch_once(void)
{
    char url[256];
    char city[CITY_LEN];
    char key[64];
    http_resp_t resp;
    weather_data_t data;

    config_get_str(CFG_CITY, "Shenzhen", city, sizeof(city));
    config_get_str(CFG_WEA_KEY, "", key, sizeof(key));
    if (key[0] == '\0') {
        LOGW("weather_key empty, set it in config first");
        return ERR;
    }

    snprintf(url, sizeof(url),
             "http://api.openweathermap.org/data/2.5/weather"
             "?q=%s&appid=%s&units=metric&lang=zh_cn",
             city, key);

    if (http_get(url, &resp) != OK || !resp.body) {
        LOGE("weather http failed");
        http_resp_free(&resp);
        return ERR;
    }

    if (weather_parse_json(resp.body, &data) != OK) {
        LOGE("weather json parse failed");
        http_resp_free(&resp);
        return ERR;
    }
    http_resp_free(&resp);

    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(data.update_time, sizeof(data.update_time), "%H:%M", &tm);

    /* 关键一步: 后台线程不碰 UI, 只投消息 */
    app_manager_post(APP_MSG_WEATHER, &data, sizeof(data));
    LOGI("weather posted: %s %s %dC", data.city, data.weather, data.temp);
    return OK;
}

static void *weather_thread(void *arg)
{
    (void)arg;
    /* 上电先刷一次 */
    weather_fetch_once();

    while (1) {
        /* 每 30 秒醒一次, 检查是否到了刷新周期 / 是否有手动刷新 */
        for (int i = 0; i < WEATHER_PERIOD_MIN * 2; i++) {
            sleep(30);
            if (g_refresh_flag) {
                g_refresh_flag = 0;
                break;
            }
        }
        weather_fetch_once();
    }
    return NULL;
}

int task_weather_start(void)
{
    if (pthread_create(&g_tid, NULL, weather_thread, NULL) != 0) {
        LOGE("create weather thread failed");
        return ERR;
    }
    pthread_detach(g_tid);
    LOGI("weather thread started");
    return OK;
}

int task_weather_refresh(void)
{
    g_refresh_flag = 1;
    return OK;
}
