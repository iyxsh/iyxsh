#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <memory>
#include <vector>
// cJSON
#include "../cJSON/cJSON.h"
#include "../logger/logger.h"
// UNO C++ 头文件
#include <com/sun/star/uno/Reference.hxx>
#include <rtl/ustring.hxx>

namespace com { namespace sun { namespace star {
    namespace uno { class XInterface; }
    namespace sheet { class XSpreadsheet; class XSpreadsheetDocument; }
}}}

namespace filemanager {
// 列索引与Excel列名转换
std::string columnIndexToName(int columnIndex);

/// @brief 读取单元格内容为JSON
cJSON* readCellToJson(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface>& sheet, 
                      const rtl::OUString& cellAddress);

/// @brief 保存文档到指定路径
void saveDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface>& doc, 
                  const rtl::OUString& filePath);

/// @brief 关闭文档
void closeDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface>& docInterface);

/// @brief 读取整个电子表格文件为JSON
cJSON* readSpreadsheetFile(const rtl::OUString& filePath);

/// @brief 创建新电子表格文件
cJSON* createNewSpreadsheetFile(const rtl::OUString& filePath,
                               const rtl::OUString& sheetName,
                               const cJSON* contentData = nullptr);

/// @brief 更新电子表格内容
cJSON* updateSpreadsheetContent(const rtl::OUString& filePath,
                                const rtl::OUString& sheetName,
                                const rtl::OUString& cellAddress,
                                const rtl::OUString& newValue,
                                const rtl::OUString& cellType);

/// @brief 获取工作表
com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet> getSheet(
    const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument>& document,
    const rtl::OUString& sheetName);

void newfileCreate(cJSON *results, const char *body);
void updatefile(cJSON *results, const char *body);
void editfile(cJSON *results, const char *body);
void findInSheet(cJSON *results, const char *body);
void readSheetContents(cJSON *results, const char *body);
    
// 内部辅助函数
cJSON* findValueInSheet(const rtl::OUString& filePath, const rtl::OUString& sheetName, const rtl::OUString& searchValue);
cJSON* readSheetData(const rtl::OUString& filePath, const rtl::OUString& sheetName);
rtl::OUString findCharPositions(const rtl::OUString& newValue, cJSON* sheetData);
} // namespace filemanager
#endif // FILEMANAGER_H