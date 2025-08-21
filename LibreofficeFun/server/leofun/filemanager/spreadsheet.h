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

using namespace com::sun::star;

namespace filemanager
{
    /// @brief 读取单元格内容为JSON
    cJSON *readCellToJson(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &sheet,
                          const rtl::OUString &cellAddress);
    /// @brief 直接保存打开的文档，不要路径
    void saveDocumentDirect(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &docIface);
    /// @brief 保存文档到指定路径
    void saveDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &doc,
                      const rtl::OUString &filePath);

    /// @brief 关闭文档
    void closeDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &docInterface);

    /// @brief 创建新电子表格文件
    cJSON *createNewSpreadsheetFile(const rtl::OUString &filePath,
                                    const rtl::OUString &sheetName);

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

    // 批量更新函数
    cJSON *batchUpdateSpreadsheetContent(const rtl::OUString &filePath,
                                         const rtl::OUString &sheetName,
                                         const cJSON *updateData);

    // 内部辅助函数
    cJSON *findValueInSheet(const rtl::OUString &filePath, const rtl::OUString &sheetName, const rtl::OUString &searchValue);
    cJSON *readSheetData(const rtl::OUString &filePath, const rtl::OUString &sheetName);

    // 公共函数用于加载文档
    com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument>
    loadSpreadsheetDocument(const rtl::OUString &filename, com::sun::star::uno::Reference<com::sun::star::lang::XComponent> &xComp);
    // 获取工作表总记录数
    int getTotalRecordCount(const rtl::OUString &filePath, const rtl::OUString &sheetName);
    // 分页读取工作表内容
    cJSON *readSheetDataRange(const rtl::OUString &filePath, const rtl::OUString &sheetName, int startIndex, int endIndex);
    void clearSheet(const uno::Reference<sheet::XSpreadsheet> &sheet);
    // 声明用于查找字符串位置的函数
    std::string findStringInSpreadsheet(const rtl::OUString &targetString, css::uno::Reference<css::sheet::XSpreadsheet> sheet);
    std::vector<LanguageGroup> readSheetAndGroupByLanguage(css::uno::Reference<css::sheet::XSpreadsheet> sheet);

    // 批量写入 CharacterInfo 到指定文件和工作表
    bool writeCharacterInfosToSheet(const rtl::OUString &filePath,
                                    const rtl::OUString &sheetName,
                                    const std::vector<TextCharInfo> &infos);
    std::vector<TextCharInfo> splitAndClassifyTextFromIndex(const std::string& text, std::shared_ptr<CharacterIndex> index);
    rtl::OUString infosToFormatString(const std::vector<TextCharInfo> &infos);
} // namespace filemanager

#endif // SPREADSHEET_H