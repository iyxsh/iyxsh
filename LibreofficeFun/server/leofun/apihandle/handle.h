#ifndef HANDLE_H
#define HANDLE_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define BUFFER_SIZE 4096
#define MAX_PAGE_SIZE 100     // 最大单页数据量
#define DEFAULT_PAGE_SIZE 20  // 默认每页条数
// 限制请求体大小（如 64KB）
constexpr size_t MAX_REQUEST_SIZE = 65536;
// 限制响应体大小（如 128KB）
constexpr size_t MAX_RESPONSE_SIZE = 131072;
typedef struct RequestBody {
    char method[16];
    char path[256];
    char protocol[16];
    char* body_start;
    unsigned int page_size;    // 每页条数
    unsigned int current_page; // 当前页码
}RequestBody;
typedef struct ResponseBody {
    int status;
    char errmsg[512]; // 错误信息
    char content_type[20];
    char body[MAX_RESPONSE_SIZE];
    unsigned int total_count;  // 总数据量
    unsigned int page_size;    // 每页条数
    unsigned int current_page; // 当前页码
}ResponseBody;
void RequestBodyInit(RequestBody *request);
void ResponseBodyInit(struct ResponseBody* response);
void deal_default_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_newfile_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_updatefile_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_editfile_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_filedata_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_filelist_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_filestatus_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_opendata_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_deletefile_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_addworksheet_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_removeworksheet_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_renamefile_request(RequestBody requestbody, ResponseBody *responsebody);
void deal_renameworksheet_request(RequestBody requestbody, ResponseBody *responsebody);
void deal_sheetlist_request(RequestBody requestbody, ResponseBody *responsebody);
// 添加处理工作表数据请求的函数声明
void deal_sheetdata_request(RequestBody requestbody, ResponseBody *responsebody);

#endif