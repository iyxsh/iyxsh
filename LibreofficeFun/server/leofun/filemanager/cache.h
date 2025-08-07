#ifndef CACHE_H
#define CACHE_H

#include <com/sun/star/uno/Exception.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/sheet/XSpreadsheets.hpp>
#include <com/sun/star/table/XCell.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>

#include <rtl/ustrbuf.hxx>
#include <rtl/ustring.hxx>

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

// cJSON
#include "../cJSON/cJSON.h"
#include "../logger/logger.h"
// 配置文件
#include "../config/json_config.h"
#include "utils.h"

using namespace com::sun::star;

namespace filemanager
{
    // 缓存条目结构
    struct CacheEntry {
        cJSON* data;
        time_t timestamp;
        time_t lastAccess;
        
        CacheEntry(cJSON* d, time_t t) : data(d), timestamp(t), lastAccess(t) {}
    };
    
    // 缓存加载状态
    enum class CacheLoadStatus {
        NOT_LOADED,
        LOADING,
        LOADED,
        ERROR
    };
    
    // 缓存管理器类
    class TemplateCacheManager {
    public:
        static TemplateCacheManager& getInstance();
        
        // 缓存相关函数
        void preloadTemplateData(const rtl::OUString &filePath, const rtl::OUString &sheetName);
        void initializeTemplateCache();
        void startFileMonitoring();
        void cleanupExpiredCache();
        void updateAllFilesWithTemplateData(const cJSON* templateData);
        void clearTemplateCache();
        void clearSheetDataCache();
        cJSON *getCachedSheetData(const rtl::OUString &filePath, const rtl::OUString &sheetName);
        
        // 状态检查函数
        bool isTemplateLoading() const;
        void setTemplateLoading(bool loading);
        
    private:
        TemplateCacheManager();
        ~TemplateCacheManager();
        
        // 禁止拷贝构造和赋值
        TemplateCacheManager(const TemplateCacheManager&) = delete;
        TemplateCacheManager& operator=(const TemplateCacheManager&) = delete;
        
        // 内部辅助函数
        std::string buildCacheKey(const rtl::OUString &filePath, const rtl::OUString &sheetName);
        bool isCacheEntryExpired(const std::string& cacheKey) const;
        void removeOldestCacheEntry();
        void waitForLoading(const std::string& cacheKey);
        void startLoading(const std::string& cacheKey);
        void finishLoading(const std::string& cacheKey, bool success = true);
        
        // 缓存数据
        std::unordered_map<std::string, std::unique_ptr<CacheEntry>> templateDataCache;
        std::unordered_map<std::string, CacheLoadStatus> templateDataCacheStatus;
        mutable std::mutex cacheMutex;
        
        // 加载状态相关
        std::atomic<bool> templateLoading{false};
        std::condition_variable loadingCondition;
        
        // 全局变量用于文件监控
        std::string g_defaultFilePathStr;
        rtl::OUString g_defaultFilePath;
        rtl::OUString g_wordsSheetName;
        
        // 监控线程
        std::unique_ptr<std::thread> monitorThread;
        std::atomic<bool> shouldStopMonitor{false};
    };
    
    // 为了向后兼容，保留原有的函数接口
    inline void preloadTemplateData(const rtl::OUString &filePath, const rtl::OUString &sheetName) {
        TemplateCacheManager::getInstance().preloadTemplateData(filePath, sheetName);
    }
    
    inline void initializeTemplateCache() {
        TemplateCacheManager::getInstance().initializeTemplateCache();
    }
    
    inline void startFileMonitoring() {
        TemplateCacheManager::getInstance().startFileMonitoring();
    }
    
    inline void cleanupExpiredCache() {
        TemplateCacheManager::getInstance().cleanupExpiredCache();
    }
    
    inline void updateAllFilesWithTemplateData(const cJSON* templateData) {
        TemplateCacheManager::getInstance().updateAllFilesWithTemplateData(templateData);
    }
    
    inline void clearTemplateCache() {
        TemplateCacheManager::getInstance().clearTemplateCache();
    }
    
    inline void clearSheetDataCache() {
        TemplateCacheManager::getInstance().clearSheetDataCache();
    }
    
    inline cJSON *getCachedSheetData(const rtl::OUString &filePath, const rtl::OUString &sheetName) {
        return TemplateCacheManager::getInstance().getCachedSheetData(filePath, sheetName);
    }
} // namespace filemanager

#endif // CACHE_H