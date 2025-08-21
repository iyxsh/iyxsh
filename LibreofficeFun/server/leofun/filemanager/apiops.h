#ifndef APIOPS_H
#define APIOPS_H

#include "../cJSON/cJSON.h"
#include <string>

namespace filemanager 
{
    //文件相关函数
    void newfileCreate(cJSON *results, const char *body);
    void updatefile(cJSON *results, const char *body);
    void editfile(cJSON *results, const char *body);
    void renamefile(cJSON *results, const char *body);
    void filestatus(cJSON *results, const char *body);
    void filelist(cJSON *results, const char *body);
    void deletefile(cJSON *results, const char *body);
    // 工作表相关函数
    void addworksheet(cJSON *results, const char *body);
    void removeworksheet(cJSON *results, const char *body);
    void renameworksheet(cJSON *results, const char *body);    
    /// @brief 获取文件的所有工作表名称
    void sheetlist(cJSON *results, const char *body);
    // 添加获取工作表数据的函数声明
    void sheetdata(cJSON *results, const char *body);
    
    // 系统初始化函数
    void defaultGet(cJSON *results, const char *body);

} // namespace filemanager

#endif // APIOPS_H