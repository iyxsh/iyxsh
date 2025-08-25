#include "template_index_cache.h"
#include "utils.h"
#include "../config/json_config.h"
#include <sys/stat.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <dirent.h>
#include "spreadsheet.h"
#include "filequeue.h"
#include "../error/error_codes.h"

namespace filemanager
{
    bool TemplateIndexCacheManager::isLoading() const
    {
        return loading.load();
    }
    std::shared_ptr<CharacterIndex> TemplateIndexCacheManager::getCharacterInfos(const rtl::OUString &filePath, const rtl::OUString &sheetName)
    {
        std::string cacheKey = buildCacheKey(filePath, sheetName);
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = indexCache.find(cacheKey);
        if (it != indexCache.end() && it->second->index)
        {
            // 直接调用 CharacterIndex::getAll 获取所有索引内容
            return it->second->index;
        }
        return nullptr;
    }
    TemplateIndexCacheManager &TemplateIndexCacheManager::getInstance()
    {
        static TemplateIndexCacheManager instance;
        return instance;
    }

    std::string TemplateIndexCacheManager::buildCacheKey(const rtl::OUString &filePath, const rtl::OUString &sheetName)
    {
        std::string filePathStr = rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr();
        std::string sheetNameStr = rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr();
        return filePathStr + "#" + sheetNameStr;
    }

    bool TemplateIndexCacheManager::isCacheEntryExpired(const std::string &cacheKey) const
    {
        const char *cacheExpiryTimeStr = json_config_get_string("cacheExpiryTime");
        const int CACHE_EXPIRY_TIME = cacheExpiryTimeStr ? atoi(cacheExpiryTimeStr) : 3600;
        auto it = indexCache.find(cacheKey);
        if (it != indexCache.end())
        {
            time_t currentTime = std::time(nullptr);
            return (currentTime - it->second->timestamp) > CACHE_EXPIRY_TIME;
        }
        return true;
    }

    void TemplateIndexCacheManager::removeOldestCacheEntry()
    {
        if (indexCache.empty())
            return;
        auto oldestIt = indexCache.begin();
        time_t oldestTime = oldestIt->second->lastAccess;
        for (auto it = indexCache.begin(); it != indexCache.end(); ++it)
        {
            // 优化：仅淘汰无外部引用的索引
            if (it->second->lastAccess < oldestTime && (!it->second->index || it->second->index.use_count() <= 1))
            {
                oldestIt = it;
                oldestTime = it->second->lastAccess;
            }
        }
        // 仅当无外部引用时才删除
        if (!oldestIt->second->index || oldestIt->second->index.use_count() <= 1)
        {
            indexCache.erase(oldestIt);
        }
    }

