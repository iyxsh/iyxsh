#include "spreadsheet.h"
#include "LibreOfficeService.h"
#include "ErrorHandler.h"
#include "filequeue.h"
#include "template_index_cache.h"
#include "lofficeconn.h"
#include "unicodetable.h"
#include "utils.h"
#include "../cJSON/cJSON.h"
#include "../logger/logger.h"
#include "../config/json_config.h"
#include <map>
#include <utility> // std::pair

namespace filemanager {

// 加载电子表格文档
com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument>
loadSpreadsheetDocument(const rtl::OUString &filePath, 
                       com::sun::star::uno::Reference<com::sun::star::lang::XComponent> &xComp) 
{
    try {
        auto xDoc = LibreOfficeService::loadSpreadsheetDocument(filePath);
        if (xDoc.is()) {
            xComp = uno::Reference<lang::XComponent>(xDoc, uno::UNO_QUERY);
        }
        return xDoc;
    } 
    catch (const uno::Exception &e) {
        ErrorHandler::handleUnoException(e, "loadSpreadsheetDocument");
        if (xComp.is()) {
            LibreOfficeService::closeDocument(xComp);
        }
        return nullptr;
    }
    catch (const std::exception &e) {
        ErrorHandler::handleStdException(e, "loadSpreadsheetDocument");
        if (xComp.is()) {
            LibreOfficeService::closeDocument(xComp);
        }
        return nullptr;
    }
    catch (...) {
        ErrorHandler::handleUnknownException("loadSpreadsheetDocument");
        if (xComp.is()) {
            LibreOfficeService::closeDocument(xComp);
        }
        return nullptr;
    }
}

    // 列索引与Excel列名转换
    std::string columnIndexToName(int columnIndex)
    {
        std::string result;
        columnIndex++;
        while (columnIndex > 0)
        {
            int remainder = (columnIndex - 1) % 26;
            result = static_cast<char>('A' + remainder) + result;
            columnIndex = (columnIndex - 1) / 26;
        }
        return result;
    }

    /// @brief 读取单元格内容为JSON
    cJSON *readCellToJson(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &sheet,
                          const rtl::OUString &cellAddress)
    {
        // 实现读取单元格内容的函数
        return nullptr;
    }

    /// @brief 直接保存当前文档（不指定路径，保存到原始位置）
    void saveDocumentDirect(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &docIface)
    {
        try {
            LibreOfficeService::saveDocument(docIface);
        }
        catch (const uno::Exception &e) {
            ErrorHandler::handleUnoException(e, "saveDocumentDirect");
        }
        catch (const std::exception &e) {
            ErrorHandler::handleStdException(e, "saveDocumentDirect");
        }
        catch (...) {
            ErrorHandler::handleUnknownException("saveDocumentDirect");
        }
    }

