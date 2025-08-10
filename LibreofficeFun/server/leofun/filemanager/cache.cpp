#include "cache.h"
#include "lofficeconn.h"
#include "filequeue.h"
#include "spreadsheet.h"
#include "utils.h"
#include "../logger/logger.h"
#include "../config/json_config.h"
#include "../cJSON/cJSON.h"
#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <ctime>
#if __cplusplus >= 201703L
#include <filesystem>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace filemanager
{
    // 获取单例实例
    TemplateCacheManager& TemplateCacheManager::getInstance() {
        static TemplateCacheManager instance;
        return instance;
    }
    
    // 构造函数
    TemplateCacheManager::TemplateCacheManager() {
        // 初始化时清理过期缓存
        cleanupExpiredCache();
    }
    
    // 析构函数
    TemplateCacheManager::~TemplateCacheManager() {
        // 停止监控线程
        shouldStopMonitor = true;
        
        // 清理缓存
        clearTemplateCache();
    }
    
    // 构建缓存键
    std::string TemplateCacheManager::buildCacheKey(const rtl::OUString &filePath, const rtl::OUString &sheetName) {
        std::string filePathStr = rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr();
        std::string sheetNameStr = rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr();
        return filePathStr + "#" + sheetNameStr;
    }
    
    // 检查缓存条目是否过期
    bool TemplateCacheManager::isCacheEntryExpired(const std::string& cacheKey) const {
        const char* cacheExpiryTimeStr = json_config_get_string("cacheExpiryTime");
        const int CACHE_EXPIRY_TIME = cacheExpiryTimeStr ? atoi(cacheExpiryTimeStr) : 3600; // 默认1小时过期时间
        
        auto it = templateDataCache.find(cacheKey);
        if (it != templateDataCache.end()) {
            time_t currentTime = std::time(nullptr);
            return (currentTime - it->second->timestamp) > CACHE_EXPIRY_TIME;
        }
        return true;
    }
    
    // 移除最旧的缓存条目（LRU实现）
    void TemplateCacheManager::removeOldestCacheEntry() {
        if (templateDataCache.empty()) return;
        
        // 查找最久未访问的条目
        auto oldestIt = templateDataCache.begin();
        time_t oldestTime = oldestIt->second->lastAccess;
        
        for (auto it = templateDataCache.begin(); it != templateDataCache.end(); ++it) {
            if (it->second->lastAccess < oldestTime) {
                oldestIt = it;
                oldestTime = it->second->lastAccess;
            }
        }
        
        // 删除最旧的条目
        std::string oldestKey = oldestIt->first;
        cJSON_Delete(oldestIt->second->data);
        templateDataCache.erase(oldestIt);
        templateDataCacheStatus.erase(oldestKey);
        
        std::cout << "removeOldestCacheEntry: Removed oldest cache entry: " << oldestKey << std::endl;
    }
    
    // 等待加载完成
    void TemplateCacheManager::waitForLoading(const std::string& cacheKey) {
        std::unique_lock<std::mutex> lock(cacheMutex);
        loadingCondition.wait(lock, [this, &cacheKey]() {
            auto statusIt = templateDataCacheStatus.find(cacheKey);
            return statusIt == templateDataCacheStatus.end() || 
                   statusIt->second != CacheLoadStatus::LOADING;
        });
    }
    
    // 开始加载
    void TemplateCacheManager::startLoading(const std::string& cacheKey) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        templateDataCacheStatus[cacheKey] = CacheLoadStatus::LOADING;
    }
    
    // 完成加载
    void TemplateCacheManager::finishLoading(const std::string& cacheKey, bool success) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        templateDataCacheStatus[cacheKey] = success ? CacheLoadStatus::LOADED : CacheLoadStatus::ERROR;
        loadingCondition.notify_all();
    }
    
    // 获取缓存的表格数据
    cJSON *TemplateCacheManager::getCachedSheetData(const rtl::OUString &filePath, const rtl::OUString &sheetName) {
        std::string cacheKey = buildCacheKey(filePath, sheetName);
        
        // 检查是否正在加载
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            auto statusIt = templateDataCacheStatus.find(cacheKey);
            if (statusIt != templateDataCacheStatus.end() && 
                statusIt->second == CacheLoadStatus::LOADING) {
                std::cout << "getCachedSheetData: Data is currently loading for " << cacheKey << std::endl;
                return nullptr;
            }
        }
        
        // 检查缓存中是否有数据
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            auto it = templateDataCache.find(cacheKey);
            if (it != templateDataCache.end() && !isCacheEntryExpired(cacheKey)) {
                // 更新访问时间
                it->second->lastAccess = std::time(nullptr);
                std::cout << "getCachedSheetData: Found cached data for " << cacheKey << std::endl;
                return it->second->data;
            }
        }
        
        // 缓存中没有数据，启动预加载
        std::cerr << "getCachedSheetData: No cached data found for " << cacheKey << std::endl;
        
        // 在新线程中预加载数据
        try {
            std::thread preloadThread([this, filePath, sheetName]() {
                preloadTemplateData(filePath, sheetName);
            });
            preloadThread.detach();
        }
        catch (const std::system_error &e) {
            std::cerr << "getCachedSheetData: Failed to start preload thread: " << e.what() << std::endl;
            std::cerr << "Error code: " << e.code() << ", Error category: " << e.code().category().name() << std::endl;
        }
        catch (const std::exception &e) {
            std::cerr << "getCachedSheetData: Exception when starting preload thread: " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr << "getCachedSheetData: Unknown exception when starting preload thread" << std::endl;
        }
        
        return nullptr;
    }
    
    // 预加载模板数据
    void TemplateCacheManager::preloadTemplateData(const rtl::OUString &filePath, const rtl::OUString &sheetName) {
        std::cerr << "Preloading template data for " << convertOUStringToString(filePath) << std::endl;
        std::cerr << "Sheet name: " << convertOUStringToString(sheetName) << std::endl;
        std::string cacheKey = buildCacheKey(filePath, sheetName);
        
        // 检查是否已经在加载
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            auto statusIt = templateDataCacheStatus.find(cacheKey);
            if (statusIt != templateDataCacheStatus.end() && 
                statusIt->second == CacheLoadStatus::LOADING) {
                std::cerr << "Template is already loading..." << std::endl;
                // 已经在加载中
                return;
            }
        }
        
        // 标记为正在加载
        startLoading(cacheKey);
        
        // 设置模板正在加载状态
        try {
            setTemplateLoading(true);
        } catch (...) {
            std::cerr << "preloadTemplateData: Failed to set template loading status to true" << std::endl;
            // 即使设置失败也继续执行，但需要确保能正确清理状态
        }
        
        bool loadingSuccess = false;
        try {
            // 检查文件是否存在
            std::string filePathStr = rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr();
            struct stat fileStat;
            if (stat(filePathStr.c_str(), &fileStat) != 0) {
                std::cerr << "preloadTemplateData: File not found: " << filePathStr << std::endl;
                finishLoading(cacheKey, false);
                loadingSuccess = false;
            } else {
                time_t lastModified = fileStat.st_mtime;
                
                // 检查是否需要限制缓存大小
                const char* maxCacheSizeStr = json_config_get_string("maxTemplateCacheSize");
                size_t maxCacheSize = maxCacheSizeStr ? atoi(maxCacheSizeStr) : 100;
                
                {
                    std::lock_guard<std::mutex> lock(cacheMutex);
                    if (templateDataCache.size() >= maxCacheSize) {
                        std::cout << "preloadTemplateData: Cache size limit reached. Removing oldest entry." << std::endl;
                        removeOldestCacheEntry();
                    }
                }
                
                // 读取数据
                cJSON *data = readSheetData(filePath, sheetName);
                logger_log_info("data: %s", cJSON_PrintUnformatted(data));
                if (data) {
                    {
                        std::lock_guard<std::mutex> lock(cacheMutex);
                        // 如果已有旧缓存，先删除
                        auto it = templateDataCache.find(cacheKey);
                        if (it != templateDataCache.end()) {
                            cJSON_Delete(it->second->data);
                            templateDataCache.erase(it);
                        }
                        
                        // 缓存新数据
                        time_t currentTime = std::time(nullptr);
                        // 如果已有旧缓存状态，更新时间戳
                        auto statusIt = templateDataCacheStatus.find(cacheKey);
                        if (statusIt != templateDataCacheStatus.end()) {
                            statusIt->second = CacheLoadStatus::LOADED;
                        }
                        
                        templateDataCache[cacheKey] = std::make_unique<CacheEntry>(data, currentTime);
                    }
                    
                    std::cout << "preloadTemplateData: Successfully loaded and cached data for "
                              << cacheKey << " with " << cJSON_GetArraySize(data) << " items" << std::endl;
                    
                    // 创建更新模板任务并添加到队列
                    FileTask updateTemplateTask;
                    updateTemplateTask.type = TASK_UPDATE_TEMPLATE;
                    updateTemplateTask.filename = "template_update";
                    updateTemplateTask.data = cacheKey;
                    updateTemplateTask.createTime = std::time(nullptr);
                    updateTemplateTask.taskData = cJSON_Duplicate(data, cJSON_True); // 创建副本以避免内存问题
                    
                    try {
                        FileQueueManager::getInstance().addFileTask(updateTemplateTask);
                        finishLoading(cacheKey, true);
                        loadingSuccess = true;
                    } catch (const std::exception& e) {
                        std::cerr << "preloadTemplateData: Exception when adding template update task: " << e.what() << std::endl;
                        // 清理已分配的内存
                        cJSON_Delete(data);
                        if (updateTemplateTask.taskData) {
                            cJSON_Delete(static_cast<cJSON*>(updateTemplateTask.taskData));
                        }
                        finishLoading(cacheKey, false);
                        loadingSuccess = false;
                    } catch (...) {
                        std::cerr << "preloadTemplateData: Unknown exception when adding template update task" << std::endl;
                        // 清理已分配的内存
                        cJSON_Delete(data);
                        if (updateTemplateTask.taskData) {
                            cJSON_Delete(static_cast<cJSON*>(updateTemplateTask.taskData));
                        }
                        finishLoading(cacheKey, false);
                        loadingSuccess = false;
                    }
                } else {
                    std::cerr << "preloadTemplateData: Failed to load data for " << cacheKey << std::endl;
                    std::cerr << "This may be due to LibreOffice not running or file access issues." << std::endl;
                    finishLoading(cacheKey, false);
                    loadingSuccess = false;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "preloadTemplateData: Exception during data loading: " << e.what() << std::endl;
            finishLoading(cacheKey, false);
            loadingSuccess = false;
        } catch (...) {
            std::cerr << "preloadTemplateData: Unknown exception during data loading" << std::endl;
            finishLoading(cacheKey, false);
            loadingSuccess = false;
        }
        
        // 设置模板加载完成状态
        try {
            setTemplateLoading(false);
        } catch (...) {
            std::cerr << "preloadTemplateData: Failed to set template loading status to false" << std::endl;
        }
    }
    
    // 初始化模板缓存
    void TemplateCacheManager::initializeTemplateCache() {
        // 清理过期的缓存项
        cleanupExpiredCache();
        
        // 从配置文件中获取数据路径和默认文件名
        const char *datapath = json_config_get_string("datapath");
        const char *defaultname = json_config_get_string("defaultname");
        const char *wordsSheetNameConfig = json_config_get_string("WordsSheet");
        
        if (!datapath) datapath = "../data"; // 默认数据路径
        if (!defaultname) defaultname = "default.xlsx"; // 默认文件名
        
        // 默认工作表名
        const char *defaultSheetName = wordsSheetNameConfig ? wordsSheetNameConfig : "WordsSheet";
        
        // 构建默认文件的完整路径
        std::string defaultFilePathStr = std::string(datapath) + "/" + std::string(defaultname);
        
        // 处理相对路径，转换为绝对路径
        std::string absoluteDefaultFilePathStr = convertToAbsolutePath(defaultFilePathStr);
        
        rtl::OString absoluteDefaultFilePathOStr = rtl::OString(absoluteDefaultFilePathStr.c_str());
        rtl::OUString defaultFilePath = rtl::OStringToOUString(absoluteDefaultFilePathOStr, RTL_TEXTENCODING_UTF8);
        rtl::OUString wordsSheetName = rtl::OUString::createFromAscii(defaultSheetName);
        
        // 保存全局变量供监控线程使用
        g_defaultFilePathStr = absoluteDefaultFilePathStr;
        g_defaultFilePath = defaultFilePath;
        g_wordsSheetName = wordsSheetName;
        
        // 检查文件是否存在
        struct stat fileStat;
        if (stat(absoluteDefaultFilePathStr.c_str(), &fileStat) != 0) {
            std::cerr << "initializeTemplateCache: Template file not found: " << absoluteDefaultFilePathStr << std::endl;
            std::cerr << "Please make sure the template file exists at the specified path." << std::endl;
            return;
        }
        
        // 预加载默认模板数据
        std::cout << "Initializing template cache with default template: " << absoluteDefaultFilePathStr << std::endl;
        preloadTemplateData(defaultFilePath, wordsSheetName);
        
        // 等待首次加载完成，最多等待15秒
        std::string cacheKey = buildCacheKey(defaultFilePath, wordsSheetName);
        int waitCount = 0;
        bool loadingCompleted = false;
        
        while (waitCount < 150) { // 最多等待15秒 (150 * 100ms)
            {
                std::lock_guard<std::mutex> lock(cacheMutex);
                auto statusIt = templateDataCacheStatus.find(cacheKey);
                if (statusIt == templateDataCacheStatus.end() || 
                    statusIt->second != CacheLoadStatus::LOADING) {
                    loadingCompleted = true;
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            waitCount++;
        }
        
        if (!loadingCompleted) {
            std::cerr << "initializeTemplateCache: Timeout waiting for template loading to complete" << std::endl;
        }
        
        // 检查是否成功加载
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            auto it = templateDataCache.find(cacheKey);
            if (it != templateDataCache.end()) {
                std::cout << "Template cache initialized successfully with "
                          << cJSON_GetArraySize(it->second->data) << " items" << std::endl;
            } else {
                std::cerr << "Template cache initialization failed" << std::endl;
                std::cerr << "This may be due to LibreOffice not running or file access issues." << std::endl;
                std::cerr << "The data will be loaded on-demand when needed." << std::endl;
                std::cerr << "Please ensure LibreOffice is running with: soffice --headless --accept=\"socket,host=127.0.0.1,port=2002;urp;\"" << std::endl;
            }
        }
        
        // 启动文件监控线程
        startFileMonitoring();
    }
    
    // 启动文件监控
    void TemplateCacheManager::startFileMonitoring() {
        try {
            shouldStopMonitor = false;
            monitorThread = std::make_unique<std::thread>([this]() {
                // 每30秒检查一次文件变化
                while (!shouldStopMonitor) {
                    try {
                        std::this_thread::sleep_for(std::chrono::seconds(30));
                        
                        // 检查文件是否存在
                        struct stat fileStat;
                        if (stat(g_defaultFilePathStr.c_str(), &fileStat) == 0) {
                            time_t lastModified = fileStat.st_mtime;
                            
                            // 构建缓存键
                            std::string cacheKey = buildCacheKey(g_defaultFilePath, g_wordsSheetName);
                            
                            // 检查缓存中的时间戳
                            bool needUpdate = false;
                            {
                                std::lock_guard<std::mutex> lock(cacheMutex);
                                auto it = templateDataCache.find(cacheKey);
                                if (it != templateDataCache.end()) {
                                    std::cout << "File modification time: " << lastModified 
                                              << ", Cache timestamp: " << it->second->timestamp << std::endl;
                                    if (it->second->timestamp < lastModified) {
                                        // 文件已修改，需要更新缓存
                                        std::cout << "Template file changed, updating cache: " << g_defaultFilePathStr << std::endl;
                                        needUpdate = true;
                                        // 更新缓存中的时间戳，避免重复触发更新
                                        it->second->timestamp = lastModified;
                                    } else {
                                        //std::cout << "Template file not changed, no update needed" << std::endl;
                                    }
                                } else {
                                    std::cout << "No cache entry found for template file, triggering initial load" << std::endl;
                                    needUpdate = true;
                                }
                            }
                                                    
                            // 如果需要更新，则重新加载模板数据
                            if (needUpdate) {
                                std::cout << "Triggering preloadTemplateData due to file change or initial load" << std::endl;
                                preloadTemplateData(g_defaultFilePath, g_wordsSheetName);
                            }
                        } else {
                            std::cerr << "Template file no longer exists: " << g_defaultFilePathStr << std::endl;
                        }
                    }
                    catch (const std::exception &e) {
                        std::cerr << "Exception in file monitoring thread: " << e.what() << std::endl;
                    }
                    catch (...) {
                        std::cerr << "Unknown exception in file monitoring thread" << std::endl;
                    }
                }
            });
            
            std::cout << "File monitoring thread started for: " << g_defaultFilePathStr << std::endl;
        } catch (const std::system_error& e) {
            std::cerr << "startFileMonitoring: Failed to start file monitoring thread: " << e.what() << std::endl;
            std::cerr << "Error code: " << e.code() << ", Error category: " << e.code().category().name() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "startFileMonitoring: Exception starting file monitoring thread: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "startFileMonitoring: Unknown exception starting file monitoring thread" << std::endl;
        }
    }
    
    // 清理过期缓存
    void TemplateCacheManager::cleanupExpiredCache() {
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        // 查找所有过期的缓存项
        std::vector<std::string> expiredKeys;
        for (const auto &pair : templateDataCache) {
            if (isCacheEntryExpired(pair.first)) {
                expiredKeys.push_back(pair.first);
            }
        }
        
        // 删除所有过期的缓存项
        for (const std::string &cacheKey : expiredKeys) {
            auto it = templateDataCache.find(cacheKey);
            if (it != templateDataCache.end()) {
                cJSON_Delete(it->second->data);
                templateDataCache.erase(it);
            }
            templateDataCacheStatus.erase(cacheKey);
            std::cout << "cleanupExpiredCache: Removed expired cache entry: " << cacheKey << std::endl;
        }
    }
    
    // 更新所有文件的模板数据
    void TemplateCacheManager::updateAllFilesWithTemplateData(const cJSON* templateData) {
        logger_log_info("Starting update of all files with template data");
        
        try {
            rtl::OUString defaultFilePath, wordsSheetName;
            getDefaultData(defaultFilePath, wordsSheetName);

            // 获取默认模板文件名
            const char* defaultTemplateName = json_config_get_string("defaultname");
            if (!defaultTemplateName) {
                defaultTemplateName = "default.xlsx"; // 默认模板文件名
            }
            std::string defaultTemplateFile(defaultTemplateName);

            // 遍历所有文件状态并更新就绪状态的文件
            std::unordered_map<std::string, FileInfo> fileStatusMap = FileQueueManager::getInstance().getFileStatusMapCopy();

            int updatedFiles = 0;
            int failedFiles = 0;
            
            for (const auto& fileEntry : fileStatusMap) {
                const std::string& filename = fileEntry.first;
                const FileInfo& fileInfo = fileEntry.second;

                // 只处理就绪状态的文件，并排除默认模板文件
                if (fileInfo.status == FILE_STATUS_READY && filename != defaultTemplateFile) {
                    logger_log_info("Updating WordsSheet for file: %s", filename.c_str());
                    
                    try {
                        // 更新文件中的WordsSheet
                        rtl::OUString fileNameOUString = convertStringToOUString(ensureXlsxExtension(filename).c_str());
                        rtl::OUString absoluteFilePath;
                        getAbsolutePath(fileNameOUString, absoluteFilePath);
                        if(!cJSON_IsObject(templateData))
                        {
                            logger_log_error("templateData is not an object: %s", cJSON_PrintUnformatted(const_cast<cJSON*>(templateData)));
                            continue;
                        }
                        cJSON* result = batchUpdateSpreadsheetContent(absoluteFilePath, wordsSheetName, templateData);
                        
                        if (!result) {
                            logger_log_warn("Failed to update WordsSheet for file: %s", filename.c_str());
                            failedFiles++;
                        } else {
                            cJSON_Delete(result); // 清理返回的结果对象
                            updatedFiles++;
                        }
                    } catch (const uno::Exception& e) {
                        logger_log_error("UNO Exception updating WordsSheet for file %s: %s", 
                                  filename.c_str(), 
                                  rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
                        failedFiles++;
                    } catch (const std::exception& e) {
                        logger_log_error("Exception updating WordsSheet for file %s: %s", 
                                  filename.c_str(), 
                                  e.what());
                        failedFiles++;
                    } catch (...) {
                        logger_log_error("Unknown exception updating WordsSheet for file: %s", filename.c_str());
                        failedFiles++;
                    }
                }
            }
            
            logger_log_info("Completed update of all files with template data. Updated: %d, Failed: %d", 
                          updatedFiles, failedFiles);
        } catch (const std::exception& e) {
            logger_log_error("Exception in updateAllFilesWithTemplateData: %s", e.what());
        } catch (...) {
            logger_log_error("Unknown exception in updateAllFilesWithTemplateData");
        }
        
        logger_log_info("Completed update of all files with template data");
    }
    
    // 清理模板缓存
    void TemplateCacheManager::clearTemplateCache() {
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        // 删除所有缓存的数据
        for (auto &pair : templateDataCache) {
            cJSON_Delete(pair.second->data);
        }
        templateDataCache.clear();
        templateDataCacheStatus.clear();
        
        std::cout << "clearTemplateCache: All template cache cleared" << std::endl;
    }
    
    // 清理工作表数据缓存
    void TemplateCacheManager::clearSheetDataCache() {
        // 注意：目前模板缓存和工作表数据缓存使用的是相同的数据结构
        // 因此暂时不实现单独清理工作表数据缓存的逻辑
        // 如果将来有需要，可以考虑分离这两个缓存结构
        std::cout << "clearSheetDataCache: Sheet data cache cleared (currently shares data with template cache)" << std::endl;
    }
    
    // 模板加载状态管理
    bool TemplateCacheManager::isTemplateLoading() const {
        return templateLoading.load();
    }

    void TemplateCacheManager::setTemplateLoading(bool loading) {
        templateLoading.store(loading);
        // 通知所有等待的线程状态已改变
        try {
            loadingCondition.notify_all();
        } catch (...) {
            std::cerr << "setTemplateLoading: Exception when notifying condition variable" << std::endl;
        }
    }
}