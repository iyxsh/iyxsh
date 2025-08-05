#include "handle.h"
#include "../filemanager/filemanager.h"
#include "../cJSON/cJSON.h"
extern char* databasedata;
extern char* defaultname;

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
// 新增处理函数
void deal_style_request(RequestBody requestbody,ResponseBody *responsebody) 
{
    if (strcmp(requestbody.method, "POST") != 0 && strcmp(requestbody.path, "/get-gridcircle-default")!= 0) {
        responsebody->status = STATUS_METHOD_NOT_ALLOWED;
        memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
        sprintf(responsebody->content_type, "application/json");
        return;
    }
    cJSON *results = cJSON_CreateArray();
    responsebody->current_page = requestbody.current_page;
    responsebody->page_size = requestbody.page_size;
    //gridcirclesearch(results,requestbody.body_start,1,responsebody->current_page,&responsebody->page_size,&responsebody->total_count);
    char *json_str = cJSON_Print(results);
    responsebody->status = STATUS_SUCCESS;
    memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
    sprintf(responsebody->content_type, "application/json");
    memset(responsebody->body, 0, sizeof(responsebody->body));
    sprintf(responsebody->body, "%s", json_str);
    cJSON_Delete(results);

}

// 新增处理函数
void deal_newfile_request(RequestBody requestbody,ResponseBody *responsebody) 
{
    if (strcmp(requestbody.method, "POST") != 0 && strcmp(requestbody.path, "/newfile")!= 0) {
        responsebody->status = STATUS_METHOD_NOT_ALLOWED;
        memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
        sprintf(responsebody->content_type, "application/json");
        return;
    }
    
    // 检查请求体是否为空
    if (!requestbody.body_start) {
        responsebody->status = STATUS_BAD_REQUEST;
        memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
        sprintf(responsebody->content_type, "application/json");
        cJSON *error_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(error_obj, "error", "Empty request body");
        char *error_str = cJSON_Print(error_obj);
        memset(responsebody->body, 0, sizeof(responsebody->body));
        sprintf(responsebody->body, "%s", error_str);
        cJSON_Delete(error_obj);
        free(error_str);
        return;
    }
    
    cJSON *results = cJSON_CreateObject();
    filemanager::newfileCreate(results,requestbody.body_start);
    char *json_str = cJSON_Print(results);
    responsebody->status = STATUS_SUCCESS;
    memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
    sprintf(responsebody->content_type, "application/json");
    memset(responsebody->body, 0, sizeof(responsebody->body));
    sprintf(responsebody->body, "%s", json_str);
    cJSON_Delete(results);
    free(json_str);
}

void deal_updatefile_request(RequestBody requestbody,ResponseBody *responsebody)
{
    if (strcmp(requestbody.method, "POST") != 0 && strcmp(requestbody.path, "/updatefile") != 0) {
        responsebody->status = STATUS_METHOD_NOT_ALLOWED;
        memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
        sprintf(responsebody->content_type, "application/json");
        return;
    }
    
    // 检查请求体是否为空
    if (!requestbody.body_start) {
        responsebody->status = STATUS_BAD_REQUEST;
        memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
        sprintf(responsebody->content_type, "application/json");
        cJSON *error_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(error_obj, "error", "Empty request body");
        char *error_str = cJSON_Print(error_obj);
        memset(responsebody->body, 0, sizeof(responsebody->body));
        sprintf(responsebody->body, "%s", error_str);
        cJSON_Delete(error_obj);
        free(error_str);
        return;
    }
    
    cJSON *results = cJSON_CreateArray();
    printf("request update body:%s\n", requestbody.body_start);
    filemanager::updatefile(results,requestbody.body_start);
    char *json_str = cJSON_Print(results);
    responsebody->status = STATUS_SUCCESS;
    memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
    sprintf(responsebody->content_type, "application/json");
    memset(responsebody->body, 0, sizeof(responsebody->body));
    sprintf(responsebody->body, "%s", json_str);
    cJSON_Delete(results);
    free(json_str);
}