    int TemplateIndexCacheManager::backupdefaultfile()
    {
        // 生成唯一文件名
        time_t now = time(0);
        tm *ltm = localtime(&now);

        char filename[100];
        sprintf(filename, "default_%04d%02d%02d_%02d%02d%02d",
                1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday,
                ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
        logger_log_info("filename: %s", filename);

        // 创建文件
        rtl::OUString filePath;
        rtl::OUString defaultFileName;
        rtl::OUString sheetName;
        getDefaultData(filePath, defaultFileName, sheetName);
        // 备份在 default目录下
        rtl::OUString absoluteFilePath = filePath + convertStringToOUString(ensureOdsExtension(std::string(filename)).c_str());
        // 从缓存索引获取数据
        std::shared_ptr<CharacterIndex> idxPtr = filemanager::TemplateIndexCacheManager::getInstance().getCharacterInfos(filePath + defaultFileName, sheetName);
        // 启动时没有缓存，此处为空
        if (idxPtr && !idxPtr->isEmpty())
        {
            std::vector<TextCharInfo> infos = idxPtr->getAll(); // 获取所有数据
            if (infos.empty())
            {
                logger_log_warn("Character index data is empty");
                return -1;
            }
            // 首先创建个空文档 默认工作表名
            cJSON *newCreate = createNewSpreadsheetFile(absoluteFilePath, sheetName);
            if (newCreate == nullptr)
            {
                logger_log_error("Failed to create new spreadsheet file: %s", filename);
                return -1;
            }
            // 使用DocumentManager打开文档获取docId
            std::string docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(absoluteFilePath));
            if (docId.empty())
            {
                logger_log_error("Failed to open document: %s", convertOUStringToString(absoluteFilePath).c_str());
                return -1;
            }
            
            if (!writeCharacterInfosToSheet(docId, sheetName, infos))
            {
                filemanager::DocumentManager::getInstance().closeDocument(docId);
                logger_log_error("Failed to write character infos to sheet");
                return -1;
            }
            
            // 关闭文档
            filemanager::DocumentManager::getInstance().closeDocument(docId);
            {
                logger_log_error("Failed to write character infos to sheet");
                return -1;
            }
        }
        else
        {
            logger_log_error("Failed to get character infos from cache: idxPtr is null");
        }
        return 0;
    }
    //缓存新加保存机制，与备份功能相反
    int TemplateIndexCacheManager::setnewTodefaultfile()
    {
        // 生成唯一文件名
        time_t now = time(0);
        tm *ltm = localtime(&now);

        char filename[100];
        sprintf(filename, "newdefault_%04d%02d%02d_%02d%02d%02d",
                1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday,
                ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
        logger_log_info("filename: %s", filename);

        // 创建文件
        rtl::OUString filePath;
        rtl::OUString defaultFileName;
        rtl::OUString sheetName;
        getDefaultData(filePath, defaultFileName, sheetName);
        // 备份在 default目录下
        rtl::OUString absoluteFilePath = filePath + convertStringToOUString(ensureOdsExtension(std::string(filename)).c_str());
        // 从缓存索引获取数据
        std::shared_ptr<CharacterIndex> idxPtr = filemanager::TemplateIndexCacheManager::getInstance().getCharacterInfos(filePath + defaultFileName, sheetName);
        // 启动时没有缓存，此处为空
        if (idxPtr && !idxPtr->isEmpty())
        {
            std::vector<TextCharInfo> infos = idxPtr->getAll(); // 获取所有数据
            if (infos.empty())
            {
                logger_log_warn("Character index data is empty");
                return -1;
            }
            // 首先创建个空文档 默认工作表名
            cJSON *newCreate = createNewSpreadsheetFile(absoluteFilePath, sheetName);
            if (newCreate == nullptr)
            {
                logger_log_error("Failed to create new spreadsheet file: %s", filename);
                return -1;
            }
            // 使用DocumentManager打开文档获取docId
            std::string docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(absoluteFilePath));
            if (docId.empty())
            {
                logger_log_error("Failed to open document: %s", convertOUStringToString(absoluteFilePath).c_str());
                return -1;
            }
            
            if (!writeCharacterInfosToSheet(docId, sheetName, infos))
            {
                filemanager::DocumentManager::getInstance().closeDocument(docId);
                logger_log_error("Failed to write character infos to sheet");
                return -1;
            }
            
            // 关闭文档
            filemanager::DocumentManager::getInstance().closeDocument(docId);
            {
                logger_log_error("Failed to write character infos to sheet");
                return -1;
            }
        }
        else
        {
            logger_log_error("Failed to get character infos from cache: idxPtr is null");
        }
        return 0;
    }
    

