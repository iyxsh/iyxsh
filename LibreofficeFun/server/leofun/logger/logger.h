#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

// 初始化日志文件
void logger_init(const char* log_file_path);

// 记录日志信息
void logger_log(const char* format, ...);

// 关闭日志文件
void logger_close();

#endif
