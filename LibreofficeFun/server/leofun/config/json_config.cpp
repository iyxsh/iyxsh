#include "json_config.h"
#include <stdio.h>
#include <stdlib.h>
static cJSON* config_json = NULL;

void json_config_init(const char* config_file_path) {
    FILE* fp = fopen(config_file_path, "rb");
    if (!fp) {
        perror("配置文件打开失败");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fclose(fp);
        return;
    }

    fread(buffer, 1, length, fp);
    buffer[length] = '\0';
    fclose(fp);

    config_json = cJSON_Parse(buffer);
    free(buffer);

    if (!config_json) {
        fprintf(stderr, "JSON解析错误: %s\n", cJSON_GetErrorPtr());
    }
}

const char* json_config_get_string(const char* key) {
    if (!config_json) return NULL;
    
    cJSON* item = cJSON_GetObjectItem(config_json, key);
    char* value = cJSON_Print(item);
    printf("key:%s,value:%s\n",key,value);
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