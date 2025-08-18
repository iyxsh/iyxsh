#include "template_index_cache.h"
#include "utils.h"
#include "../config/json_config.h"
#include <sys/stat.h>
#include <thread>
#include <chrono>
#include <ctime>
#include "spreadsheet.h"
#include "fileops.h"

namespace filemanager
{
    bool TemplateIndexCacheManager::isLoading() const
    {
        return loading.load();
    }
    std::vector<TextCharInfo> TemplateIndexCacheManager::getCharacterInfos(const rtl::OUString &filePath, const rtl::OUString &sheetName)
    {
        std::string cacheKey = buildCacheKey(filePath, sheetName);
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = indexCache.find(cacheKey);
        if (it != indexCache.end() && it->second->index)
        {
            // 直接调用 CharacterIndex::getAll 获取所有索引内容
            return it->second->index->getAll();
        }
        return {};
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
        reloadTemplateIndex(filePath, sheetName);
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
        uno::Reference<lang::XComponent> xComp;
        try
        {
            uno::Reference<sheet::XSpreadsheetDocument> xDoc = loadSpreadsheetDocument(filePath, xComp);
            if (!xDoc.is())
            {
                closeDocument(xComp);
                loading.store(false);
                return;
            }
            uno::Reference<sheet::XSpreadsheet> sheet = getSheet(xDoc, sheetName);
            if (!sheet.is())
            {
                closeDocument(xComp);
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
            closeDocument(xComp);
        }
        catch (const std::exception &e)
        {
            closeDocument(xComp);
            logger_log_error("Exception in reloadTemplateIndex: %s", e.what());
        }
        catch (...)
        {
            closeDocument(xComp);
            logger_log_error("Unknown exception in reloadTemplateIndex");
        }
        loading.store(false);
    }

    void TemplateIndexCacheManager::monitorTemplateFile(const std::string &filePath, const std::string &sheetName)
    {
        std::thread([this, filePath, sheetName]()
                    {
        time_t lastModified = 0;
        while (true) {
            try {
                std::this_thread::sleep_for(std::chrono::seconds(30));
                struct stat fileStat;
                if (stat(filePath.c_str(), &fileStat) == 0) {
                    if (fileStat.st_mtime > lastModified) {
                        lastModified = fileStat.st_mtime;
                        rtl::OUString filePathOU = convertStringToOUString(filePath.c_str());
                        rtl::OUString sheetNameOU = convertStringToOUString(sheetName.c_str());
                        //重载前进行缓存备份到文件
                        int res = backupdefaultfile();
                        if(res != 0)
                        {
                            logger_log_error("backupdefaultfile failed!!!!!!!!!!!!!!!!!!!!!!!!!");
                        }
                        //重载默认模板文件
                        reloadTemplateIndex(filePathOU, sheetNameOU);
                        //重新更换当前所有文件的状态和模板数据
                        filemanager::initializeDataPathFiles();
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
