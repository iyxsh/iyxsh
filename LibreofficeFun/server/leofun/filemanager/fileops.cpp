#include "fileops.h"
#include "filequeue.h"
#include "spreadsheet.h"
#include "../cJSON/cJSON.h"
#include "../logger/logger.h"
#include "../config/json_config.h"
#include <iostream>
#include <ctime>
#include <string>
#include <mutex>
#include <unistd.h> // 添加 getcwd 头文件

namespace filemanager
{
    void filestatus(cJSON *results, const char *body)
    {
        // 解析请求体获取文件名
        if (!body)
        {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }

        cJSON *root = cJSON_Parse(body);
        if (!root)
        {
            cJSON_AddStringToObject(results, "error", "Invalid JSON body");
            return;
        }

        cJSON *filenameItem = cJSON_GetObjectItem(root, "filename");
        if (!filenameItem || !cJSON_IsString(filenameItem))
        {
            cJSON_AddStringToObject(results, "error", "Missing or invalid filename");
            cJSON_Delete(root);
            return;
        }

        // 复制文件名字符串，避免使用悬空指针
        std::string filename(filenameItem->valuestring);

        // 获取文件信息
        filemanager::FileInfo fileInfo = filemanager::FileQueueManager::getInstance().getFileInfo(filename);

        // 检查文件信息是否有效
        if (fileInfo.filename.empty())
        {
            cJSON_AddStringToObject(results, "error", "File not found");
            cJSON_Delete(root);
            return;
        }

        // 添加文件状态信息到结果
        cJSON_AddStringToObject(results, "filename", fileInfo.filename.c_str());

        // 将枚举状态转换为字符串
        const char *statusStr;
        switch (fileInfo.status)
        {
        case filemanager::FILE_STATUS_CREATED:
            statusStr = "created";
            break;
        case filemanager::FILE_STATUS_PROCESSING:
            statusStr = "processing";
            break;
        case filemanager::FILE_STATUS_READY:
            statusStr = "ready";
            break;
        case filemanager::FILE_STATUS_ERROR:
            statusStr = "error";
            break;
        case filemanager::FILE_STATUS_CLOSED:
            statusStr = "closed";
            break;
        default:
            statusStr = "unknown";
            break;
        }

        cJSON_AddStringToObject(results, "status", statusStr);
        cJSON_AddNumberToObject(results, "lastModified", static_cast<double>(fileInfo.lastModified));

        if (!fileInfo.errorMessage.empty())
        {
            cJSON_AddStringToObject(results, "errorMessage", fileInfo.errorMessage.c_str());
        }

        cJSON_Delete(root);
    }
    void newfileCreate(cJSON *results, const char *body)
    {
        // 生成唯一文件名
        time_t now = time(0);
        tm *ltm = localtime(&now);

        char filename[100];
        sprintf(filename, "spreadsheet_%04d%02d%02d_%02d%02d%02d.xlsx",
                1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday,
                ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
        logger_log_info("filename: %s", filename);
        // 创建文件任务
        filemanager::FileTask task;
        task.type = filemanager::TASK_CREATE_FILE;
        task.filename = std::string(filename);
        task.createTime = std::time(nullptr);
        filemanager::FileQueueManager::getInstance().addFileStatus(task.filename, filemanager::FILE_STATUS_CREATED);
        filemanager::FileQueueManager::getInstance().addFileTask(task);

        // 构造返回结果
        cJSON *resultItem = cJSON_CreateObject();
        cJSON_AddStringToObject(results, "result", "success");
        cJSON_AddStringToObject(results, "filename", filename);
        cJSON_AddStringToObject(results, "filestatus", "processing");
    }

    void updatefile(cJSON *results, const char *body)
    {
        // 创建文件任务并添加到队列
        filemanager::FileTask createTask;
        createTask.type = filemanager::TASK_UPDATE_FILE;
        createTask.filename = std::string("");
        createTask.data = std::string("");
        createTask.createTime = std::time(nullptr);
        createTask.taskData = nullptr; // 初始化为nullptr

        // body 应为 JSON 字符串，支持单个或批量更新
        if (!body)
        {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }

        cJSON *jsonRoot = cJSON_Parse(body);
        if (!jsonRoot)
        {
            cJSON_AddStringToObject(results, "error", "Failed to parse JSON");
            return;
        }

        cJSON *filenameItem = cJSON_GetObjectItem(jsonRoot, "filename");
        if (!filenameItem)
        {
            logger_log_error("error Missing filename or sheetname in batch data");
            cJSON_AddStringToObject(results, "error", "Missing filename or sheetname in update data");

            // 使用智能指针后需要正确清理
            cJSON_Delete(jsonRoot);
            return;
        }

        createTask.filename = std::string(filenameItem->valuestring);

        // 正确管理cJSON内存，首先释放旧数据，然后分配新数据
        if (createTask.taskData)
        {
            cJSON_Delete(createTask.taskData);
        }
        createTask.taskData = cJSON_Parse(body);

        // 添加文件状态
        filemanager::FileQueueManager::getInstance().addFileStatus(
            createTask.filename,
            filemanager::FILE_STATUS_PROCESSING);

        // 添加任务到队列
        filemanager::FileQueueManager::getInstance().addFileTask(createTask);
        cJSON_Delete(jsonRoot);
        cJSON_AddStringToObject(results, "result", "success");
        cJSON_AddStringToObject(results, "filestatus", "processing");
    }

    void editfile(cJSON *results, const char *body)
    {
        // body 应为 JSON 字符串，支持批量编辑（如批量写入单元格）
        // 这里实现与 updatefile 类似，支持多单元格写入
        updatefile(results, body);
    }

} // namespace filemanager