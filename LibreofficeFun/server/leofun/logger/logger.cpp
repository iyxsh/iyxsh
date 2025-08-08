#include "logger.h"
#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "../config/json_config.h"

#define DEFAULT_LOG_SIZE 10 * 1024 * 1024  // 默认10MB
#define DEFAULT_BACKUPS 30                 // 默认保留30个备份

// 日志系统状态结构体
typedef struct {
    FILE* file;
    char* path;
    size_t max_size;
    int max_backups;
    LogLevel level;
    pthread_mutex_t mutex;
    int initialized;
} LoggerState;

// 全局日志状态
static LoggerState g_logger = {
    .file = NULL,
    .path = NULL,
    .max_size = DEFAULT_LOG_SIZE,
    .max_backups = DEFAULT_BACKUPS,
    .level = LOG_LEVEL_INFO,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .initialized = 0
};

// 日志级别名称
static const char* log_level_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR"
};

// 内部函数声明
static int create_log_file();
static void close_log_file();
static int check_log_size();
static void create_backup();
static int filter_backup(const struct dirent *entry);
static int sort_backup(const struct dirent **a, const struct dirent **b);

// 创建日志文件
static int create_log_file() {
    if (g_logger.path) {
        g_logger.file = fopen(g_logger.path, "a");
        if (!g_logger.file) {
            perror("无法创建日志文件");
            return -1;
        }
        // 设置行缓冲
        setvbuf(g_logger.file, NULL, _IOLBF, BUFSIZ);
        return 0;
    }
    return -1;
}

// 关闭日志文件
static void close_log_file() {
    if (g_logger.file) {
        fflush(g_logger.file);
        fclose(g_logger.file);
        g_logger.file = NULL;
    }
}

// 检查日志文件大小
static int check_log_size() {
    if (!g_logger.path) return 0;
    
    struct stat st;
    if (stat(g_logger.path, &st) == 0 && st.st_size > g_logger.max_size) {
        return 1;
    }
    return 0;
}

// 备份文件过滤函数
static int filter_backup(const struct dirent *entry) {
    if (!g_logger.path) return 0;
    
    const char *base = strrchr(g_logger.path, '/');
    base = base ? base + 1 : g_logger.path;
    
    // 匹配格式：原文件名_YYYYMMDDHH.log
    char prefix[256];
    snprintf(prefix, sizeof(prefix), "%s_", base);
    
    return strncmp(entry->d_name, prefix, strlen(prefix)) == 0 &&
           strlen(entry->d_name) == strlen(prefix) + 12; // 12 = 10时间字符 + .log
}

// 备份文件排序函数
static int sort_backup(const struct dirent **a, const struct dirent **b) {
    // 提取时间戳部分进行比较：原文件名_2024060512.log -> 2024060512
    long ts1 = strtol((*a)->d_name + strlen((*a)->d_name) - 14, NULL, 10);
    long ts2 = strtol((*b)->d_name + strlen((*b)->d_name) - 14, NULL, 10);
    return ts2 - ts1; // 降序排列（最新在前）
}

// 创建备份
static void create_backup() {
    if (!g_logger.path) return;
    
    // 获取当前时间
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H", tm);
    
    // 构造备份文件名
    char backup_path[512];
    snprintf(backup_path, sizeof(backup_path), "%s_%s.log", 
            g_logger.path, timestamp);
    
    // 关闭当前日志文件
    close_log_file();
    
    // 重命名文件
    if (rename(g_logger.path, backup_path) != 0) {
        // 即使重命名失败，也要尝试重新打开日志文件
        perror("日志备份失败");
    }
    
    // 重新打开日志文件
    if (create_log_file() != 0) {
        perror("重新打开日志文件失败");
    }

    // 备份轮转逻辑
    if (g_logger.max_backups > 0) {
        struct dirent **namelist;
        char path[512];
        snprintf(path, sizeof(path), "%s", g_logger.path);
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
            // 扫描失败，不处理备份文件
            return;
        }
        
        // 删除超出数量的旧备份
        for (int i = g_logger.max_backups; i < n; i++) {
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, namelist[i]->d_name);
            remove(fullpath);
            free(namelist[i]);
        }
        free(namelist);
    }
}

// 初始化日志系统（带配置）
int logger_init_with_config(const LoggerConfig* config) {
    if (!config) return -1;
    
    pthread_mutex_lock(&g_logger.mutex);
    
    // 设置配置
    g_logger.level = config->level;
    g_logger.max_size = config->max_size > 0 ? config->max_size : DEFAULT_LOG_SIZE;
    g_logger.max_backups = config->max_backups > 0 ? config->max_backups : DEFAULT_BACKUPS;
    
    // 释放旧路径
    if (g_logger.path) {
        free(g_logger.path);
        g_logger.path = NULL;
    }
    
    // 关闭旧文件
    close_log_file();
    
    // 设置新路径
    g_logger.path = strdup(config->file_path);
    if (!g_logger.path) {
        pthread_mutex_unlock(&g_logger.mutex);
        return -1;
    }
    
    // 创建日志文件
    if (create_log_file() != 0) {
        pthread_mutex_unlock(&g_logger.mutex);
        return -1;
    }
    
    g_logger.initialized = 1;
    pthread_mutex_unlock(&g_logger.mutex);
    return 0;
}

