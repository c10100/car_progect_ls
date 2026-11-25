#ifndef __TASK_AI_H__
#define __TASK_AI_H__

/*
 * task_ai.h - AI 聊天 (必写项: HTTP + cJSON)
 *
 * 流程: 用户在 ui_ai 输入问题 -> ai_chat_async() 起临时工作线程
 *       -> 线程内 HTTP POST (cJSON 组包) -> cJSON 解析回复
 *       -> post(APP_MSG_AI_REPLY) -> ui_ai 刷新气泡
 *
 * 提问接口: ai_chat_async("你好"); 发后不管, 回复通过消息队列回来。
 * 历史对话上下文: g_ai_history 由本模块维护, 随请求一起发送 (TODO可选).
 */

#define AI_Q_LEN   1024
#define AI_R_LEN   4096

typedef struct {
    char question[AI_Q_LEN];
    char reply[AI_R_LEN];
    int  ok;                  /* 1=成功 0=失败(网络/密钥问题) */
} ai_data_t;

/* 异步提问, 立即返回 */
int ai_chat_async(const char *question);

/* 同步版本 (调试用, 会阻塞) */
int ai_chat_sync(const char *question, char *reply, int reply_len);

/* 解析函数单独导出, 方便单元测试 */
int ai_parse_reply(const char *json, char *out, int out_len);

#endif /* __TASK_AI_H__ */
