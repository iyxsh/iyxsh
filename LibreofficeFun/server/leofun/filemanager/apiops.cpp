#include "apiops.h"
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
#include <sys/stat.h>
#include "../error/error_codes.h"
#include "LibreOfficeService.h" // 确保 LibreOfficeService 头文件 可用

namespace filemanager
{
    void defaultGet(cJSON *results, const char *body)
    {
        logger_log_info("defaultGet start .................");
        rtl::OUString AbsolutefilePath;
        rtl::OUString defaultFileName;
        rtl::OUString sheetName;
        getDefaultData(AbsolutefilePath, defaultFileName, sheetName);
        std::shared_ptr<CharacterIndex> idxPtr = filemanager::TemplateIndexCacheManager::getInstance().getCharacterInfos(AbsolutefilePath + defaultFileName, sheetName);
        if (!idxPtr)
        {
            logger_log_error("Failed to get character infos from cache: idxPtr is null");
            return;
        }
        std::vector<TextCharInfo> infos;
        // 但如果有body且不为空，需要验证其为有效JSON
        if (body && strlen(body) > 0)
        {
            cJSON *root = cJSON_Parse(body);
            if (!root)
            {
                ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
                return;
            }

            // 检查请求体是否包含text字段
            cJSON *textItem = cJSON_GetObjectItem(root, "text");
            if (!textItem || textItem->type != cJSON_String)
            {
                ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
                cJSON_Delete(root);
                return;
            }
            std::string textStr(textItem->valuestring);
            // 这里 idx 只是 CharacterInfo 列表，需获取 CharacterIndex
            // 但 splitAndClassifyTextFromIndex 需要 CharacterIndex，实际应从缓存获取 index
            // 调用此接口默认查询维护模板文件，不存在直接添加到此文件中
            std::string docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(AbsolutefilePath + defaultFileName));
            if (docId.empty())
            {
                logger_log_error("Failed to load document: %s", convertOUStringToString(AbsolutefilePath + defaultFileName).c_str());
                return;
            }
            if (idxPtr)
            {
                infos = splitAndClassifyTextFromIndex(textStr, idxPtr, docId, sheetName); // 获取指定文本的字符信息
            }
            cJSON_Delete(root);
        }
        else
        {
            infos = idxPtr->getAll(); // 获取所有数据
        }
        cJSON *infosArray = cJSON_CreateArray();
        for (const auto &info : infos)
        {
            cJSON *infoObj = cJSON_CreateObject();
            cJSON_AddStringToObject(infoObj, "character", info.character.c_str());
            cJSON_AddStringToObject(infoObj, "pos", info.pos.c_str());
            cJSON_AddStringToObject(infoObj, "languageType", info.languageType.c_str());
            cJSON_AddStringToObject(infoObj, "bodyname", info.bodyname.c_str());
            cJSON_AddItemToArray(infosArray, infoObj);
        }
        cJSON_AddItemToObject(results, "infos", infosArray);
        ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);
        logger_log_info("defaultGet end   .................");
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
                ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
        cJSON *fileArray = cJSON_CreateArray();
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

            cJSON_AddStringToObject(fileObj, "filestatus", statusStr);
            cJSON_AddNumberToObject(fileObj, "lastModified", static_cast<double>(fileInfo.lastModified));

            if (!fileInfo.errorMessage.empty())
            {
                cJSON_AddStringToObject(fileObj, "errorMessage", fileInfo.errorMessage.c_str());
            }

            cJSON_AddItemToArray(fileArray, fileObj);
        }
        cJSON_AddItemToObject(results, "files", fileArray);
        ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);
    }

    void filestatus(cJSON *results, const char *body)
    {
        // 解析请求体获取文件名
        if (!body || strlen(body) == 0)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *root = cJSON_Parse(body);
        if (!root)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *filenameItem = cJSON_GetObjectItem(root, "filename");
        if (!filenameItem || !cJSON_IsString(filenameItem))
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
            ErrorCodeManager::setErrorMessage(results, RESPONSE_FILE_NOT_FOUND);
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

        cJSON_AddStringToObject(results, "filestatus", statusStr);
        cJSON_AddNumberToObject(results, "lastModified", static_cast<double>(fileInfo.lastModified));

        if (!fileInfo.errorMessage.empty())
        {
            cJSON_AddStringToObject(results, "errorMessage", fileInfo.errorMessage.c_str());
        }

        ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);
        cJSON_Delete(root);
    }
    void newfileCreate(cJSON *results, const char *body)
    {
        // newfileCreate不需要请求体，所以即使body为空也继续处理
        // 但如果有body且不为空，需要验证其为有效JSON（如果需要解析）
        // 因为UNO单例模式，删除重新建同名文件会出问题，所以设计一下流程
        logger_log_info("newfileCreate called with body: %s ", body);
        std::string inputfileName(""); // 默认为空
        if (body && strlen(body) > 0)
        {
            cJSON *jsonRoot = cJSON_Parse(body);
            if (!jsonRoot)
            {
                ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
                return;
            }
            cJSON *filenameItem = cJSON_GetObjectItem(jsonRoot, "filename");
            if (filenameItem && cJSON_IsString(filenameItem))
            {
                inputfileName = std::string(filenameItem->valuestring);
            }
            else
            {
                ErrorCodeManager::setErrorMessage(results, RESPONSE_FILE_NAME_INVALID);
                return;
            }
        }
        // 生成唯一文件名
        time_t now = time(0);
        tm *ltm = localtime(&now);

        char filename[100];
        sprintf(filename, "spreadsheet_%04d%02d%02d_%02d%02d%02d",
                1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday,
                ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
        logger_log_info("filename: %s", filename);

        std::string fileName = std::string(filename);
        // 实现创建文件的函数
        logger_log_info("Creating file: %s", fileName.c_str());
        try
        {
            rtl::OUString filePathStr = convertStringToOUString(ensureOdsExtension(fileName).c_str());
            // 获取绝对路径
            rtl::OUString filePath;
            getAbsolutePath(filePathStr, filePath);
            rtl::OUString defaultname;
            rtl::OUString defaultFileName;
            rtl::OUString sheetName;
            getDefaultData(defaultname, defaultFileName, sheetName);
            // 首先判断是否文件已经加载
            FileInfo fileInfo = filemanager::FileQueueManager::getInstance().getFileInfo(fileName);
            uno::Reference<sheet::XSpreadsheetDocument> xDoc;
            if (fileInfo.xDoc == nullptr && !fileInfo.xDoc.is() && fileInfo.status == FileStatus::FILE_STATUS_NOT_FOUND)
            {
                cJSON *newCreate = createNewSpreadsheetFile(filePath, sheetName); // 创建新的电子表格文件
                if (newCreate == nullptr)
                {
                    logger_log_error("Failed to create new spreadsheet file: %s", fileName.c_str());
                    ErrorCodeManager::setErrorMessage(results, RESPONSE_FAILED_TO_PROCESS);
                    return;
                }
            }
            else
            {
                // 如果文件已经加载，直接获取文档引用
                logger_log_info("File already loaded: %s", fileName.c_str());
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in createfile for %s: %s", fileName.c_str(), e.what());
            ErrorCodeManager::setErrorMessage(results, RESPONSE_FAILED_TO_PROCESS);
            return;
        }
        catch (...)
        {
            logger_log_error("Unknown exception in createfile for %s", fileName.c_str());
            ErrorCodeManager::setErrorMessage(results, RESPONSE_FAILED_TO_PROCESS);
            return;
        }
        // 创建文件任务
        filemanager::FileTask task;
        task.type = filemanager::TASK_CREATE_FILE;
        task.filename = fileName;
        task.createTime = std::time(nullptr);
        filemanager::FileQueueManager::getInstance().addFileStatus(task.filename, filemanager::FILE_STATUS_CREATED);
        filemanager::FileQueueManager::getInstance().addFileTask(task);

        // 如果有上送的文件名采用重命名逻辑实现
        if (!inputfileName.empty())
        {
            // 创建文件任务
            filemanager::FileTask task;
            task.type = filemanager::TASK_RENAME_FILE;
            task.filename = fileName;
            task.createTime = std::time(nullptr);
            cJSON *taskData = cJSON_CreateObject();
            cJSON_AddStringToObject(taskData, "oldFilename", fileName.c_str());
            cJSON_AddStringToObject(taskData, "newFilename", inputfileName.c_str());
            task.taskData = taskData;
            filemanager::FileQueueManager::getInstance().addFileTask(task);
            // 构造返回结果
            cJSON_AddStringToObject(results, "filename", inputfileName.c_str());
            cJSON_AddStringToObject(results, "filestatus", "created");
            ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);
        }
        else
        {
            // 构造返回结果
            cJSON_AddStringToObject(results, "filename", task.filename.c_str());
            cJSON_AddStringToObject(results, "filestatus", "created");
            ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);
        }
    }
    // 切换模板备份文件
    void newfile(cJSON *results, const char *body)
    {
        // newfile不需要请求体，所以即使body为空也继续处理
        cJSON_AddStringToObject(results, "filestatus", "processing");
        ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);
    }

    void updatefile(cJSON *results, const char *body)
    {
        // 更新文件需要请求体，检查body是否为空
        if (!body || strlen(body) == 0)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *jsonRoot = cJSON_Parse(body);
        if (!jsonRoot)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *filenameItem = cJSON_GetObjectItem(jsonRoot, "filename");
        cJSON *updatedataItem = cJSON_GetObjectItem(jsonRoot, "updatedata");
        if (!filenameItem || !updatedataItem)
        {
            logger_log_error("error Missing filename or updatedata in updatefile data");
            cJSON_AddStringToObject(results, "error", "Missing filename or updatedata in updatefile data");
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
        cJSON_AddStringToObject(results, "filename", removeFileExtension(std::string(filenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "filestatus", "processing");
        ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);
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
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *jsonRoot = cJSON_Parse(body);
        if (!jsonRoot)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *filenameItem = cJSON_GetObjectItem(jsonRoot, "filename");
        if (!filenameItem)
        {
            logger_log_error("error Missing filename in deletefile data");
            cJSON_AddStringToObject(results, "error", "Missing filename in deletefile data");
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        deleteTask.filename = std::string(filenameItem->valuestring);

        // 添加任务到队列
        filemanager::FileQueueManager::getInstance().addFileTask(deleteTask);
        cJSON_AddStringToObject(results, "filename", removeFileExtension(std::string(filenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "filestatus", "deleted");
        ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);
        cJSON_Delete(jsonRoot);
    }

    void addworksheet(cJSON *results, const char *body)
    {
        // 添加工作表需要请求体，检查body是否为空
        if (!body || strlen(body) == 0)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *jsonRoot = cJSON_Parse(body);
        if (!jsonRoot)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *filenameItem = cJSON_GetObjectItem(jsonRoot, "filename");
        cJSON *sheetnameItem = cJSON_GetObjectItem(jsonRoot, "sheetname");
        if (!filenameItem || !sheetnameItem)
        {
            logger_log_error("error Missing filename or sheetname in addworksheet data");
            cJSON_AddStringToObject(results, "error", "Missing filename or sheetname in addworksheet data");
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
        cJSON_AddStringToObject(results, "filename", removeFileExtension(std::string(filenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "sheetname", removeFileExtension(std::string(sheetnameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "filestatus", "processing");
        ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);
        cJSON_Delete(jsonRoot);
    }

    void removeworksheet(cJSON *results, const char *body)
    {
        // 删除工作表需要请求体，检查body是否为空
        if (!body || strlen(body) == 0)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *jsonRoot = cJSON_Parse(body);
        if (!jsonRoot)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *filenameItem = cJSON_GetObjectItem(jsonRoot, "filename");
        cJSON *sheetnameItem = cJSON_GetObjectItem(jsonRoot, "sheetname");
        if (!filenameItem || !sheetnameItem)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
        cJSON_AddStringToObject(results, "filename", removeFileExtension(std::string(filenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "sheetname", removeFileExtension(std::string(sheetnameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "filestatus", "processing");
        ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);
        cJSON_Delete(jsonRoot);
    }

    void renameworksheet(cJSON *results, const char *body)
    {
        // 重命名工作表需要请求体，检查body是否为空
        if (!body || strlen(body) == 0)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *jsonRoot = cJSON_Parse(body);
        if (!jsonRoot)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *filenameItem = cJSON_GetObjectItem(jsonRoot, "filename");
        cJSON *sheetnameItem = cJSON_GetObjectItem(jsonRoot, "sheetname");
        cJSON *newsheetnameItem = cJSON_GetObjectItem(jsonRoot, "newsheetname");
        if (!filenameItem || !sheetnameItem || !newsheetnameItem)
        {
            logger_log_error("error Missing filename, sheetname or newsheetname in renameworksheet data");
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
        cJSON_AddStringToObject(results, "filename", removeFileExtension(std::string(filenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "sheetname", removeFileExtension(std::string(sheetnameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "newsheetname", removeFileExtension(std::string(newsheetnameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "filestatus", "processing");
        ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);
        cJSON_Delete(jsonRoot);
    }

    void sheetlist(cJSON *results, const char *body)
    {
        cJSON *filenameItem = cJSON_GetObjectItem(cJSON_Parse(body), "filename");
        if (!filenameItem || !cJSON_IsString(filenameItem))
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        try
        {
            rtl::OUString filePathStr = convertStringToOUString(ensureOdsExtension(std::string(filenameItem->valuestring)).c_str());
            // 获取绝对路径
            rtl::OUString filePath;
            getAbsolutePath(filePathStr, filePath);
            // 首先判断是否文件已经加载
            // 使用DocumentManager打开文档
            std::string docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(filePath));
            if (docId.empty())
            {
                logger_log_error("Failed to load document: %s", filenameItem->valuestring);
                return;
            }

            // 获取文档对象
            auto docObj = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!docObj)
            {
                logger_log_error("Failed to get document object: %s", filenameItem->valuestring);
                return;
            }

            // 转换为SpreadsheetDocument类型
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(docObj);
            if (!spreadsheetDoc)
            {
                logger_log_error("Document is not a spreadsheet: %s", filenameItem->valuestring);
                return;
            }

            // 获取XSpreadsheetDocument接口
            uno::Reference<sheet::XSpreadsheetDocument> xDoc = spreadsheetDoc->getSpreadsheetDocument();
            if (!xDoc.is())
            {
                logger_log_error("Failed to get spreadsheet document interface: %s", filenameItem->valuestring);
                return;
            }

            if (!xDoc.is())
            {
                return;
            }

            uno::Reference<sheet::XSpreadsheets> xSheetsRaw = xDoc->getSheets();
            uno::Reference<container::XNameAccess> xSheets(xSheetsRaw, uno::UNO_QUERY);
            if (!xSheets.is())
            {
                ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
                return;
            }

            uno::Sequence<rtl::OUString> sheetNames = xSheets->getElementNames();
            cJSON *sheetArray = cJSON_CreateArray();
            rtl::OUString defaultfilePath, defaultFileName, wordsSheetName;
            getDefaultData(defaultfilePath, defaultFileName, wordsSheetName);
            for (sal_Int32 i = 0; i < sheetNames.getLength(); ++i)
            {
                // 过滤所有以wordsSheetName开头的工作表
                if (sheetNames[i].startsWith(wordsSheetName))
                {
                    continue;
                }
                cJSON_AddItemToArray(sheetArray, cJSON_CreateString(sheetNames[i].toUtf8().getStr()));
            }
            cJSON_AddItemToObject(results, "sheets", sheetArray);
        }
        catch (uno::Exception &e)
        {
            rtl::OUString errorMessage = e.Message;
            rtl::OString errorString = rtl::OUStringToOString(errorMessage, RTL_TEXTENCODING_UTF8);
            cJSON_AddStringToObject(results, "error", errorString.getStr());
        }
        ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);
    }

    // Optimized implementation for handling sheet data directly without queue processing
    void sheetdata(cJSON *results, const char *body)
    {
        // Check if the request body is empty
        if (!body || strlen(body) == 0)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        // Parse the input JSON string
        cJSON *jsonRoot = cJSON_Parse(body);
        if (!jsonRoot)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            cJSON_Delete(jsonRoot);
            return;
        }

        int status = filemanager::querysheetdata(jsonRoot, results);
        if (status != 0)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_FILE_INVALID_CONTENT);
            return;
        }
        ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);

        // Clean up
        cJSON_Delete(jsonRoot);
    }

    void renamefile(cJSON *results, const char *body)
    {
        // 重命名文件需要请求体，检查body是否为空
        if (!body || strlen(body) == 0)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *jsonRoot = cJSON_Parse(body);
        if (!jsonRoot)
        {
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
            return;
        }

        cJSON *oldFilenameItem = cJSON_GetObjectItem(jsonRoot, "oldFilename");
        cJSON *newFilenameItem = cJSON_GetObjectItem(jsonRoot, "newFilename");
        if (!oldFilenameItem || !newFilenameItem)
        {
            logger_log_error("error Missing oldFilename or newFilename in renamefile data");
            ErrorCodeManager::setErrorMessage(results, RESPONSE_INVALID_PARAMS);
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
        cJSON_AddStringToObject(results, "oldFilename", removeFileExtension(std::string(oldFilenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "newFilename", removeFileExtension(std::string(newFilenameItem->valuestring)).c_str());
        cJSON_AddStringToObject(results, "filestatus", "processing");
        ErrorCodeManager::setErrorMessage(results, RESPONSE_SUCCESS);
    }
} // namespace filemanager