// 初始化日志系统（仅路径）
void logger_init(const char* path) {
    LoggerConfig config;
    config.level = LOG_LEVEL_INFO;
    config.max_size = DEFAULT_LOG_SIZE;
    config.max_backups = DEFAULT_BACKUPS;
    strncpy(config.file_path, path, sizeof(config.file_path) - 1);
    config.file_path[sizeof(config.file_path) - 1] = '\0';
    
    logger_init_with_config(&config);
}

// 设置日志级别
void logger_set_level(LogLevel level) {
    pthread_mutex_lock(&g_logger.mutex);
    g_logger.level = level;
    pthread_mutex_unlock(&g_logger.mutex);
}

// 获取当前日志级别
LogLevel logger_get_level() {
    pthread_mutex_lock(&g_logger.mutex);
    LogLevel level = g_logger.level;
    pthread_mutex_unlock(&g_logger.mutex);
    return level;
}

// 写入日志的核心函数
static void log_message(LogLevel level, const char* file, int line, const char* format, va_list args) {
    // 检查日志级别
    if (level < g_logger.level) {
        return;
    }
    
    pthread_mutex_lock(&g_logger.mutex);
    
    // 检查日志系统是否已初始化
    if (!g_logger.initialized) {
        pthread_mutex_unlock(&g_logger.mutex);
        return;
    }
    
    // 检查日志文件是否有效
    if (!g_logger.file && g_logger.path) {
        // 尝试重新打开日志文件
        if (create_log_file() != 0) {
            pthread_mutex_unlock(&g_logger.mutex);
            return;
        }
    }
    
    if (!g_logger.file) {
        pthread_mutex_unlock(&g_logger.mutex);
        return;
    }
    
    // 检查日志大小并轮转
    if (check_log_size()) {
        create_backup();
        // 检查重新打开是否成功
        if (!g_logger.file && g_logger.path) {
            if (create_log_file() != 0) {
                pthread_mutex_unlock(&g_logger.mutex);
                return;
            }
        }
        
        if (!g_logger.file) {
            pthread_mutex_unlock(&g_logger.mutex);
            return;
        }
    }

    // 生成时间戳
    time_t now = time(NULL);
    char timestr[20];
    struct tm *tm_info = localtime(&now);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // 写入日志前缀
    int result;
    if (file) {
        // 提取文件名（去掉路径）
        const char* filename = strrchr(file, '/');
        if (!filename) {
            filename = strrchr(file, '\\');
        }
        filename = filename ? filename + 1 : file;
        
        result = fprintf(g_logger.file, "[%s] [%s] [%s:%d] ", timestr, log_level_names[level], filename, line);
    } else {
        result = fprintf(g_logger.file, "[%s] [%s] ", timestr, log_level_names[level]);
    }
    
    if (result < 0) {
        // 写入失败，尝试刷新并重新写入
        fflush(g_logger.file);
        if (file) {
            const char* filename = strrchr(file, '/');
            if (!filename) {
                filename = strrchr(file, '\\');
            }
            filename = filename ? filename + 1 : file;
            result = fprintf(g_logger.file, "[%s] [%s] [%s:%d] ", timestr, log_level_names[level], filename, line);
        } else {
            result = fprintf(g_logger.file, "[%s] [%s] ", timestr, log_level_names[level]);
        }
    }
    
    // 写入日志内容
    if (result >= 0) {
        result = vfprintf(g_logger.file, format, args);
        if (result < 0) {
            fflush(g_logger.file);
            result = vfprintf(g_logger.file, format, args);
        }
    }
    
    // 写入换行符
    if (result >= 0) {
        result = fprintf(g_logger.file, "\n");
        if (result < 0) {
            fflush(g_logger.file);
            result = fprintf(g_logger.file, "\n");
        }
    }
    
    // 确保日志立即写入
    fflush(g_logger.file);
    
    // 检查写入是否成功
    if (result < 0) {
        // 如果写入失败，输出到标准错误
        if (file) {
            const char* filename = strrchr(file, '/');
            if (!filename) {
                filename = strrchr(file, '\\');
            }
            filename = filename ? filename + 1 : file;
            fprintf(stderr, "[%s] [%s] [%s:%d] ", timestr, log_level_names[level], filename, line);
        } else {
            fprintf(stderr, "[%s] [%s] ", timestr, log_level_names[level]);
        }
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
    }
    
    pthread_mutex_unlock(&g_logger.mutex);
}

// 记录日志信息
void logger_log(LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(level, NULL, 0, format, args);
    va_end(args);
}

// 带文件名和行号的日志记录
void logger_log_with_location(LogLevel level, const char* file, int line, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(level, file, line, format, args);
    va_end(args);
}

// 强制刷新日志缓冲区
void logger_flush() {
    pthread_mutex_lock(&g_logger.mutex);
    if (g_logger.file) {
        fflush(g_logger.file);
    }
    pthread_mutex_unlock(&g_logger.mutex);
}

// 关闭日志系统
void logger_close() {
    pthread_mutex_lock(&g_logger.mutex);
    if (g_logger.initialized) {
        close_log_file();
        
        if (g_logger.path) {
            free(g_logger.path);
            g_logger.path = NULL;
        }
        
        g_logger.initialized = 0;
    }
    pthread_mutex_unlock(&g_logger.mutex);
}