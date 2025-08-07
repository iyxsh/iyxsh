#ifndef JSON_CONFIG_H
#define JSON_CONFIG_H
#include <stddef.h>
#include "../cJSON/cJSON.h"
// 初始化配置系统
int json_config_init(const char* config_file_path);
// 获取字符串配置值
const char* json_config_get_string(const char* key);
// 释放配置资源
void json_config_free();
// 获取数值配置（int类型）
int json_config_get_int(const char* key);

// 获取文件中的配置项
cJSON* gridcircleItem(const char* key);

#endif