    /// @brief 保存文档到指定路径
    void saveDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &docIface,
                     const rtl::OUString &filePath)
    {
        try {
            LibreOfficeService::saveDocument(docIface, filePath);
        }
        catch (const uno::Exception &e) {
            ErrorHandler::handleUnoException(e, "saveDocument");
        }
        catch (const std::exception &e) {
            ErrorHandler::handleStdException(e, "saveDocument");
        }
        catch (...) {
            ErrorHandler::handleUnknownException("saveDocument");
        }
    }

    /// @brief 关闭文档
    void closeDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &docIface)
    {
        try {
            LibreOfficeService::closeDocument(docIface);
        }
        catch (const uno::Exception &e) {
            ErrorHandler::handleUnoException(e, "closeDocument");
        }
        catch (const std::exception &e) {
            ErrorHandler::handleStdException(e, "closeDocument");
        }
        catch (...) {
            ErrorHandler::handleUnknownException("closeDocument");
        }
    }

    /// @brief 创建新电子表格文件
    cJSON *createNewSpreadsheetFile(const rtl::OUString &filePath,
                                    const rtl::OUString &sheetName)
    {
        uno::Reference<lang::XComponent> xComp;
        try
        {
            // 获取ComponentLoader
            uno::Reference<frame::XComponentLoader> xLoader = LibreOfficeConnectionManager::getComponentLoader();
            if (!xLoader.is())
            {
                std::cerr << "createNewSpreadsheetFile: Failed to get component loader" << std::endl;
                return nullptr;
            }

            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<uno::XInterface> loadedIface = xLoader->loadComponentFromURL(
                convertStringToOUString("private:factory/scalc"), convertStringToOUString("_blank"), 0, args);
            if (!loadedIface.is())
            {
                std::cerr << "createNewSpreadsheetFile: loadComponentFromURL returned null!" << std::endl;
                return nullptr;
            }

            xComp = uno::Reference<lang::XComponent>(loadedIface, uno::UNO_QUERY);
            if (!xComp.is())
            {
                std::cerr << "createNewSpreadsheetFile: loaded interface is not XComponent!" << std::endl;
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is())
            {
                std::cerr << "createNewSpreadsheetFile: loaded component is not a spreadsheet document!" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheets> sheets = xDoc->getSheets();
            if (!sheets.is())
            {
                std::cerr << "createNewSpreadsheetFile: Cannot get sheets" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);
            if (!nameAccess.is())
            {
                std::cerr << "createNewSpreadsheetFile: Cannot get name access" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheet> sheet;
            if (nameAccess->hasByName(sheetName))
            {
                // 已存在则直接获取
                uno::Any any = nameAccess->getByName(sheetName);
                any >>= sheet;
            }
            else
            {
                // 不存在则插入
                sheets->insertNewByName(sheetName, 0);
                uno::Any any = nameAccess->getByName(sheetName);
                any >>= sheet;
            }

            if (!sheet.is())
            {
                std::cerr << "createNewSpreadsheetFile: Cannot get sheet" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            // 新建文档空的，第一次保存并关闭文档
            saveDocument(xDoc, filePath);
            // closeDocument(xComp);
            return cJSON_CreateString("success");
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "createNewSpreadsheetFile UNO error: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            if (xComp.is())
            {
                try
                {
                    closeDocument(xComp); // 确保在异常情况下关闭文档
                }
                catch (...)
                {
                    std::cerr << "createNewSpreadsheetFile: Exception while closing document" << std::endl;
                }
            }
            return nullptr;
        }
        catch (const std::exception &e)
        {
            std::cerr << "createNewSpreadsheetFile std exception: " << e.what() << std::endl;
            if (xComp.is())
            {
                try
                {
                    closeDocument(xComp); // 确保在异常情况下关闭文档
                }
                catch (...)
                {
                    std::cerr << "createNewSpreadsheetFile: Exception while closing document" << std::endl;
                }
            }
            return nullptr;
        }
        catch (...)
        {
            std::cerr << "createNewSpreadsheetFile unknown exception occurred" << std::endl;
            if (xComp.is())
            {
                try
                {
                    closeDocument(xComp); // 确保在异常情况下关闭文档
                }
                catch (...)
                {
                    std::cerr << "createNewSpreadsheetFile: Exception while closing document" << std::endl;
                }
            }
            return nullptr;
        }
        return cJSON_CreateString("success");
    }

    /// @brief 更新电子表格内容
    cJSON *updateSpreadsheetContent(const rtl::OUString &filePath,
                                    const rtl::OUString &sheetName,
                                    const rtl::OUString &cellAddress,
                                    const rtl::OUString &newValue,
                                    const rtl::OUString &cellType)
    {
        uno::Reference<lang::XComponent> xComp;
        try
        {
            // 获取ComponentLoader
            uno::Reference<frame::XComponentLoader> xLoader = LibreOfficeConnectionManager::getComponentLoader();
            if (!xLoader.is())
            {
                std::cerr << "updateSpreadsheetContent: Failed to get component loader" << std::endl;
                return cJSON_CreateString("Failed to get component loader");
            }

            logger_log_info("filePath: %s", rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
            logger_log_info("newValue: %s", rtl::OUStringToOString(newValue, RTL_TEXTENCODING_UTF8).getStr());
            rtl::OUString curFilePath;
            getAbsolutePath(filePath, curFilePath);

            // 构造文件URL
            rtl::OUString fileUrl = convertStringToOUString("file://") + curFilePath.replaceAll("\\", "/");
            logger_log_info("curFilePath: %s", rtl::OUStringToOString(curFilePath, RTL_TEXTENCODING_UTF8).getStr());
            logger_log_info("SheetName: %s", rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr());
            // 加载文档
            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<lang::XComponent> xComponent = xLoader->loadComponentFromURL(fileUrl, convertStringToOUString("_blank"), 0, args);
            xComp = xComponent;
            if (!xComp.is())
            {
                std::cerr << "updateSpreadsheetContent: Failed to load document" << std::endl;
                return cJSON_CreateString("Failed to load document");
            }

            // 获取文档和工作表
            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is())
            {
                std::cerr << "updateSpreadsheetContent: Loaded component is not a spreadsheet document" << std::endl;
                closeDocument(xComp);
                return cJSON_CreateString("Loaded component is not a spreadsheet document");
            }

            uno::Reference<sheet::XSpreadsheet> sheet = getSheet(xDoc, sheetName);
            if (!sheet.is())
            {
                std::cerr << "updateSpreadsheetContent: Cannot get sheet" << std::endl;
                closeDocument(xComp);
                return cJSON_CreateString("Cannot get sheet");
            }

            // 解析单元格地址
            sal_Int32 col = 0, row = 0;
            parseCellAddress(cellAddress, col, row);

            // 获取单元格
            uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
            if (!cell.is())
            {
                std::cerr << "updateSpreadsheetContent: Cannot get cell at " << col << "," << row << std::endl;
                closeDocument(xComp);
                return cJSON_CreateString("Cannot get cell");
            }
            // 直接默认设置为公式
            if (sheetName == "wordsSheet")
            {
                // 直接默认覆盖值
                cell->setFormula(newValue);
            }
            else
            {
                std::string tmp = findStringInSpreadsheet(newValue, sheet);
                logger_log_info("updateSpreadsheetContent: %s", tmp.c_str());
                cell->setFormula(convertStringToOUString(tmp.c_str()));
            }

            // 保存文档
            saveDocumentDirect(xDoc);
            // closeDocument(xComp);

            return cJSON_CreateString("success");
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "updateSpreadsheetContent UNO exception: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            if (xComp.is())
            {
                closeDocument(xComp);
            }
            return cJSON_CreateString(rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            std::cerr << "updateSpreadsheetContent std exception: " << e.what() << std::endl;
            if (xComp.is())
            {
                closeDocument(xComp);
            }
            return cJSON_CreateString(e.what());
        }
        catch (...)
        {
            std::cerr << "updateSpreadsheetContent unknown exception occurred" << std::endl;
            if (xComp.is())
            {
                closeDocument(xComp);
            }
            return cJSON_CreateString("Unknown error occurred");
        }
        return cJSON_CreateString("success");
    }

    /// @brief 获取工作表
    com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet> getSheet(
        const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument> &document,
        const rtl::OUString &sheetName)
    {
        if (!document.is())
            throw std::runtime_error("Invalid document reference");
        uno::Reference<sheet::XSpreadsheets> sheets = document->getSheets();
        uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);
        if (!nameAccess->hasByName(sheetName))
            throw std::runtime_error("Sheet not found");
        uno::Any any = nameAccess->getByName(sheetName);
        uno::Reference<sheet::XSpreadsheet> sheet;
        any >>= sheet;
        return sheet;
    }

    // 批量更新函数
    cJSON *batchUpdateSpreadsheetContent(const rtl::OUString &filePathStr,
                                         const rtl::OUString &sheetName,
                                         const cJSON *updatecells)
    {
        uno::Reference<lang::XComponent> xComp;
        try
        {
            // 获取绝对路径
            rtl::OUString filePath;
            getAbsolutePath(filePathStr, filePath);
            // 首先判断是否文件已经加载
            FileInfo fileInfo = filemanager::FileQueueManager::getInstance().getFileInfo(convertOUStringToString(filePathStr));
            uno::Reference<sheet::XSpreadsheetDocument> xDoc;
            if (!fileInfo.xComponent.is())
            {
                xDoc = loadSpreadsheetDocument(filePath, fileInfo.xComponent);
                if (!fileInfo.xComponent.is())
                {
                    logger_log_error("Failed to load document: %s", convertOUStringToString(filePathStr).c_str());
                    return cJSON_CreateString("Failed to load document");
                }
            }
            else
            {
                xDoc = uno::Reference<sheet::XSpreadsheetDocument>(fileInfo.xComponent, uno::UNO_QUERY); // 类型转换
            }
            uno::Reference<lang::XComponent> xComp(fileInfo.xComponent); // 声明并初始化 xComp

            if (!xDoc.is())
            {
                return cJSON_CreateString("Failed to load document");
            }

            uno::Reference<sheet::XSpreadsheet> sheet = getSheet(xDoc, sheetName);
            if (!sheet.is())
            {
                std::cerr << "batchUpdateSpreadsheetContent: Cannot get sheet" << std::endl;
                closeDocument(xComp);
                return cJSON_CreateString("Cannot get sheet");
            }

            rtl::OUString defaultFilePath, defaultFileName, wordsSheetName;
            getDefaultData(defaultFilePath, defaultFileName, wordsSheetName);
            // 从缓存索引获取数据
            std::shared_ptr<CharacterIndex> idxPtr = filemanager::TemplateIndexCacheManager::getInstance().getCharacterInfos(defaultFilePath + defaultFileName, wordsSheetName);
            if (!idxPtr)
            {
                logger_log_error("Failed to get character infos from cache: idxPtr is null");
                return cJSON_CreateString("Failed to get character infos from cache");
            }
            // 处理批量更新数据
            // 支持两种格式：
            // 1. 原来的数组格式
            // 2. 新的对象格式 {"value": "cellAddress", ...}
            if (updatecells && cJSON_IsArray(const_cast<cJSON *>(updatecells)))
            {
                // 原来的数组格式处理
                cJSON *item = nullptr;
                cJSON_ArrayForEach(item, const_cast<cJSON *>(updatecells))
                {
                    if (cJSON_IsObject(item))
                    {
                        cJSON *celladdrItem = cJSON_GetObjectItem(item, "celladdr");
                        cJSON *valueItem = cJSON_GetObjectItem(item, "value");
                        cJSON *typeItem = cJSON_GetObjectItem(item, "type");

                        if (celladdrItem && valueItem && cJSON_IsString(celladdrItem) && cJSON_IsString(valueItem))
                        {
                            rtl::OUString cellAddr = convertStringToOUString(celladdrItem->valuestring);
                            rtl::OUString newValue = convertStringToOUString(valueItem->valuestring);
                            rtl::OUString celltype = typeItem && cJSON_IsString(typeItem) ? convertStringToOUString(typeItem->valuestring) : convertStringToOUString("string");

                            // 解析单元格地址
                            sal_Int32 col = 0, row = 0;
                            parseCellAddress(cellAddr, col, row);

                            // 获取单元格
                            uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                            if (!cell.is())
                            {
                                std::cerr << "updateSpreadsheetContent: Cannot get cell at " << col << "," << row << std::endl;
                                closeDocument(xComp);
                                return cJSON_CreateString("Cannot get cell");
                            }
                            if (sheetName == wordsSheetName)
                            {
                                // 直接设置字符串值
                                cell->setFormula(newValue);
                            }
                            else
                            {
                                std::vector<TextCharInfo> infos = splitAndClassifyTextFromIndex(convertOUStringToString(newValue), idxPtr);
                                rtl::OUString tmpStr = infosToFormatString(infos);
                                logger_log_info("updateSpreadsheetContent: %s", convertOUStringToString(tmpStr).c_str());
                                cell->setFormula(tmpStr);
                            }
                        }
                    }
                }
            }
            else if (updatecells && cJSON_IsObject(const_cast<cJSON *>(updatecells)))
            {
                // 新的对象格式处理：{"cellAddress": "value", ...}
                rtl::OUString defaultFilePath, defaultFileName, wordsSheetName;
                getDefaultData(defaultFilePath, defaultFileName, wordsSheetName);

                // 从配置文件读取最大列数和行数限制
                int maxCols = json_config_get_int("maxCols");
                int maxRows = json_config_get_int("maxRows");
                logger_log_info("maxRows: %d", maxRows);
                logger_log_info("maxCols: %d", maxCols);

                // 遍历对象中的每个键值对
                cJSON *item = updatecells->child;
                while (item)
                {
                    if (item->type == cJSON_String && item->valuestring)
                    {
                        // 获取单元格地址（值）和要写入的内容（键）
                        const char *cellAddress = item->valuestring;
                        if (cellAddress && item->string)
                        {
                            rtl::OUString cellAddr = rtl::OStringToOUString(cellAddress, RTL_TEXTENCODING_UTF8);
                            logger_log_info("Processing cell address: %s", cellAddress);
                            rtl::OUString newValue = convertStringToOUString(item->string);

                            // 解析单元格地址
                            sal_Int32 col = 0, row = 0;
                            parseCellAddress(cellAddr, col, row);
                            if (row >= maxRows || col >= maxCols)
                            {
                                logger_log_error("Cell address %s is out of bounds (maxRows: %d, maxCols: %d)", cellAddress, maxRows, maxCols);
                                item = item->next;
                                continue; // 跳过这个单元格，继续处理下一个
                            }
                            // 获取单元格
                            uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                            if (!cell.is())
                            {
                                std::cerr << "batchUpdateSpreadsheetContent: Cannot get cell at " << col << "," << row << std::endl;
                                closeDocument(xComp);
                                return cJSON_CreateString("Cannot get cell");
                            }

                            // 写入单元格内容
                            if (sheetName == wordsSheetName)
                            {
                                // 直接设置字符串值
                                cell->setFormula(newValue);
                            }
                            else
                            {
                                std::vector<TextCharInfo> infos = splitAndClassifyTextFromIndex(convertOUStringToString(newValue), idxPtr);
                                rtl::OUString tmpStr = infosToFormatString(infos);
                                logger_log_info("updateSpreadsheetContent: %s", convertOUStringToString(tmpStr).c_str());
                                cell->setFormula(tmpStr);
                            }
                        }
                        else
                        {
                            logger_log_error("Invalid cell address or missing string key in updatecells");
                        }
                    }
                    // 移动到下一个元素
                    item = item->next;
                }
            }
            else
            {
                logger_log_error("Error: Invalid data in batch update");
                return cJSON_CreateString("Error: Invalid data in batch update");
            }

            // 保存文档
            saveDocumentDirect(xDoc);
            // closeDocument(xComp);
            cJSON *jobj = cJSON_CreateObject();
            cJSON_AddStringToObject(jobj, "result", "success");
            return jobj;
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "batchUpdateSpreadsheetContent UNO exception: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            if (xComp.is())
            {
                closeDocument(xComp);
            }
            return cJSON_CreateString(rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            std::cerr << "batchUpdateSpreadsheetContent std exception: " << e.what() << std::endl;
            if (xComp.is())
            {
                closeDocument(xComp);
            }
            return cJSON_CreateString(e.what());
        }
        catch (...)
        {
            std::cerr << "batchUpdateSpreadsheetContent unknown exception occurred" << std::endl;
            if (xComp.is())
            {
                closeDocument(xComp);
            }
            return cJSON_CreateString("Unknown error occurred");
        }
    }

    // 内部辅助函数
    cJSON *findValueInSheet(const rtl::OUString &filePath, const rtl::OUString &sheetName, const rtl::OUString &searchValue)
    {
        // 实现查找值的函数
        return nullptr;
    }

    cJSON *readSheetData(const rtl::OUString &filePath, const rtl::OUString &sheetName)
    {
        // 减少日志输出以提高性能
        // logger_log_info("filePath: %s", rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
        // logger_log_info("sheetName: %s", rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr());
        uno::Reference<lang::XComponent> xComp; // 在函数作用域内声明，确保在任何情况下都能正确关闭
        try
        {
            // 获取ComponentLoader
            uno::Reference<frame::XComponentLoader> xLoader = LibreOfficeConnectionManager::getComponentLoader();
            if (!xLoader.is())
            {
                std::cerr << "readSheetData: Failed to get component loader" << std::endl;
                return nullptr;
            }

            rtl::OUString url = convertStringToOUString("file://") + filePath.replaceAll("\\", "/");
            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<lang::XComponent> xComponent = xLoader->loadComponentFromURL(url, convertStringToOUString("_blank"), 0, args);
            xComp = xComponent;
            if (!xComp.is())
            {
                std::cerr << "readSheetData: Cannot load document!" << std::endl;
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is())
            {
                std::cerr << "readSheetData: Cannot open spreadsheet!" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheet> sheet = getSheet(xDoc, sheetName);
            if (!sheet.is())
            {
                std::cerr << "readSheetData: Cannot get sheet!" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            // 创建返回的 cJSON 对象，用于存储单元格内容和位置的映射
            cJSON *contentMap = cJSON_CreateObject();

            // 创建一个用于跟踪已添加内容的辅助对象，确保内容不重复
            cJSON *addedContents = cJSON_CreateObject();

            // 按列优先读取数据，每列读取到第一个空单元格为止
            sal_Int32 col = 0;
            // 从配置文件读取最大列数和行数限制
            int maxCols = json_config_get_int("maxCols");
            int maxRows = json_config_get_int("maxRows");
            logger_log_info("maxRows: %d", maxRows);
            logger_log_info("maxCols: %d", maxCols);
            // 循环处理每一列
            while (col < maxCols)
            {
                bool foundDataInColumn = false;

                // 处理当前列的每一行
                for (sal_Int32 row = 0; row < maxRows; ++row)
                {
                    uno::Reference<table::XCell> cell;
                    try
                    {
                        cell = sheet->getCellByPosition(col, row);
                    }
                    catch (const uno::Exception &)
                    {
                        logger_log_error("Exception getting cell at col %d, row %d", col, row);
                        break;
                    }
                    if (!cell.is())
                    {
                        logger_log_error("Cell at col %d, row %d is null", col, row);
                        break;
                    }
                    double cellValue = cell->getValue();
                    rtl::OUString cellFormula = cell->getFormula();
                    // 判断是否为空单元格
                    if (cellValue == 0.0 && cellFormula.getLength() == 0)
                    {
                        continue; // 跳过空单元格
                    }
                    // 优先返回单元格显示内容（XText->getString），如果为空再返回数值或公式
                    rtl::OUString cellString;
                    try
                    {
                        uno::Reference<com::sun::star::text::XText> xText(cell, uno::UNO_QUERY);
                        if (xText.is())
                        {
                            cellString = xText->getString();
                        }
                    }
                    catch (...)
                    {
                        cellString = rtl::OUString();
                    }
                    std::string content;
                    if (cellString.getLength() > 0)
                    {
                        content = rtl::OUStringToOString(cellString, RTL_TEXTENCODING_UTF8).getStr();
                    }
                    else if (cellValue != 0.0)
                    {
                        content = std::to_string(cellValue);
                    }
                    else if (cellFormula.getLength() > 0)
                    {
                        content = rtl::OUStringToOString(cellFormula, RTL_TEXTENCODING_UTF8).getStr();
                    }
                    // 检查内容是否已经添加过（避免重复）
                    if (cJSON_GetObjectItem(addedContents, content.c_str()) != nullptr)
                    {
                        continue; // 内容已存在，跳过
                    }
                    // 计算列名 (A, B, ..., Z, AA, AB, ...)
                    std::string columnName;
                    int column = col;
                    do
                    {
                        columnName = static_cast<char>('A' + (column % 26)) + columnName;
                        column = (column / 26) - 1;
                    } while (column >= 0);
                    // 计算行号 (1-based)
                    int rowNumber = row + 1;
                    // 创建位置字符串，如 "A1", "B2" 等
                    std::string position = columnName + std::to_string(rowNumber);
                    // 添加内容到结果对象（内容作为key，位置作为value）
                    cJSON_AddStringToObject(contentMap, content.c_str(), position.c_str());
                    // 标记内容已添加
                    cJSON_AddStringToObject(addedContents, content.c_str(), "added");
                }

                // 如果此列没有数据，则认为是空列，停止处理后续列
                if (!foundDataInColumn)
                {
                    break;
                }

                ++col;
            }

            // 清理辅助对象
            cJSON_Delete(addedContents);

            closeDocument(xComp);
            return contentMap;
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "readSheetData UNO error: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            return nullptr;
        }
        catch (const std::exception &e)
        {
            std::cerr << "readSheetData std exception: " << e.what() << std::endl;
            return nullptr;
        }
        catch (...)
        {
            std::cerr << "readSheetData unknown exception occurred" << std::endl;
            return nullptr;
        }
        return cJSON_CreateString("success");
    }

    // 获取工作表总行数（持续读取行直到遇到第一个空白行）
    int getTotalRecordCount(const rtl::OUString &filePathStr, const rtl::OUString &sheetName)
    {
        try
        {
            // 获取绝对路径
            rtl::OUString filePath;
            getAbsolutePath(filePathStr, filePath);
            // 首先判断是否文件已经加载
            FileInfo fileInfo = filemanager::FileQueueManager::getInstance().getFileInfo(convertOUStringToString(filePathStr));
            uno::Reference<sheet::XSpreadsheetDocument> xDoc;
            if (!fileInfo.xComponent.is())
            {
                xDoc = loadSpreadsheetDocument(filePath, fileInfo.xComponent);
                if (!fileInfo.xComponent.is())
                {
                    logger_log_error("Failed to load document: %s", convertOUStringToString(filePathStr).c_str());
                    return -1;
                }
            }
            else
            {
                xDoc = uno::Reference<sheet::XSpreadsheetDocument>(fileInfo.xComponent, uno::UNO_QUERY); // 类型转换
            }
            uno::Reference<lang::XComponent> xComp(fileInfo.xComponent); // 声明并初始化 xComp

            if (!xDoc.is())
            {
                return -1;
            }

            uno::Reference<sheet::XSpreadsheets> sheets = xDoc->getSheets();
            uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);
            if (!nameAccess->hasByName(sheetName))
            {
                closeDocument(xComp);
                return -1;
            }
            uno::Reference<sheet::XSpreadsheet> sheet(nameAccess->getByName(sheetName), uno::UNO_QUERY);

            int maxCols = json_config_get_int("maxCols");
            int maxRows = json_config_get_int("maxRows");
            logger_log_info("maxRows: %d", maxRows);
            logger_log_info("maxCols: %d", maxCols);
            int validRowCount = 0;
            int col = 1; // 列索引
            int row = 0; // 行索引
            while (true)
            {
                if (++row > maxRows)
                {
                    logger_log_error("Reached max row limit: %d", maxRows);
                    break;
                }
                uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, validRowCount);
                double cellValue = cell->getValue();
                rtl::OUString cellFormula = cell->getFormula();
                if (cellValue != 0.0 || cellFormula.getLength() > 0)
                {
                    ++validRowCount; // 统计有效行数
                }
                else
                {
                    break;
                }
            }

            closeDocument(xComp);
            return validRowCount;
        }
        catch (...)
        {
            return -1;
        }
    }

    // 分页读取工作表内容（简单实现：返回 startIndex 到 endIndex 行的内容）
    cJSON *readSheetDataRange(const rtl::OUString &filePathStr, const rtl::OUString &sheetName, int startIndex, int endIndex)
    {
        cJSON *result = cJSON_CreateArray();
        try
        {
            // 获取绝对路径
            rtl::OUString filePath;
            getAbsolutePath(filePathStr, filePath);
            // 首先判断是否文件已经加载
            FileInfo fileInfo = filemanager::FileQueueManager::getInstance().getFileInfo(convertOUStringToString(filePathStr));
            uno::Reference<sheet::XSpreadsheetDocument> xDoc;
            if (!fileInfo.xComponent.is())
            {
                xDoc = loadSpreadsheetDocument(filePath, fileInfo.xComponent);
                if (!fileInfo.xComponent.is())
                {
                    logger_log_error("Failed to load document: %s", convertOUStringToString(filePathStr).c_str());
                    return cJSON_CreateString("failed");
                }
            }
            else
            {
                xDoc = uno::Reference<sheet::XSpreadsheetDocument>(fileInfo.xComponent, uno::UNO_QUERY); // 类型转换
            }
            uno::Reference<lang::XComponent> xComp(fileInfo.xComponent); // 声明并初始化 xComp

            if (!xDoc.is())
            {
                return cJSON_CreateString("failed");
            }
            uno::Reference<sheet::XSpreadsheets> sheets = xDoc->getSheets();
            uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);
            if (!nameAccess->hasByName(sheetName))
            {
                closeDocument(xComp);
                return result;
            }
            uno::Reference<sheet::XSpreadsheet> sheet(nameAccess->getByName(sheetName), uno::UNO_QUERY);
            for (int row = startIndex; row < endIndex; ++row)
            {
                cJSON *rowObj = cJSON_CreateObject();
                // 假设最多100列
                for (int col = 0; col < 100; ++col)
                {
                    uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                    double cellValue = cell->getValue();
                    rtl::OUString cellFormula = cell->getFormula();
                    // 判断是否为空单元格
                    if (cellValue == 0.0 && cellFormula.getLength() == 0)
                    {
                        continue; // 跳过空单元格
                    }
                    // 生成单元格位置，如A1、B2
                    std::string colName = filemanager::columnIndexToName(col);
                    std::string cellPos = colName + std::to_string(row + 1);
                    std::string content;
                    // 优先返回单元格显示内容（XText->getString），如果为空再返回数值或公式
                    rtl::OUString cellString;
                    try
                    {
                        uno::Reference<com::sun::star::text::XText> xText(cell, uno::UNO_QUERY);
                        if (xText.is())
                        {
                            cellString = xText->getString();
                        }
                    }
                    catch (...)
                    {
                        cellString = rtl::OUString();
                    }
                    if (cellString.getLength() > 0)
                    {
                        content = rtl::OUStringToOString(cellString, RTL_TEXTENCODING_UTF8).getStr();
                    }
                    else if (cellValue != 0.0)
                    {
                        content = std::to_string(cellValue);
                    }
                    else if (cellFormula.getLength() > 0)
                    {
                        content = rtl::OUStringToOString(cellFormula, RTL_TEXTENCODING_UTF8).getStr();
                    }
                    cJSON_AddStringToObject(rowObj, content.c_str(), cellPos.c_str());
                }
                cJSON_AddItemToArray(result, rowObj);
            }
            // closeDocument(xComp);
        }
        catch (...)
        {
            // 忽略异常，返回已收集内容
        }
        return result;
    }

    // 清空工作表内容
    void clearSheet(const uno::Reference<sheet::XSpreadsheet> &sheet)
    {
        if (!sheet.is())
            return;
        int maxCols = json_config_get_int("maxCols");
        int maxRows = json_config_get_int("maxRows");
        for (int row = 0; row < maxRows; ++row)
        {
            for (int col = 0; col < maxCols; ++col)
            {
                uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                if (cell.is())
                {
                    cell->setFormula(rtl::OUString()); // 清空公式和内容
                }
            }
        }
    }

    // 实现用于查找字符串位置的函数
    std::string findStringInSpreadsheet(const rtl::OUString &targetString, css::uno::Reference<css::sheet::XSpreadsheet> sheet)
    {
        try
        {
            int maxRows = json_config_get_int("maxRows");
            int maxCols = json_config_get_int("maxCols");
            // 遍历单元格以查找目标字符串
            for (int row = 0; row < maxRows; ++row)
            {
                for (int col = 0; col < maxCols; ++col)
                {
                    css::uno::Reference<css::table::XCell> cell = sheet->getCellByPosition(col, row);
                    if (cell.is())
                    {
                        rtl::OUString cellValue = cell->getFormula();
                        if (cellValue == targetString)
                        {
                            // 转换为A1形式
                            std::string colName = filemanager::columnIndexToName(col);
                            std::string cellPos = colName + std::to_string(row + 1);
                            return cellPos;
                        }
                    }
                }
            }
            // 未找到返回空字符串
            return std::string("");
        }
        catch (const css::uno::Exception &e)
        {
            std::string errorMessage = rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr();
            logger_log_error("Error in findStringInSpreadsheet: %s", errorMessage.c_str());
            return std::string("");
        }
        catch (...)
        {
            logger_log_error("Unknown exception occurred in findStringInSpreadsheet");
            return std::string("");
        }
    }

    // 主处理函数：拆分文本，分类，查找位置，返回每个字符的详细信息
    std::vector<TextCharInfo> splitAndClassifyText(const std::string &text, css::uno::Reference<css::sheet::XSpreadsheet> sheet)
    {
        std::vector<TextCharInfo> result;
        std::vector<char32_t> chars = filemanager::splitToUnicode(text);
        for (char32_t ch : chars)
        {
            std::string utf8char;
            if (ch <= 0x7F)
            {
                utf8char += static_cast<char>(ch);
            }
            else if (ch <= 0x7FF)
            {
                utf8char += static_cast<char>(0xC0 | ((ch >> 6) & 0x1F));
                utf8char += static_cast<char>(0x80 | (ch & 0x3F));
            }
            else if (ch <= 0xFFFF)
            {
                utf8char += static_cast<char>(0xE0 | ((ch >> 12) & 0x0F));
                utf8char += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                utf8char += static_cast<char>(0x80 | (ch & 0x3F));
            }
            else
            {
                utf8char += static_cast<char>(0xF0 | ((ch >> 18) & 0x07));
                utf8char += static_cast<char>(0x80 | ((ch >> 12) & 0x3F));
                utf8char += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                utf8char += static_cast<char>(0x80 | (ch & 0x3F));
            }
            rtl::OUString unoChar = rtl::OStringToOUString(utf8char.c_str(), RTL_TEXTENCODING_UTF8);
            std::string pos = findStringInSpreadsheet(unoChar, sheet);
            std::string langType = getLanguageType(ch);
            // splitAndClassifyText 没有 bodyname 来源，暂设为空
            result.push_back(TextCharInfo{utf8char, pos, langType, ""});
        }
        return result;
    }

    // 从工作表读取所有单元格内容，按语言分组返回
    std::vector<LanguageGroup> readSheetAndGroupByLanguage(css::uno::Reference<css::sheet::XSpreadsheet> sheet)
    {
        // 优化为三层分组：LanguageGroup → CharacterBody → CharacterInfo
        // 循环顺序为先列后行，bodyname 取列号（A、B、C...）任意拍
        std::unordered_map<std::string, std::unordered_map<std::string, std::vector<CharacterInfo>>> tempMap;
        int maxCols = json_config_get_int("maxCols");
        int maxRows = json_config_get_int("maxRows");
        for (int col = 0; col < maxCols; ++col)
        {
            bool colHasData = false;
            std::string bodyname = columnIndexToName(col); // 列名作为 bodyname
            for (int row = 0; row < maxRows; ++row)
            {
                css::uno::Reference<css::table::XCell> cell = sheet->getCellByPosition(col, row);
                if (!cell.is())
                    continue;
                double cellValue = cell->getValue();
                rtl::OUString cellFormula = cell->getFormula();
                if (cellValue == 0.0 && cellFormula.getLength() == 0)
                {
                    if (colHasData)
                        break;
                    else
                        continue;
                }
                colHasData = true;
                rtl::OUString cellString;
                try
                {
                    css::uno::Reference<com::sun::star::text::XText> xText(cell, css::uno::UNO_QUERY);
                    if (xText.is())
                        cellString = xText->getString();
                }
                catch (...)
                {
                    cellString = rtl::OUString();
                }
                std::string content;
                if (cellString.getLength() > 0)
                {
                    content = rtl::OUStringToOString(cellString, RTL_TEXTENCODING_UTF8).getStr();
                }
                else
                {
                    if (cellValue != 0.0)
                        content = std::to_string(cellValue);
                    else if (cellFormula.getLength() > 0)
                        content = rtl::OUStringToOString(cellFormula, RTL_TEXTENCODING_UTF8).getStr();
                }
                if (content.empty())
                    continue;
                std::vector<char32_t> chars = filemanager::splitToUnicode(content);
                for (char32_t ch : chars)
                {
                    std::string utf8char;
                    if (ch <= 0x7F)
                        utf8char += static_cast<char>(ch);
                    else if (ch <= 0x7FF)
                    {
                        utf8char += static_cast<char>(0xC0 | ((ch >> 6) & 0x1F));
                        utf8char += static_cast<char>(0x80 | (ch & 0x3F));
                    }
                    else if (ch <= 0xFFFF)
                    {
                        utf8char += static_cast<char>(0xE0 | ((ch >> 12) & 0x0F));
                        utf8char += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                        utf8char += static_cast<char>(0x80 | (ch & 0x3F));
                    }
                    else
                    {
                        utf8char += static_cast<char>(0xF0 | ((ch >> 18) & 0x07));
                        utf8char += static_cast<char>(0x80 | ((ch >> 12) & 0x3F));
                        utf8char += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                        utf8char += static_cast<char>(0x80 | (ch & 0x3F));
                    }
                    std::string langType = getLanguageType(ch);
                    int langTypeCount = tempMap.size(); // tempMap 的个数
                    if (tempMap[langType].empty())
                    {
                        bodyname = numberToExcelColumn(langTypeCount + 1); // 重新设置 bodyname
                    }
                    else
                    {
                        bodyname = tempMap[langType].begin()->first; // 保留原来的 bodyname
                    }
                    int characterCount = tempMap[langType][bodyname].size();
                    std::string cellPos = bodyname + std::to_string(characterCount + 1);
                    logger_log_info("readSheetAndGroupByLanguage: Reading info: %s %s %s %s", utf8char.c_str(), cellPos.c_str(), langType.c_str(), bodyname.c_str());
                    tempMap[langType][bodyname].push_back(CharacterInfo{utf8char, cellPos});
                }
            }
            if (!colHasData)
                break;
        }
        std::vector<LanguageGroup> result;
        for (const auto &langPair : tempMap)
        {
            LanguageGroup group;
            group.languageType = langPair.first;
            for (const auto &bodyPair : langPair.second)
            {
                CharacterBody body;
                body.bodyname = bodyPair.first;
                body.characters = bodyPair.second;
                group.bodies.push_back(body);
            }
            result.push_back(group);
        }
        return result;
    }

    // 批量写入 CharacterInfo 到指定文件和工作表
    bool writeCharacterInfosToSheet(const rtl::OUString &filePathStr,
                                    const rtl::OUString &sheetName,
                                    const std::vector<TextCharInfo> &infos)
    {
        // 获取绝对路径
        rtl::OUString filePath;
        getAbsolutePath(filePathStr, filePath);
        // 首先判断是否文件已经加载
        FileInfo fileInfo = filemanager::FileQueueManager::getInstance().getFileInfo(convertOUStringToString(filePathStr));
        uno::Reference<sheet::XSpreadsheetDocument> xDoc;
        if (!fileInfo.xComponent.is())
        {
            xDoc = loadSpreadsheetDocument(filePath, fileInfo.xComponent);
            if (!fileInfo.xComponent.is())
            {
                logger_log_error("Failed to load document: %s", convertOUStringToString(filePathStr).c_str());
                return false;
            }
        }
        else
        {
            xDoc = uno::Reference<sheet::XSpreadsheetDocument>(fileInfo.xComponent, uno::UNO_QUERY); // 类型转换
        }
        uno::Reference<lang::XComponent> xComp(fileInfo.xComponent); // 声明并初始化 xComp

        if (!xDoc.is())
        {
            return false;
        }

        uno::Reference<sheet::XSpreadsheet> sheet = getSheet(xDoc, sheetName);
        if (!sheet.is())
        {
            logger_log_error("writeCharacterInfosToSheet: Failed to get sheet");
            closeDocument(xComp);
            return false;
        }
        for (const auto &info : infos)
        {
            logger_log_info("writeCharacterInfosToSheet: Writing info: %s %s %s %s", info.character.c_str(), info.pos.c_str(), info.languageType.c_str(), info.bodyname.c_str());
            try
            {
                // pos 形如 "A1"，需解析为行列
                std::string posStr = info.pos;
                int col = 0, row = 0;
                ExcelColumnToNumber(posStr, &col, &row);
                uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                cell->setFormula(convertStringToOUString(info.character.c_str()));
            }
            catch (...)
            {
                logger_log_error("writeCharacterInfosToSheet: Failed to write %s to %s", info.character.c_str(), info.pos.c_str());
            }
        }
        saveDocumentDirect(xDoc);
        // closeDocument(xComp);
        return true;
    }

    // 使用 CharacterIndex::queryAll 查询字符所有位置（特定一般一个），返回 pos 列表，没有的新添加（后保存到模板文件）
    std::vector<TextCharInfo> splitAndClassifyTextFromIndex(const std::string &text, std::shared_ptr<CharacterIndex> index)
    {
        std::vector<TextCharInfo> result;
        bool newAdded = false;
        std::vector<char32_t> chars = filemanager::splitToUnicode(text);
        for (char32_t ch : chars)
        {
            std::string utf8char;
            if (ch <= 0x7F)
            {
                utf8char += static_cast<char>(ch);
            }
            else if (ch <= 0x7FF)
            {
                utf8char += static_cast<char>(0xC0 | ((ch >> 6) & 0x1F));
                utf8char += static_cast<char>(0x80 | (ch & 0x3F));
            }
            else if (ch <= 0xFFFF)
            {
                utf8char += static_cast<char>(0xE0 | ((ch >> 12) & 0x0F));
                utf8char += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                utf8char += static_cast<char>(0x80 | (ch & 0x3F));
            }
            else
            {
                utf8char += static_cast<char>(0xF0 | ((ch >> 18) & 0x07));
                utf8char += static_cast<char>(0x80 | ((ch >> 12) & 0x3F));
                utf8char += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                utf8char += static_cast<char>(0x80 | (ch & 0x3F));
            }
            std::string langType = getLanguageType(ch);
            std::vector<TextCharInfo> infos = index->queryAll(utf8char);
            if (!infos.empty())
            {
                for (const auto &info : infos)
                {
                    result.push_back(info);
                }
            }
            else
            {
                newAdded = true;
                // 如果没有找到，则添加到对应语言下
                int langTypeCount = index->data.size(); // 添加到对应语言下
                std::string bodyname;
                if (index->data[langType].empty())
                {
                    bodyname = numberToExcelColumn(langTypeCount + 1); // 添加到对应语言下
                }
                else
                {
                    bodyname = index->data[langType].begin()->first; // 保留原来的 bodyname
                }
                int characterCount = index->data[langType].begin()->second.size();

                std::string cellPos = bodyname + std::to_string(characterCount + 1);
                index->data[langType][bodyname][utf8char].push_back(CharacterInfo{utf8char, cellPos});
                result.push_back(TextCharInfo{utf8char, cellPos, langType, bodyname});
            }
        }
        if (newAdded)
        {
            int res = filemanager::TemplateIndexCacheManager::getInstance().setnewTodefaultfile();
            if (res)
            {
                logger_log_error("setnewTodefaultfile: Failed to");
                return result;
            }
        }
        return result;
    }
    rtl::OUString infosToFormatString(const std::vector<TextCharInfo> &infos)
    {
        rtl::OUStringBuffer resultBuffer;
        bool first = true;
        for (auto &info : infos)
        {
            // 如果找到位置信息，添加到结果中
            if (first)
            {
                // 除了第一个字符外，其他字符前面都加&
                resultBuffer.append(convertStringToOUString("="));
                first = false;
            }
            else
            {
                resultBuffer.append(convertStringToOUString("&"));
            }
            resultBuffer.append(convertStringToOUString("$"));
            // 从配置中获取默认工作表名
            const char *defaultSheetName = json_config_get_string("WordsSheet");
            if (!defaultSheetName)
            {
                defaultSheetName = "WordsSheet"; // 默认值
            }
            resultBuffer.append(convertStringToOUString(defaultSheetName));
            resultBuffer.append(convertStringToOUString("."));
            resultBuffer.append(convertStringToOUString(info.pos.c_str()));
        }
        return resultBuffer.makeStringAndClear();
    }

    bool FileExists(const rtl::OUString &filePath)
    {
        // return osl::FileBase::E_None == osl::File::isExistent(filePath);
        //osl::FileStatus status(osl_FileStatus_Mask_Type);
        //if (osl::FileBase::E_None == osl::File::getStatus(filePath, status))
        //{
        //    return status.isValid();
        //}
    }

    void DeleteFileWithUNO(const rtl::OUString &filePath)
    {
        try
        {
            // 获取组件上下文
            uno::Reference<uno::XComponentContext> xContext = LibreOfficeConnectionManager::getContext();

            // 获取服务管理器
            uno::Reference<lang::XMultiComponentFactory> xServiceManager = xContext->getServiceManager();

            // 获取 XSimpleFileAccess
            uno::Reference<ucb::XSimpleFileAccess> fileAccess(xServiceManager->createInstanceWithContext(
                                                                  "com.sun.star.ucb.SimpleFileAccess", xContext),
                                                              uno::UNO_QUERY);

            // 检查文件是否存在
            if (fileAccess->exists(filePath))
            {
                // 删除文件
                fileAccess->kill(filePath);
                logger_log_info("File deleted successfully: %s", convertOUStringToString(filePath).c_str());
            }
            else
            {
                logger_log_error("File does not exist: %s", convertOUStringToString(filePath).c_str());
            }
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO Exception occurred while deleting file: %s, Error: %s", convertOUStringToString(filePath).c_str(), rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
    }

    void SafeDeleteFile(const rtl::OUString &filePath)
    {
        try
        {
            // 检查文件存在性
            if (!FileExists(filePath))
            {
                logger_log_error("File does not exist: %s", convertOUStringToString(filePath).c_str());
                return;
            }

            // 调用 UNO API 删除文件
            DeleteFileWithUNO(filePath);
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO Exception occurred while deleting file: %s, Error: %s", convertOUStringToString(filePath).c_str(), rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            logger_log_error("Standard exception occurred while deleting file: %s, Error: %s", convertOUStringToString(filePath).c_str(), std::string(e.what()).c_str());
        }
        catch (...)
        {
            logger_log_error("Unknown exception occurred while deleting file: %s", convertOUStringToString(filePath).c_str());
        }
    }
}
