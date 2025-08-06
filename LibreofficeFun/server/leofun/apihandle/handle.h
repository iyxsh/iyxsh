#ifndef HANDLE_H
#define HANDLE_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
//返回码
#define STATUS_SUCCESS 200
#define STATUS_METHOD_NOT_ALLOWED 405
#define STATUS_INTERNAL_SERVER_ERROR 500
#define STATUS_BAD_REQUEST 400 
#define STATUS_UNAUTHORIZED 401
#define STATUS_FORBIDDEN 403
#define STATUS_NOT_FOUND 404
#define STATUS_SERVICE_UNAVAILABLE 503
#define STATUS_GATEWAY_TIMEOUT 504
#define STATUS_UNSUPPORTED_MEDIA_TYPE 415

#define BUFFER_SIZE 4096
#define MAX_PAGE_SIZE 100     // 最大单页数据量
#define DEFAULT_PAGE_SIZE 20  // 默认每页条数
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
    char content_type[20];
    char body[BUFFER_SIZE];
    unsigned int total_count;  // 总数据量
    unsigned int page_size;    // 每页条数
    unsigned int current_page; // 当前页码
}ResponseBody;
void RequestBodyInit(RequestBody *request);
void ResponseBodyInit(struct ResponseBody* response);
void deal_style_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_newfile_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_updatefile_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_editfile_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_filedata_request(RequestBody requestbody,ResponseBody *responsebody);
void deal_filelist_request(RequestBody requestbody,ResponseBody *responsebody);
#endif