#ifndef SPREADSHEET_H
#define SPREADSHEET_H

#include <com/sun/star/uno/Exception.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/sheet/XSpreadsheets.hpp>
#include <com/sun/star/sheet/XSheetCellRanges.hpp>
#include <com/sun/star/sheet/XSheetCellCursor.hpp>
#include <com/sun/star/sheet/XArrayFormulaRange.hpp>
#include <com/sun/star/sheet/XCellRangesQuery.hpp>
#include <com/sun/star/sheet/CellFlags.hpp>
#include <com/sun/star/table/XCellRange.hpp>
#include <com/sun/star/table/XCell.hpp>
#include <com/sun/star/table/XColumnRowRange.hpp>
#include <com/sun/star/table/CellRangeAddress.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/ucb/XSimpleFileAccess.hpp>
#include <rtl/ustrbuf.hxx>
#include <rtl/ustring.hxx>
#include <osl/file.hxx>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

// cJSON
#include "../cJSON/cJSON.h"
#include "../logger/logger.h"
// 配置文件
#include "../config/json_config.h"
#include "utils.h"
#include "template_index_cache.h"
#include "DocumentManager.h"

using namespace com::sun::star;

namespace filemanager
{
    /// @brief 读取单元格内容为JSON
    cJSON *readCellToJson(const std::string &docId,
                          const rtl::OUString &sheetName,
                          const rtl::OUString &cellAddress);
    /// @brief 保存文档到指定路径
    bool saveDocument(const std::string &docId,
                      const rtl::OUString &filePath);

    /// @brief 关闭文档
    bool closeDocument(const std::string &docId);

    /// @brief 创建新电子表格文件
    cJSON *createNewSpreadsheetFile(const rtl::OUString &filePath,
                                    const rtl::OUString &sheetName);

    /// @brief 更新电子表格内容
    cJSON *updateSpreadsheetContent(const std::string &docId,
                                    const rtl::OUString &sheetName,
                                    const rtl::OUString &cellAddress,
                                    const rtl::OUString &newValue,
                                    const rtl::OUString &cellType);

    /// @brief 获取工作表
    com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet> getSheet(
        const std::string &docId,
        const rtl::OUString &sheetName);

    // 批量更新函数
    cJSON *batchUpdateSpreadsheetContent(const std::string &docId,
                                         const rtl::OUString &sheetName,
                                         const cJSON *updateData);

    // 内部辅助函数
    std::string findValueInSheet(const std::string &docId, const rtl::OUString &sheetName, const rtl::OUString &searchValue);
    cJSON *readSheetData(const std::string &docId, const rtl::OUString &sheetName);

    // 公共函数用于加载文档
    std::string loadSpreadsheetDocument(const rtl::OUString &filename);
    // 获取工作表总记录数
    int getTotalRecordCount(const std::string &docId, const rtl::OUString &sheetName);
    // 分页读取工作表内容
    cJSON *readSheetDataRange(const std::string &docId, const rtl::OUString &sheetName, int startIndex, int endIndex);
    // 内部函数：清空工作表内容
    void clearSheet(const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet> &sheet);
    // 公共函数：清空指定文档和工作表的内容
    void clearSheet(const std::string &docId, const rtl::OUString &sheetName);
    // 声明用于查找字符串位置的函数
    std::string findStringInSpreadsheet(const rtl::OUString &targetString, com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet> sheet);
    std::string findStringInDocIdSpreadsheet(const std::string &docId, const rtl::OUString &sheetName, const rtl::OUString &targetString);
    
    // 内部函数：从工作表读取所有单元格内容，按语言分组返回
    std::vector<LanguageGroup> readSheetAndGroupByLanguage(com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet> sheet);
    // 公共函数：从指定文档和工作表读取所有单元格内容，按语言分组返回
    std::vector<LanguageGroup> readSheetAndGroupByLanguage(const std::string &docId, const rtl::OUString &sheetName);

    // 主处理函数：拆分文本，分类，查找位置，返回每个字符的详细信息
    std::vector<TextCharInfo> splitAndClassifyText(const std::string &text, com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet> sheet);
    
    // 批量写入 CharacterInfo 到指定文件和工作表
    bool writeCharacterInfosToSheet(const std::string &docId,
                                    const rtl::OUString &sheetName,
                                    const std::vector<TextCharInfo> &infos);
    std::vector<TextCharInfo> splitAndClassifyTextFromIndex(const std::string& text, std::shared_ptr<CharacterIndex> index,const std::string &docId,const rtl::OUString &sheetName);
    rtl::OUString infosToFormatString(const std::vector<TextCharInfo> &infos);
    
    // 通过UNO API删除文件，确保连接状态与文件系统一致
    bool DeleteFileWithUNO(const rtl::OUString &filePath);
    
    // 安全删除文件，调用UNO API删除并处理异常
    bool SafeDeleteFile(const rtl::OUString &filePath);
} // namespace filemanager

#endif // SPREADSHEET_H