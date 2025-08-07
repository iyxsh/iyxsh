#include "json_config.h"
#include <stdio.h>
#include <stdlib.h>
static cJSON* config_json = NULL;

int json_config_init(const char* config_file_path) {
    // 如果没有提供配置文件路径，则使用默认路径
    const char* config_path = config_file_path ? config_file_path : "config.json";
    
    FILE* fp = fopen(config_path, "rb");
    if (!fp) {
        // 尝试在上级目录中查找配置文件
        fp = fopen("../bin/config.json", "rb");
        if (!fp) {
            perror("配置文件打开失败");
            return -1;
        }
    }

    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fclose(fp);
        return -1;
    }

    fread(buffer, 1, length, fp);
    buffer[length] = '\0';
    fclose(fp);

    config_json = cJSON_Parse(buffer);
    free(buffer);

    if (!config_json) {
        fprintf(stderr, "JSON解析错误: %s\n", cJSON_GetErrorPtr());
    }
    
    return 0;
}

const char* json_config_get_string(const char* key) {
    if (!config_json) return NULL;
    
    cJSON* item = cJSON_GetObjectItem(config_json, key);
    if (!item || !cJSON_IsString(item)) {
        return NULL;
    }
    return item->valuestring;
}

int json_config_get_int(const char* key) {
    if (!config_json) return 0;
    
    cJSON* item = cJSON_GetObjectItem(config_json, key);
    return item->valueint;
}

cJSON* gridcircleItem(const char* key)
{
    if (!config_json) return NULL;   
    return cJSON_GetObjectItem(config_json, key);
}


void json_config_free() {
    if (config_json) {
        cJSON_Delete(config_json);
        config_json = NULL;
    }
}