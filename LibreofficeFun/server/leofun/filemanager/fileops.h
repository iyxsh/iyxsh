#ifndef FILEOPS_H
#define FILEOPS_H

#include "../cJSON/cJSON.h"
#include <string>

namespace filemanager 
{
    void defaultGet(cJSON *results, const char *body);
    void newfileCreate(cJSON *results, const char *body);
    void updatefile(cJSON *results, const char *body);
    void editfile(cJSON *results, const char *body);
    void filestatus(cJSON *results, const char *body);
    void filelist(cJSON *results, const char *body);
    int backupdefaultfile();
    int setnewTodefaultfile();
    // 新增的接口函数声明
    void deletefile(cJSON *results, const char *body);
    void addworksheet(cJSON *results, const char *body);
    void removeworksheet(cJSON *results, const char *body);
    void renameworksheet(cJSON *results, const char *body);
    void renamefile(cJSON *results, const char *body);
    
    /// @brief 获取文件的所有工作表名称
    void sheetlist(cJSON *results, const char *body);

    // 添加获取工作表数据的函数声明
    void sheetdata(cJSON *results, const char *body);
    
    // 系统初始化函数
    void initializeDataPathFiles();
    
} // namespace filemanager

#endif // FILEOPS_H