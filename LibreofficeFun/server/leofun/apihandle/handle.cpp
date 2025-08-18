#include "handle.h"
#include "../filemanager/fileops.h"
#include "../filemanager/template_index_cache.h"  // 添加文件队列管理器头文件
#include "../cJSON/cJSON.h"
#include <stdbool.h>
#include "../error/error_codes.h"
extern char* databasedata;
extern char* defaultname;

// 函数指针类型定义
typedef void (*file_operation_func)(cJSON *results, const char *body);

// 添加检查模板加载状态的函数
static bool checkTemplateLoading(ResponseBody *responsebody) {
    if (filemanager::TemplateIndexCacheManager::getInstance().isLoading()) {
        responsebody->status = STATUS_SERVICE_UNAVAILABLE;
        strcpy(responsebody->content_type, "application/json");
        cJSON *error_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(error_obj, "error", "System is updating template files, please try again later");
        char *error_str = cJSON_Print(error_obj);
        strcpy(responsebody->body, error_str);
        cJSON_Delete(error_obj);
        free(error_str);
        return true;
    }
    return false;
}

void RequestBodyInit(RequestBody *request)
{
    memset(request->method, 0, sizeof(request->method));
    memset(request->path, 0, sizeof(request->path));
    memset(request->protocol, 0, sizeof(request->protocol));
    request->body_start = NULL;
    request->current_page = 1;
    request->page_size = 20;
}

void ResponseBodyInit(ResponseBody* response)
{
    response->status = 200;
    memset(response->content_type, 0, sizeof(response->content_type));
    memset(response->body, 0, sizeof(response->body));
    response->total_count = 0;
    response->page_size = 0;
    response->current_page = 0;
}

/**
 * 通用请求验证函数
 */
static bool validate_request(RequestBody requestbody, ResponseBody *responsebody, 
                            const char *expected_method, const char *expected_path) 
{
    // 在处理请求前检查模板加载状态
    if (checkTemplateLoading(responsebody)) {
        return false;
    }
    
    printf("method: %s, path: %s\n", requestbody.method, requestbody.path);
    if (strcmp(requestbody.method, expected_method) != 0 || strcmp(requestbody.path, expected_path) != 0) {
        responsebody->status = STATUS_METHOD_NOT_ALLOWED;
        strcpy(responsebody->content_type, "application/json");
        return false;
    }
    return true;
}

/**
 * 处理需要请求体的请求的通用错误处理
 */
static bool handle_empty_body(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!requestbody.body_start) {
        responsebody->status = STATUS_BAD_REQUEST;
        sprintf(responsebody->errmsg,"%s",ErrorCodeManager::getErrorMessage(STATUS_BAD_REQUEST).c_str());
        strcpy(responsebody->content_type, "application/json");
        cJSON *error_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(error_obj, "error", "Empty request body");
        char *error_str = cJSON_Print(error_obj);
        strcpy(responsebody->body, error_str);
        cJSON_Delete(error_obj);
        free(error_str);
        return true;
    }
    return false;
}

/**
 * 通用响应处理函数
 */
static void send_response(ResponseBody *responsebody, cJSON *results, int status)
{
    char *json_str = cJSON_Print(results);
    if (json_str && strlen(json_str) >= MAX_RESPONSE_SIZE) {
        responsebody->status = STATUS_BAD_REQUEST;
        strcpy(responsebody->content_type, "application/json");
        strcpy(responsebody->body, "{\"error\":\"Response too large, please use paging or split your request.\"}");
        cJSON_Delete(results);
        free(json_str);
        return;
    }
    responsebody->status = status;
    strcpy(responsebody->content_type, "application/json");
    strcpy(responsebody->body, json_str);
    cJSON_Delete(results);
    free(json_str);
}

/**
 * 处理获取网格圆圈默认设置的请求
 */
void deal_default_request(RequestBody requestbody, ResponseBody *responsebody) 
{
    if (!validate_request(requestbody, responsebody, "GET", "/api/default")) {
        return;
    }
    
    cJSON *results = cJSON_CreateArray();
    responsebody->current_page = requestbody.current_page;
    responsebody->page_size = requestbody.page_size;
    filemanager::defaultGet(results,requestbody.body_start);
    send_response(responsebody, results, STATUS_SUCCESS);
}

/**
 * 处理创建新文件的请求
 */
void deal_newfile_request(RequestBody requestbody, ResponseBody *responsebody) 
{
    if (!validate_request(requestbody, responsebody, "POST", "/api/newfile")) {
        return;
    }
    
    cJSON *results = cJSON_CreateObject();
    printf("request update body:%s\n", requestbody.body_start);
    filemanager::newfileCreate(results, requestbody.body_start);
    send_response(responsebody, results, STATUS_SUCCESS);
}

/**
 * 处理更新文件的请求
 */
void deal_updatefile_request(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!validate_request(requestbody, responsebody, "POST", "/api/updatefile")) {
        return;
    }
    
    if (handle_empty_body(requestbody, responsebody)) {
        return;
    }
    
    cJSON *results = cJSON_CreateObject();
    printf("request update body:%s\n", requestbody.body_start);
    filemanager::updatefile(results, requestbody.body_start);
    send_response(responsebody, results, STATUS_SUCCESS);
}

/**
 * 处理编辑文件的请求
 */
void deal_editfile_request(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!validate_request(requestbody, responsebody, "POST", "/api/editfile")) {
        return;
    }
    
    if (handle_empty_body(requestbody, responsebody)) {
        return;
    }
    
    cJSON *results = cJSON_CreateObject();
    printf("request editfile body:%s\n", requestbody.body_start);
    filemanager::editfile(results, requestbody.body_start);
    send_response(responsebody, results, STATUS_SUCCESS);
}

