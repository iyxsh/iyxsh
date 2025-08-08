#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>

// 定义日志级别
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

// 定义日志配置结构体
typedef struct {
    LogLevel level;
    size_t max_size;
    int max_backups;
    char file_path[512];
} LoggerConfig;

// 初始化日志系统
int logger_init_with_config(const LoggerConfig* config);
void logger_init(const char* log_file_path);

// 设置日志级别
void logger_set_level(LogLevel level);

// 获取当前日志级别
LogLevel logger_get_level();

// 记录日志信息
void logger_log(LogLevel level, const char* format, ...);

// 带文件名和行号的日志记录
void logger_log_with_location(LogLevel level, const char* file, int line, const char* format, ...);

// 强制刷新日志缓冲区
void logger_flush();

// 关闭日志系统
void logger_close();

// 为向后兼容保留旧的宏定义，但使用不同的参数名
#define logger_log_info(format, ...) logger_log_with_location(LOG_LEVEL_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define logger_log_error(format, ...) logger_log_with_location(LOG_LEVEL_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define logger_log_warn(format, ...) logger_log_with_location(LOG_LEVEL_WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define logger_log_debug(format, ...) logger_log_with_location(LOG_LEVEL_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)

// 如果需要不带文件名和行号的日志，可以使用这些宏
#define logger_log_info_simple(format, ...) logger_log(LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define logger_log_error_simple(format, ...) logger_log(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#define logger_log_warn_simple(format, ...) logger_log(LOG_LEVEL_WARN, format, ##__VA_ARGS__)
#define logger_log_debug_simple(format, ...) logger_log(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)

#endif