void deal_editfile_request(RequestBody requestbody,ResponseBody *responsebody)
{
    if (strcmp(requestbody.method, "POST") != 0 && strcmp(requestbody.path, "/editfile") != 0) {
        responsebody->status = STATUS_METHOD_NOT_ALLOWED;
        memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
        sprintf(responsebody->content_type, "application/json");
        return;
    }
    
    // 检查请求体是否为空
    if (!requestbody.body_start) {
        responsebody->status = STATUS_BAD_REQUEST;
        memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
        sprintf(responsebody->content_type, "application/json");
        cJSON *error_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(error_obj, "error", "Empty request body");
        char *error_str = cJSON_Print(error_obj);
        memset(responsebody->body, 0, sizeof(responsebody->body));
        sprintf(responsebody->body, "%s", error_str);
        cJSON_Delete(error_obj);
        free(error_str);
        return;
    }
    
    cJSON *results = cJSON_CreateArray();
    printf("request editfile body:%s\n", requestbody.body_start);
    filemanager::editfile(results,requestbody.body_start);
    char *json_str = cJSON_Print(results);
    responsebody->status = STATUS_SUCCESS;
    memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
    sprintf(responsebody->content_type, "application/json");
    memset(responsebody->body, 0, sizeof(responsebody->body));
    sprintf(responsebody->body, "%s", json_str);
    cJSON_Delete(results);
    free(json_str);
}

void deal_filedata_request(RequestBody requestbody,ResponseBody *responsebody)
{
    if (strcmp(requestbody.method, "POST") != 0 && strcmp(requestbody.path, "/opendata")!= 0) {
        responsebody->status = STATUS_METHOD_NOT_ALLOWED;
        memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
        sprintf(responsebody->content_type, "application/json");
        return;
    }
    
    // 检查请求体是否为空
    if (!requestbody.body_start) {
        responsebody->status = STATUS_BAD_REQUEST;
        memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
        sprintf(responsebody->content_type, "application/json");
        cJSON *error_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(error_obj, "error", "Empty request body");
        char *error_str = cJSON_Print(error_obj);
        memset(responsebody->body, 0, sizeof(responsebody->body));
        sprintf(responsebody->body, "%s", error_str);
        cJSON_Delete(error_obj);
        free(error_str);
        return;
    }
    
    cJSON *results = cJSON_CreateArray();
    responsebody->current_page = requestbody.current_page;
    responsebody->page_size = requestbody.page_size;
    //gridcirclesearch(results,requestbody.body_start,0,responsebody->current_page,&responsebody->page_size,&responsebody->total_count);//查询文件数据非默认设置
    char *json_str = cJSON_Print(results);
    responsebody->status = STATUS_SUCCESS;
    memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
    sprintf(responsebody->content_type, "application/json");
    memset(responsebody->body, 0, sizeof(responsebody->body));
    sprintf(responsebody->body, "%s", json_str);
    cJSON_Delete(results);
    free(json_str);
}

void deal_filelist_request(RequestBody requestbody,ResponseBody *responsebody)
{
    if (strcmp(requestbody.method, "GET")!= 0 && strcmp(requestbody.path, "/filelist")!= 0) {
        responsebody->status = STATUS_METHOD_NOT_ALLOWED;
        memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
        sprintf(responsebody->content_type, "application/json");
        return;
    }
    //获取所有数据库文件名
    //cJSON *results = get_all_dbnames();
    cJSON *results;
    responsebody->current_page = requestbody.current_page;
    responsebody->page_size = requestbody.page_size;
    char *json_str = cJSON_Print(results);
    responsebody->status = STATUS_SUCCESS;
    memset(responsebody->content_type, 0, sizeof(responsebody->content_type));
    sprintf(responsebody->content_type, "application/json");
    memset(responsebody->body, 0, sizeof(responsebody->body));
    sprintf(responsebody->body, "%s", json_str);
    cJSON_Delete(results);
}

// 新增：验证Content-Type中间件
int validate_content_type(const char* request_buffer) {
    const char* content_type = strstr(request_buffer, "Content-Type:");
    if (content_type && strstr(content_type, "application/json") == NULL) {
        return 0;
    }
    return 1;
}