#ifndef FILEMANAGER_H
#define FILEMANAGER_H

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
#include <com/sun/star/bridge/UnoUrlResolver.hpp>
#include <cppuhelper/bootstrap.hxx>
#include <rtl/ustrbuf.hxx>
#include <rtl/ustring.hxx>
#include <string>
#include <memory>
#include <vector>
// cJSON
#include "../cJSON/cJSON.h"
#include "../logger/logger.h"
// 配置文件
#include "../config/json_config.h"
// UNO C++ 头文件
#include <com/sun/star/uno/Reference.hxx>
#include <rtl/ustring.hxx>
using namespace com::sun::star;

namespace com
{
    namespace sun
    {
        namespace star
        {
            namespace uno
            {
                class XInterface;
                class XComponentContext;
            }
            namespace frame
            {
                class XComponentLoader;
            }
            namespace sheet
            {
                class XSpreadsheet;
                class XSpreadsheetDocument;
            }
        }
    }
}

namespace filemanager
{
    // 列索引与Excel列名转换
    std::string columnIndexToName(int columnIndex);

    /// @brief 读取单元格内容为JSON
    cJSON *readCellToJson(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &sheet,
                          const rtl::OUString &cellAddress);

    /// @brief 保存文档到指定路径
    void saveDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &doc,
                      const rtl::OUString &filePath);

    /// @brief 关闭文档
    void closeDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &docInterface);

    /// @brief 读取整个电子表格文件为JSON
    cJSON *readSpreadsheetFile(const rtl::OUString &filePath);

    /// @brief 创建新电子表格文件
    cJSON *createNewSpreadsheetFile(const rtl::OUString &filePath,
                                    const rtl::OUString &sheetName,
                                    const cJSON *contentData = nullptr);

    /// @brief 更新电子表格内容
    cJSON *updateSpreadsheetContent(const rtl::OUString &filePath,
                                    const rtl::OUString &sheetName,
                                    const rtl::OUString &cellAddress,
                                    const rtl::OUString &newValue,
                                    const rtl::OUString &cellType);

    /// @brief 获取工作表
    com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet> getSheet(
        const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument> &document,
        const rtl::OUString &sheetName);

    // 连接管理器类
    // LibreOffice连接管理器实现
    class LibreOfficeConnectionManager
    {
    public:
        static bool initialize();
        static void release();
        static uno::Reference<frame::XComponentLoader> getComponentLoader();
        static uno::Reference<uno::XComponentContext> getContext();

    private:
        static uno::Reference<uno::XComponentContext> mContext;
        static uno::Reference<frame::XComponentLoader> mLoader;
        static bool mIsConnected;
    };

    void newfileCreate(cJSON *results, const char *body);
    void updatefile(cJSON *results, const char *body);
    void editfile(cJSON *results, const char *body);
    void findInSheet(cJSON *results, const char *body);
    void readSheetContents(cJSON *results, const char *body);

    // 内部辅助函数
    cJSON *findValueInSheet(const rtl::OUString &filePath, const rtl::OUString &sheetName, const rtl::OUString &searchValue);
    cJSON *readSheetData(const rtl::OUString &filePath, const rtl::OUString &sheetName);
    cJSON *getCachedSheetData(const rtl::OUString &filePath, const rtl::OUString &sheetName);
    rtl::OUString findCharPositions(const rtl::OUString &newValue, cJSON *sheetData);
    void preloadTemplateData(const rtl::OUString &filePath, const rtl::OUString &sheetName);
    void initializeTemplateCache();
    void startFileMonitoring();  // 添加文件监控函数声明

    // 静态缓存变量
    extern std::unordered_map<std::string, cJSON *> templateCache;
    extern std::unordered_map<std::string, cJSON *> templateDataCache;
    extern std::unordered_map<std::string, time_t> templateFileTimestamps;
    // 缓存预加载状态
    extern std::unordered_map<std::string, bool> templateDataCacheLoading;
    // 缓存限制
    const size_t MAX_TEMPLATE_CACHE_SIZE = 100;
    const size_t MAX_SHEET_DATA_CACHE_SIZE = 50;

    // 批量更新函数
    cJSON *batchUpdateSpreadsheetContent(const rtl::OUString &filePath,
                                         const rtl::OUString &sheetName,
                                         const cJSON *updateData);

    // 缓存相关函数
    cJSON *getCachedTemplate(const std::string &templateKey);
    void cacheTemplate(const std::string &templateKey, cJSON *templateData);
    void clearTemplateCache();
    void clearSheetDataCache();
} // namespace filemanager
#endif // FILEMANAGER_H