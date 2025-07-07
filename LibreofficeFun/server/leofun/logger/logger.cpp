#include "logger.h"
#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include "../config/json_config.h"
#define MAX_LOG_SIZE 10 * 1024 * 1024  // 默认10MB
#define MAX_BACKUPS 30                 // 默认保留30个备份

static FILE* log_file = NULL;
static char* log_path = NULL;
static size_t max_log_size = MAX_LOG_SIZE;
static int max_backups = MAX_BACKUPS;

// 新增备份文件过滤函数
static int filter_backup(const struct dirent *entry) {
    if (!log_path) return 0;
    
    const char *base = strrchr(log_path, '/');
    base = base ? base + 1 : log_path;
    
    // 匹配格式：原文件名_YYYYMMDDHH.log
    char prefix[256];
    snprintf(prefix, sizeof(prefix), "%s_", base);
    
    return strncmp(entry->d_name, prefix, strlen(prefix)) == 0 &&
           strlen(entry->d_name) == strlen(prefix) + 12; // 12 = 10时间字符 + .log
}

// 新增备份文件排序函数
static int sort_backup(const struct dirent **a, const struct dirent **b) {
    // 提取时间戳部分进行比较：原文件名_2024060512.log -> 2024060512
    long ts1 = strtol((*a)->d_name + strlen((*a)->d_name) - 14, NULL, 10);
    long ts2 = strtol((*b)->d_name + strlen((*b)->d_name) - 14, NULL, 10);
    return ts2 - ts1; // 降序排列（最新在前）
}

// 新增备份创建函数
static void create_backup() {
    if (!log_path) return;
    
    // 获取当前时间
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H", tm);
    
    // 构造备份文件名
    char backup_path[512];
    snprintf(backup_path, sizeof(backup_path), "%s_%s.log", 
            log_path, timestamp);
    
    // 关闭当前日志文件
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    
    // 重命名文件
    if (rename(log_path, backup_path) != 0) {
        perror("日志备份失败");
    }
    
    // 重新打开日志文件
    log_file = fopen(log_path, "a");


    // 新增备份轮转逻辑
    if (max_backups > 0) {
        struct dirent **namelist;
        char path[512];
        snprintf(path, sizeof(path), "%s", log_path);
        char *last_slash = strrchr(path, '/');
        
        // 获取日志目录路径
        if (last_slash) {
            *last_slash = '\0';
        } else {
            strcpy(path, ".");
        }
        
        // 扫描备份文件
        int n = scandir(path, &namelist, filter_backup, sort_backup);
        if (n < 0) {
            perror("扫描备份目录失败");
            return;
        }
        
        // 删除超出数量的旧备份
        for (int i = max_backups; i < n; i++) {
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, namelist[i]->d_name);
            
            if (remove(fullpath) == 0) {
                fprintf(log_file, "[CLEANUP] Removed old backup: %s\n", namelist[i]->d_name);
            } else {
                fprintf(log_file, "[ERROR] Failed to remove %s: %s\n", 
                       namelist[i]->d_name, strerror(errno));
            }
            free(namelist[i]);
        }
        free(namelist);
    }
}


void logger_init(const char* path) {
    // 从配置读取参数（需要先在配置文件中定义）
    max_log_size = json_config_get_int("log_max_size") * 1024 * 1024;
    max_backups = json_config_get_int("log_max_backups");
    
    // 自动创建日志文件
    log_file = fopen(path, "a");
    if (!log_file) {
        perror("无法创建日志文件");
        exit(EXIT_FAILURE);
    }
    
    // 设置文件缓冲区
    setvbuf(log_file, NULL, _IOLBF, BUFSIZ);
    log_path = strdup(path);
    
    // 首次检查日志大小
    struct stat st;
    if (stat(path, &st) == 0) {
        if (st.st_size > max_log_size) {
            create_backup();
        }
    }
}

void logger_log(const char* message,...) {
    if (!log_file) return;
    
    // 检查日志大小
    struct stat st;
    if (stat(log_path, &st) == 0 && st.st_size > max_log_size) {
        create_backup(); // 该函数现在包含完整的轮转逻辑
    }

    // 获取可变参数
    va_list args;
    va_start(args, message);
    
    // 生成时间戳
    time_t now = time(NULL);
    char timestr[20];
    struct tm *tm_info = localtime(&now);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", tm_info);

    // 带格式化的日志输出
    fprintf(log_file, "[%s] ", timestr);
    vfprintf(log_file, message, args);  // 关键修改：使用vfprintf处理格式化参数
    fprintf(log_file, "\n");  // 保证换行
    
    va_end(args);
}

void logger_close() {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}