/**
 * 处理文件数据请求
 */
void deal_filedata_request(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!validate_request(requestbody, responsebody, "POST", "/api/filedata")) {
        return;
    }
    
    if (handle_empty_body(requestbody, responsebody)) {
        return;
    }
    
    cJSON *results = cJSON_CreateObject();
    responsebody->current_page = requestbody.current_page;
    responsebody->page_size = requestbody.page_size;
    //gridcirclesearch(results,requestbody.body_start,0,responsebody->current_page,&responsebody->page_size,&responsebody->total_count);//查询文件数据非默认设置
    send_response(responsebody, results, STATUS_SUCCESS);
}

/**
 * 处理文件列表请求
 */
void deal_filelist_request(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!validate_request(requestbody, responsebody, "GET", "/api/filelist")) {
        return;
    }
    
    cJSON *results = cJSON_CreateObject();
    filemanager::filelist(results, requestbody.body_start);
    send_response(responsebody, results, STATUS_SUCCESS);
}

/**
 * 处理文件状态请求
 */
void deal_filestatus_request(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!validate_request(requestbody, responsebody, "POST", "/api/filestatus")) {
        return;
    }
    
    if (handle_empty_body(requestbody, responsebody)) {
        return;
    }
    
    cJSON *results = cJSON_CreateObject();
    printf("request filestatus body:%s\n", requestbody.body_start);
    filemanager::filestatus(results, requestbody.body_start);
    send_response(responsebody, results, STATUS_SUCCESS);
}

/**
 * 处理打开数据请求
 */
void deal_opendata_request(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!validate_request(requestbody, responsebody, "POST", "/api/opendata")) {
        return;
    }
    
    if (handle_empty_body(requestbody, responsebody)) {
        return;
    }
    
    cJSON *results = cJSON_CreateArray();
    responsebody->current_page = requestbody.current_page;
    responsebody->page_size = requestbody.page_size;
    //gridcirclesearch(results,requestbody.body_start,0,responsebody->current_page,&responsebody->page_size,&responsebody->total_count);//查询文件数据非默认设置
    send_response(responsebody, results, STATUS_SUCCESS);
}

/**
 * 处理删除文件请求
 */
void deal_deletefile_request(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!validate_request(requestbody, responsebody, "POST", "/api/deletefile")) {
        return;
    }

    if (handle_empty_body(requestbody, responsebody)) {
        return;
    }

    cJSON *results = cJSON_CreateObject();
    printf("request deletefile body:%s\n", requestbody.body_start);
    filemanager::deletefile(results, requestbody.body_start);
    send_response(responsebody, results, STATUS_SUCCESS);
}

/**
 * 处理添加工作表请求
 */
void deal_addworksheet_request(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!validate_request(requestbody, responsebody, "POST", "/api/addworksheet")) {
        return;
    }

    if (handle_empty_body(requestbody, responsebody)) {
        return;
    }

    cJSON *results = cJSON_CreateObject();
    printf("request addworksheet body:%s\n", requestbody.body_start);
    filemanager::addworksheet(results, requestbody.body_start);
    send_response(responsebody, results, STATUS_SUCCESS);
}

/**
 * 处理删除工作表请求
 */
void deal_removeworksheet_request(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!validate_request(requestbody, responsebody, "POST", "/api/removeworksheet")) {
        return;
    }

    if (handle_empty_body(requestbody, responsebody)) {
        return;
    }

    cJSON *results = cJSON_CreateObject();
    printf("request removeworksheet body:%s\n", requestbody.body_start);
    filemanager::removeworksheet(results, requestbody.body_start);
    send_response(responsebody, results, STATUS_SUCCESS);
}

/**
 * 处理重命名文件请求
 */
void deal_renamefile_request(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!validate_request(requestbody, responsebody, "POST", "/api/renamefile")) {
        return;
    }

    if (handle_empty_body(requestbody, responsebody)) {
        return;
    }

    cJSON *results = cJSON_CreateObject();
    printf("request renamefile body:%s\n", requestbody.body_start);
    filemanager::renamefile(results, requestbody.body_start);
    send_response(responsebody, results, STATUS_SUCCESS);
}

/**
 * 处理重命名工作表请求
 */
void deal_renameworksheet_request(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!validate_request(requestbody, responsebody, "POST", "/api/renameworksheet")) {
        return;
    }

    if (handle_empty_body(requestbody, responsebody)) {
        return;
    }

    cJSON *results = cJSON_CreateObject();
    printf("request renameworksheet body:%s\n", requestbody.body_start);
    filemanager::renameworksheet(results, requestbody.body_start);
    send_response(responsebody, results, STATUS_SUCCESS);
}

void deal_sheetlist_request(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!validate_request(requestbody, responsebody, "POST", "/api/sheetlist")) {
        return;
    }

    if (handle_empty_body(requestbody, responsebody)) {
        return;
    }

    cJSON *results = cJSON_CreateObject();
    printf("request sheetlist body:%s\n", requestbody.body_start);
    filemanager::sheetlist(results, requestbody.body_start);
    send_response(responsebody, results, STATUS_SUCCESS);
}

/**
 * 处理工作表数据请求
 */
void deal_sheetdata_request(RequestBody requestbody, ResponseBody *responsebody)
{
    if (!validate_request(requestbody, responsebody, "POST", "/api/sheetdata")) {
        return;
    }

    if (handle_empty_body(requestbody, responsebody)) {
        return;
    }

    cJSON *results = cJSON_CreateObject();
    printf("request sheetdata body:%s\n", requestbody.body_start);
    filemanager::sheetdata(results, requestbody.body_start);
    send_response(responsebody, results, STATUS_SUCCESS);
}

// namespace apihandle
