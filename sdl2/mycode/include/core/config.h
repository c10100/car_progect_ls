#ifndef __CONFIG_H__
#define __CONFIG_H__

/*
 * config.h - 配置模块 (文件IO)
 *
 * 负责把账号/密码/昵称/城市/服务器IP等持久化到板子上的文件,
 * 重启后无需重新设置。文件格式: 每行 "key=value" 的纯文本。
 *
 * 典型用法:
 *   config_init("/userdata/watch.cfg");
 *   char city[32]; config_get_str("city", "Shenzhen", city, sizeof(city));
 *   config_set_str("account", "zhangsan");
 *   config_save();
 */

#define CFG_PATH_DEFAULT  "/userdata/watch.cfg"   /* 板上路径, PC调试可改为 ./watch.cfg */

/* 配置项 key 列表 (约定优于配置) */
#define CFG_ACCOUNT   "account"     /* 登录账号 */
#define CFG_PASSWD    "passwd"      /* 密码 */
#define CFG_NICKNAME  "nickname"    /* 昵称 */
#define CFG_SIGN      "signature"   /* 个性签名 */
#define CFG_AVATAR    "avatar"      /* 头像路径 */
#define CFG_CITY      "city"        /* 天气城市 */
#define CFG_SRV_IP    "server_ip"   /* 聊天/游戏服务器 IP */
#define CFG_SRV_PORT  "server_port" /* 服务器端口 */
#define CFG_AI_KEY    "ai_key"      /* AI接口密钥 */
#define CFG_WEA_KEY   "weather_key" /* 天气接口密钥 */

int  config_init(const char *path);

/* 读取: 找不到返回默认值 def */
int  config_get_str(const char *key, const char *def, char *out, int out_len);
int  config_get_int(const char *key, int def);

/* 修改(只改内存), 最后统一 config_save() 写回文件 */
int  config_set_str(const char *key, const char *val);
int  config_set_int(const char *key, int val);

/* 写回文件 (fwrite + fclose, 体现文件IO) */
int  config_save(void);

#endif /* __CONFIG_H__ */