    void TemplateIndexCacheManager::initializeDataPathFiles()
    {
        // 读取datapath目录下所有文件并加入队列
        const char *datapath = json_config_get_string("datapath");
        const char *workpathname = json_config_get_string("workpathname");
        std::string filepath = std::string(datapath) + "/" + std::string(workpathname);
        if (filepath.length() > 0)
        {
            DIR *dir = opendir(filepath.c_str());
            if (dir)
            {
                logger_log_info("Scanning directory: %s", filepath.c_str());
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
                        std::string filepath = std::string(datapath) + "/" + std::string(workpathname) + "/" + filename;

                        // 获取文件状态
                        struct stat fileStat;
                        if (stat(filepath.c_str(), &fileStat) == 0)
                        {
                            // 创建文件任务
                            filemanager::FileTask task;
                            task.type = filemanager::TASK_UPDATE_TEMPLATE;
                            task.filename = removeFileExtension(filename);
                            task.createTime = std::time(nullptr);
                            filemanager::FileQueueManager::getInstance().addFileStatus(task.filename, filemanager::FILE_STATUS_PROCESSING);
                            filemanager::FileQueueManager::getInstance().addFileTask(task);
                            logger_log_info("Added existing file to queue: %s with mtime: %ld", filename.c_str(), (long)fileStat.st_mtime);
                        }
                        else
                        {
                            logger_log_warn("Failed to get file status for: %s", filepath.c_str());
                        }
                    }
                }
                closedir(dir);
                logger_log_info("Finished scanning directory: %s", filepath.c_str());
            }
            else
            {
                logger_log_warn("Failed to open datapath directory: %s", filepath.c_str());
            }
        }
        else
        {
            int errorCode = RESPONSE_FILE_PATH_NOT_FOUND; // 错误码宏定义
            std::string errorMessage = ErrorCodeManager::getErrorMessage(errorCode);
            logger_log_warn("%s", errorMessage.c_str());
            return;
        }
    }

    std::shared_ptr<CharacterIndex> TemplateIndexCacheManager::getTemplateIndex(const rtl::OUString &filePath, const rtl::OUString &sheetName)
    {
        std::string cacheKey = buildCacheKey(filePath, sheetName);
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            auto it = indexCache.find(cacheKey);
            if (it != indexCache.end() && !isCacheEntryExpired(cacheKey))
            {
                it->second->lastAccess = std::time(nullptr);
                return it->second->index;
            }
        }
        struct stat fileStat;
        if (stat(convertOUStringToString(filePath).c_str(), &fileStat) == 0) {
            setLastModified(fileStat.st_mtime);
            reloadTemplateIndex(filePath, sheetName);
            //重新更换当前所有文件的状态和模板数据
            initializeDataPathFiles();
        } else {
            logger_log_error("stat failed for file %s", convertOUStringToString(filePath).c_str());
            return nullptr;
        }
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = indexCache.find(cacheKey);
        if (it != indexCache.end())
        {
            it->second->lastAccess = std::time(nullptr);
            return it->second->index;
        }
        return nullptr;
    }

    void TemplateIndexCacheManager::reloadTemplateIndex(const rtl::OUString &filePath, const rtl::OUString &sheetName)
    {
        loading.store(true);
        std::string cacheKey = buildCacheKey(filePath, sheetName);
        try
        {
            logger_log_info("Reloading template index for %s %s .....................", convertOUStringToString(filePath).c_str(), convertOUStringToString(sheetName).c_str());
            // 首先判断是否文件已经加载
            // 使用DocumentManager打开文档获取docId
            std::string docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(filePath));
            if (docId.empty())
            {
                logger_log_error("Failed to load document: %s", convertOUStringToString(filePath).c_str());
                loading.store(false);
                return;
            }

            uno::Reference<sheet::XSpreadsheet> sheet = getSheet(docId, sheetName);
            if (!sheet.is())
            {
                filemanager::DocumentManager::getInstance().closeDocument(docId);
                loading.store(false);
                return;
            }
            std::vector<LanguageGroup> groups = readSheetAndGroupByLanguage(sheet);
            auto indexPtr = std::make_shared<CharacterIndex>();
            indexPtr->build(groups);
            time_t now = std::time(nullptr);
            {
                std::lock_guard<std::mutex> lock(cacheMutex);
                if (indexCache.size() >= 100)
                    removeOldestCacheEntry();
                indexCache[cacheKey] = std::make_unique<TemplateIndexEntry>(indexPtr, now);
            }
            // 关闭文档
            filemanager::DocumentManager::getInstance().closeDocument(docId);
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in reloadTemplateIndex: %s", e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in reloadTemplateIndex");
        }
        loading.store(false);
    }

    time_t TemplateIndexCacheManager::getLastModified() const
    {
        return lastModified;
    }

    void TemplateIndexCacheManager::setLastModified(time_t lastModified)
    {
        TemplateIndexCacheManager::lastModified = lastModified;
    }

    void TemplateIndexCacheManager::monitorTemplateFile(const std::string &filePath, const std::string &sheetName)
    {
        std::thread([this, filePath, sheetName]()
                    {
        lastModified = getLastModified();
        while (true) {
            try {
                std::this_thread::sleep_for(std::chrono::seconds(30));
                struct stat fileStat;
                if (stat(filePath.c_str(), &fileStat) == 0) {
                    if (fileStat.st_mtime > lastModified) {
                        //启动时没有缓存，不进行备份，只有重载才进行备份
                        if(lastModified != 0)
                        {
                            //重载前进行缓存备份到文件
                            int res = backupdefaultfile();
                            if(res != 0)
                            {
                                logger_log_error("backupdefaultfile failed!!!!!!!!!!!!!!!!!!!!!!!!!");
                            }
                        }
                        else
                        {
                            logger_log_info("First run not backup!!!!!");
                        }
                        setLastModified(fileStat.st_mtime);
                        rtl::OUString filePathOU = convertStringToOUString(filePath.c_str());
                        rtl::OUString sheetNameOU = convertStringToOUString(sheetName.c_str());
                        //重载默认模板文件
                        reloadTemplateIndex(filePathOU, sheetNameOU);
                        //重新更换当前所有文件的状态和模板数据
                        initializeDataPathFiles();
                    }
                }
            } catch (const std::exception& e) {
                logger_log_error("Exception in monitorTemplateFile thread: %s", e.what());
            } catch (...) {
                logger_log_error("Unknown exception in monitorTemplateFile thread");
            }
        } })
            .detach();
    }
} // namespace filemanager
