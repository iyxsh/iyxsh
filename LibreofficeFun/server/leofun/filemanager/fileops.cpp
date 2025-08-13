#include "fileops.h"
#include "filequeue.h"
#include "filehandlers.h"
#include "spreadsheet.h"
#include "utils.h"
#include "../cJSON/cJSON.h"
#include "../logger/logger.h"
#include "../config/json_config.h"
#include <iostream>
#include <ctime>
#include <string>
#include <mutex>
#include <unistd.h> // 添加 getcwd 头文件
#include <dirent.h>
#include <sys/stat.h>

#include "lofficeconn.h" // 确保 LibreOfficeConnectionManager 可用

namespace filemanager
{
    void initializeDataPathFiles()
    {
        // 读取datapath目录下所有文件并加入队列
        const char *datapath = json_config_get_string("datapath");
        if (datapath)
        {
            DIR *dir = opendir(datapath);
            if (dir)
            {
                logger_log_info("Scanning directory: %s", datapath);
                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL)
                {
                    // 跳过目录和隐藏文件
                    if (entry->d_type == DT_DIR || entry->d_name[0] == '.')
                    {
                        continue;
                    }

                    // 检查是否为ods文件
                    std::string filename(entry->d_name);
                    // 代码中任务和保存时使用的名称不包含路径和后缀（和接口保持一致，所有路径和后缀都在代码逻辑中处理）
                    if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".ods")
                    {
                        std::string filepath = std::string(datapath) + "/" + filename;

                        // 获取文件状态
                        struct stat fileStat;
                        if (stat(filepath.c_str(), &fileStat) == 0)
                        {
                            // 添加文件到队列管理器，使用文件的真实修改时间
                            filemanager::FileInfo info;
                            info.filename = removeFileExtension(filename);
                            info.status = filemanager::FILE_STATUS_READY;
                            info.lastModified = fileStat.st_mtime; // 使用文件的真实修改时间
                            info.errorMessage = "";

                            filemanager::FileQueueManager::getInstance().addFileStatus(info);
                            logger_log_info("Added existing file to queue: %s with mtime: %ld", filename.c_str(), (long)fileStat.st_mtime);
                        }
                        else
                        {
                            logger_log_warn("Failed to get file status for: %s", filepath.c_str());
                        }
                    }
                }
                closedir(dir);
                logger_log_info("Finished scanning directory: %s", datapath);
            }
            else
            {
                logger_log_warn("Failed to open datapath directory: %s", datapath);
            }
        }
        else
        {
            logger_log_warn("datapath not configured in config file");
        }
    }

    void filelist(cJSON *results, const char *body)
    {
        // 即使body为空也继续处理，因为filelist不需要请求体
        // 但如果有body且不为空，需要验证其为有效JSON
        if (body && strlen(body) > 0)
        {
            cJSON *root = cJSON_Parse(body);
            if (!root)
            {
                cJSON_AddStringToObject(results, "error", "Invalid JSON body");
                return;
            }
            cJSON_Delete(root);
        }

        // 获取默认模板文件名
        const char *defaultTemplateName = json_config_get_string("defaultname");
        if (!defaultTemplateName)
        {
            defaultTemplateName = "default.ods"; // 默认模板文件名
        }
        std::string defaultTemplateFile(defaultTemplateName);

        // 获取所有文件状态信息
        auto fileStatusMap = filemanager::FileQueueManager::getInstance().getFileStatusMapCopy();

        // 遍历所有文件状态信息并添加到结果中
        for (const auto &pair : fileStatusMap)
        {
            const filemanager::FileInfo &fileInfo = pair.second;

            // 过滤掉模板文件
            if (fileInfo.filename == removeFileExtension(defaultTemplateFile))
            {
                continue;
            }

            cJSON *fileObj = cJSON_CreateObject();
            cJSON_AddStringToObject(fileObj, "filename", removeFileExtension(fileInfo.filename).c_str());

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

            cJSON_AddStringToObject(fileObj, "status", statusStr);
            cJSON_AddNumberToObject(fileObj, "lastModified", static_cast<double>(fileInfo.lastModified));

            if (!fileInfo.errorMessage.empty())
            {
                cJSON_AddStringToObject(fileObj, "errorMessage", fileInfo.errorMessage.c_str());
            }

            cJSON_AddItemToArray(results, fileObj);
        }
    }

    void filestatus(cJSON *results, const char *body)
    {
        // 解析请求体获取文件名
        if (!body || strlen(body) == 0)
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
        cJSON_AddStringToObject(results, "filename", removeFileExtension(fileInfo.filename).c_str());

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
        // newfileCreate不需要请求体，所以即使body为空也继续处理
        // 但如果有body且不为空，需要验证其为有效JSON（如果需要解析）
        if (body && strlen(body) > 0)
        {
            cJSON *jsonRoot = cJSON_Parse(body);
            if (!jsonRoot)
            {
                cJSON_AddStringToObject(results, "error", "Failed to parse JSON");
                return;
            }

            // 创建文件任务
            filemanager::FileTask task;
            task.type = filemanager::TASK_CREATE_FILE;
            task.createTime = std::time(nullptr);

            cJSON *filenameItem = cJSON_GetObjectItem(jsonRoot, "filename");
            if (filenameItem && cJSON_IsString(filenameItem))
            {
                task.filename = std::string(filenameItem->valuestring);
                filemanager::FileQueueManager::getInstance().addFileStatus(task.filename, filemanager::FILE_STATUS_CREATED);
                filemanager::FileQueueManager::getInstance().addFileTask(task);
                // 构造返回结果
                cJSON_AddStringToObject(results, "result", "success");
                cJSON_AddStringToObject(results, "filename", removeFileExtension(std::string(filenameItem->valuestring)).c_str());
                cJSON_AddStringToObject(results, "filestatus", "processing");
            }
            else
            {
                cJSON_AddStringToObject(results, "result", "failed");
                cJSON_AddStringToObject(results, "message", "Filename is missing or invalid");
                return;
            }
        }
        else
        {
            // 生成唯一文件名
            time_t now = time(0);
            tm *ltm = localtime(&now);

            char filename[100];
            sprintf(filename, "spreadsheet_%04d%02d%02d_%02d%02d%02d",
                    1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday,
                    ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
            logger_log_info("filename: %s", filename);

            // 确保文件名有.ods后缀
            std::string filenameWithExt = std::string(filename);

            // 创建文件任务
            filemanager::FileTask task;
            task.type = filemanager::TASK_CREATE_FILE;
            task.filename = filenameWithExt;
            task.createTime = std::time(nullptr);
            filemanager::FileQueueManager::getInstance().addFileStatus(task.filename, filemanager::FILE_STATUS_CREATED);
            filemanager::FileQueueManager::getInstance().addFileTask(task);

            // 构造返回结果
            cJSON_AddStringToObject(results, "result", "success");
            cJSON_AddStringToObject(results, "filename", filename);
            cJSON_AddStringToObject(results, "filestatus", "processing");
        }
    }

    void updatefile(cJSON *results, const char *body)
    {
        // 更新文件需要请求体，检查body是否为空
        if (!body || strlen(body) == 0)
        {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }

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
        cJSON *updatedataItem = cJSON_GetObjectItem(jsonRoot, "updatedata");
        if (!filenameItem || !updatedataItem)
        {
            logger_log_error("error Missing filename or updatedata in updatefile data");
            cJSON_AddStringToObject(results, "error", "Missing filename or updatedata in updatefile data");

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
        createTask.taskData = cJSON_Duplicate(jsonRoot, cJSON_True);

        // 添加任务到队列
        filemanager::FileQueueManager::getInstance().addFileTask(createTask);
        cJSON_AddStringToObject(results, "result", "success");
        cJSON_AddStringToObject(results, "filename", removeFileExtension(std::string(filenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "filestatus", "processing");
        cJSON_Delete(jsonRoot);
    }

    void editfile(cJSON *results, const char *body)
    {
        // 编辑文件需要请求体，检查body是否为空
        if (!body || strlen(body) == 0)
        {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }

        // body 应为 JSON 字符串，支持批量编辑（如批量写入单元格）
        // 这里实现与 updatefile 类似，支持多单元格写入
        updatefile(results, body);
    }

    void deletefile(cJSON *results, const char *body)
    {
        // 删除文件需要请求体，检查body是否为空
        if (!body || strlen(body) == 0)
        {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }

        // 创建删除文件任务并添加到队列
        filemanager::FileTask deleteTask;
        deleteTask.type = filemanager::TASK_DELETE_FILE;
        deleteTask.filename = std::string("");
        deleteTask.data = std::string("");
        deleteTask.createTime = std::time(nullptr);
        deleteTask.taskData = nullptr;

        // body 应为 JSON 字符串，包含要删除的文件名
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
            logger_log_error("error Missing filename in deletefile data");
            cJSON_AddStringToObject(results, "error", "Missing filename in deletefile data");

            cJSON_Delete(jsonRoot);
            return;
        }

        deleteTask.filename = std::string(filenameItem->valuestring);

        // 添加任务到队列
        filemanager::FileQueueManager::getInstance().addFileTask(deleteTask);
        cJSON_AddStringToObject(results, "result", "success");
        cJSON_AddStringToObject(results, "filename", removeFileExtension(std::string(filenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "filestatus", "deleted");
        cJSON_Delete(jsonRoot);
    }

    void addworksheet(cJSON *results, const char *body)
    {
        // 添加工作表需要请求体，检查body是否为空
        if (!body || strlen(body) == 0)
        {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }

        // 创建新增工作表任务并添加到队列
        filemanager::FileTask addSheetTask;
        addSheetTask.type = filemanager::TASK_ADD_WORKSHEET;
        addSheetTask.filename = std::string("");
        addSheetTask.data = std::string("");
        addSheetTask.createTime = std::time(nullptr);
        addSheetTask.taskData = nullptr;

        // body 应为 JSON 字符串，包含文件名和工作表名
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
        cJSON *sheetnameItem = cJSON_GetObjectItem(jsonRoot, "sheetname");
        if (!filenameItem || !sheetnameItem)
        {
            logger_log_error("error Missing filename or sheetname in addworksheet data");
            cJSON_AddStringToObject(results, "error", "Missing filename or sheetname in addworksheet data");

            cJSON_Delete(jsonRoot);
            return;
        }

        addSheetTask.filename = std::string(filenameItem->valuestring);

        // 正确管理cJSON内存
        if (addSheetTask.taskData)
        {
            cJSON_Delete(addSheetTask.taskData);
        }
        addSheetTask.taskData = cJSON_Duplicate(jsonRoot, cJSON_True);

        // 添加任务到队列
        filemanager::FileQueueManager::getInstance().addFileTask(addSheetTask);
        cJSON_AddStringToObject(results, "result", "success");
        cJSON_AddStringToObject(results, "filename", removeFileExtension(std::string(filenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "sheetname", removeFileExtension(std::string(sheetnameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "filestatus", "processing");
        cJSON_Delete(jsonRoot);
    }

    void removeworksheet(cJSON *results, const char *body)
    {
        // 删除工作表需要请求体，检查body是否为空
        if (!body || strlen(body) == 0)
        {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }

        // 创建删除工作表任务并添加到队列
        filemanager::FileTask removeSheetTask;
        removeSheetTask.type = filemanager::TASK_REMOVE_WORKSHEET;
        removeSheetTask.filename = std::string("");
        removeSheetTask.data = std::string("");
        removeSheetTask.createTime = std::time(nullptr);
        removeSheetTask.taskData = nullptr;

        // body 应为 JSON 字符串，包含文件名和工作表名
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
        cJSON *sheetnameItem = cJSON_GetObjectItem(jsonRoot, "sheetname");
        if (!filenameItem || !sheetnameItem)
        {
            logger_log_error("error Missing filename or sheetname in removeworksheet data");
            cJSON_AddStringToObject(results, "error", "Missing filename or sheetname in removeworksheet data");

            cJSON_Delete(jsonRoot);
            return;
        }

        removeSheetTask.filename = std::string(filenameItem->valuestring);
        std::string sheetName = std::string(sheetnameItem->valuestring);
        // 正确管理cJSON内存
        if (removeSheetTask.taskData)
        {
            cJSON_Delete(removeSheetTask.taskData);
        }
        removeSheetTask.taskData = cJSON_Duplicate(jsonRoot, cJSON_True);

        // 添加任务到队列
        filemanager::FileQueueManager::getInstance().addFileTask(removeSheetTask);
        cJSON_AddStringToObject(results, "result", "success");
        cJSON_AddStringToObject(results, "filename", removeFileExtension(std::string(filenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "sheetname", removeFileExtension(std::string(sheetnameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "filestatus", "processing");
        cJSON_Delete(jsonRoot);
    }

    void renameworksheet(cJSON *results, const char *body)
    {
        // 重命名工作表需要请求体，检查body是否为空
        if (!body || strlen(body) == 0)
        {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }

        // 创建重命名工作表任务并添加到队列
        filemanager::FileTask renameSheetTask;
        renameSheetTask.type = filemanager::TASK_RENAME_WORKSHEET;
        renameSheetTask.filename = std::string("");
        renameSheetTask.data = std::string("");
        renameSheetTask.createTime = std::time(nullptr);
        renameSheetTask.taskData = nullptr;

        // body 应为 JSON 字符串，包含文件名、原工作表名和新工作表名
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
        cJSON *sheetnameItem = cJSON_GetObjectItem(jsonRoot, "sheetname");
        cJSON *newsheetnameItem = cJSON_GetObjectItem(jsonRoot, "newsheetname");
        if (!filenameItem || !sheetnameItem || !newsheetnameItem)
        {
            logger_log_error("error Missing filename, sheetname or newsheetname in renameworksheet data");
            cJSON_AddStringToObject(results, "error", "Missing filename, sheetname or newsheetname in renameworksheet data");

            cJSON_Delete(jsonRoot);
            return;
        }

        renameSheetTask.filename = std::string(filenameItem->valuestring);
        std::string sheetName = std::string(sheetnameItem->valuestring);
        // 正确管理cJSON内存
        if (renameSheetTask.taskData)
        {
            cJSON_Delete(renameSheetTask.taskData);
        }
        renameSheetTask.taskData = cJSON_Duplicate(jsonRoot, cJSON_True);

        // 添加任务到队列
        filemanager::FileQueueManager::getInstance().addFileTask(renameSheetTask);
        cJSON_AddStringToObject(results, "result", "success");
        cJSON_AddStringToObject(results, "filename", removeFileExtension(std::string(filenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "sheetname", removeFileExtension(std::string(sheetnameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "newsheetname", removeFileExtension(std::string(newsheetnameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "filestatus", "processing");
        cJSON_Delete(jsonRoot);
    }

    void sheetlist(cJSON *results, const char *body) {
        cJSON *filenameJson = cJSON_GetObjectItem(cJSON_Parse(body), "filename");
        if (!filenameJson || !cJSON_IsString(filenameJson)) {
            cJSON_AddStringToObject(results, "error", "Invalid filename parameter");
            return;
        }

        std::string filename = filenameJson->valuestring;
        rtl::OUString filePath = convertStringToOUString(filename.c_str());

        try {
            uno::Reference<frame::XComponentLoader> xLoader = LibreOfficeConnectionManager::getComponentLoader();
            uno::Reference<lang::XComponent> xComponent = xLoader->loadComponentFromURL(
                filePath, "_blank", 0, uno::Sequence<beans::PropertyValue>());

            uno::Reference<sheet::XSpreadsheetDocument> xSpreadsheetDocument(xComponent, uno::UNO_QUERY);
            if (!xSpreadsheetDocument.is()) {
                cJSON_AddStringToObject(results, "error", "Failed to load spreadsheet document");
                return;
            }

            uno::Reference<sheet::XSpreadsheets> xSheetsRaw = xSpreadsheetDocument->getSheets();
            uno::Reference<container::XNameAccess> xSheets(xSheetsRaw, uno::UNO_QUERY);
            if (!xSheets.is()) {
                cJSON_AddStringToObject(results, "error", "Failed to access sheets");
                return;
            }

            uno::Sequence<rtl::OUString> sheetNames = xSheets->getElementNames();
            cJSON *sheetArray = cJSON_CreateArray();
            for (sal_Int32 i = 0; i < sheetNames.getLength(); ++i) {
                cJSON_AddItemToArray(sheetArray, cJSON_CreateString(sheetNames[i].toUtf8().getStr()));
            }

            cJSON_AddItemToObject(results, "sheets", sheetArray);
        } catch (uno::Exception &e) {
            rtl::OUString errorMessage = e.Message;
            rtl::OString errorString = rtl::OUStringToOString(errorMessage, RTL_TEXTENCODING_UTF8);
            cJSON_AddStringToObject(results, "error", errorString.getStr());
        }
    }

    // Optimized implementation for handling sheet data directly without queue processing
    void sheetdata(cJSON *results, const char *body)
    {
        // Check if the request body is empty
        if (!body || strlen(body) == 0)
        {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }

        // Parse the input JSON string
        cJSON *jsonRoot = cJSON_Parse(body);
        if (!jsonRoot)
        {
            cJSON_AddStringToObject(results, "error", "Failed to parse JSON");
            return;
        }

        // Extract required parameters from the JSON object
        cJSON *filenameItem = cJSON_GetObjectItem(jsonRoot, "filename");
        cJSON *sheetnameItem = cJSON_GetObjectItem(jsonRoot, "sheetname");
        cJSON *pageSizeItem = cJSON_GetObjectItem(jsonRoot, "pageSize");
        cJSON *pageIndexItem = cJSON_GetObjectItem(jsonRoot, "pageIndex");
        cJSON *batchSizeItem = cJSON_GetObjectItem(jsonRoot, "batchSize");
        cJSON *enableStreamingItem = cJSON_GetObjectItem(jsonRoot, "enableStreaming");
        cJSON *enableCompressionItem = cJSON_GetObjectItem(jsonRoot, "enableCompression");

        // Validate required fields
        if (!filenameItem || !cJSON_IsString(filenameItem) ||
            !sheetnameItem || !cJSON_IsString(sheetnameItem) ||
            !pageSizeItem || !cJSON_IsNumber(pageSizeItem) ||
            !pageIndexItem || !cJSON_IsNumber(pageIndexItem) ||
            !batchSizeItem || !cJSON_IsNumber(batchSizeItem) ||
            !enableStreamingItem || !cJSON_IsBool(enableStreamingItem) ||
            !enableCompressionItem || !cJSON_IsBool(enableCompressionItem))
        {
            cJSON_AddStringToObject(results, "error", "Invalid or missing parameters");
            cJSON_Delete(jsonRoot);
            return;
        }

        // 调用 filehandlers.cpp 中的 sheetdata 函数
        cJSON *taskData = cJSON_CreateObject();
        cJSON_AddStringToObject(taskData, "filename", filenameItem->valuestring);
        cJSON_AddStringToObject(taskData, "sheetname", sheetnameItem->valuestring);
        cJSON_AddNumberToObject(taskData, "pageSize", pageSizeItem->valuedouble);
        cJSON_AddNumberToObject(taskData, "pageIndex", pageIndexItem->valuedouble);

        int status = filemanager::sheetdata(taskData);
        if (status != 0)
        {
            cJSON_AddStringToObject(results, "error", "Failed to process sheet data");
        }
        else
        {
            cJSON_AddItemToObject(results, "data", cJSON_Duplicate(taskData, true));
        }

        // Add response data
        cJSON_AddStringToObject(results, "result", "success");
        cJSON_AddStringToObject(results, "filename", filenameItem->valuestring);
        cJSON_AddStringToObject(results, "sheetname", sheetnameItem->valuestring);

        // 清理资源
        cJSON_Delete(taskData);

        // Clean up
        cJSON_Delete(jsonRoot);
    }

    void renamefile(cJSON *results, const char *body)
    {
        // 重命名文件需要请求体，检查body是否为空
        if (!body || strlen(body) == 0)
        {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }

        // 创建重命名文件任务并添加到队列
        filemanager::FileTask renameFileTask;
        renameFileTask.type = filemanager::TASK_RENAME_FILE;
        renameFileTask.filename = std::string("");
        renameFileTask.data = std::string("");
        renameFileTask.createTime = std::time(nullptr);
        renameFileTask.taskData = nullptr;

        // body 应为 JSON 字符串，包含原文件名和新文件名
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

        cJSON *oldFilenameItem = cJSON_GetObjectItem(jsonRoot, "oldFilename");
        cJSON *newFilenameItem = cJSON_GetObjectItem(jsonRoot, "newFilename");
        if (!oldFilenameItem || !newFilenameItem)
        {
            logger_log_error("error Missing oldFilename or newFilename in renamefile data");
            cJSON_AddStringToObject(results, "error", "Missing oldFilename or newFilename in renamefile data");

            cJSON_Delete(jsonRoot);
            return;
        }

        renameFileTask.filename = std::string(oldFilenameItem->valuestring);
        std::string newFilename = std::string(newFilenameItem->valuestring);
        // 正确管理cJSON内存
        if (renameFileTask.taskData)
        {
            cJSON_Delete(renameFileTask.taskData);
        }
        renameFileTask.taskData = cJSON_Duplicate(jsonRoot, cJSON_True);

        // 添加任务到队列
        filemanager::FileQueueManager::getInstance().addFileTask(renameFileTask);
        cJSON_AddStringToObject(results, "result", "success");
        cJSON_AddStringToObject(results, "oldFilename", removeFileExtension(std::string(oldFilenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "newFilename", removeFileExtension(std::string(newFilenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "filestatus", "processing");
        cJSON_Delete(jsonRoot);
    }
} // namespace filemanager