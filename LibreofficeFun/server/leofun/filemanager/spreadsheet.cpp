#include "spreadsheet.h"
#include "lofficeconn.h"
#include "filequeue.h"
#include "template_index_cache.h"
#include "utils.h"
#include "../cJSON/cJSON.h"
#include "../logger/logger.h"
#include "../config/json_config.h"
#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <map>
#if __cplusplus >= 201703L
#include <map>
#include <filesystem>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <utility> // std::pair

// 定义缓存大小限制
#define MAX_TEMPLATE_CACHE_SIZE 100
#define MAX_SHEET_DATA_CACHE_SIZE 100

namespace filemanager
{
    // 提取公共函数用于加载文档(传入绝对路径)
    com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument>
    loadSpreadsheetDocument(const rtl::OUString &filePath, com::sun::star::uno::Reference<com::sun::star::lang::XComponent> &xComp)
    {
        try
        {
            // 使用LibreOffice连接管理器
            if (!LibreOfficeConnectionManager::initialize())
            {
                logger_log_error("Failed to initialize LibreOffice connection");
                return nullptr;
            }

            // 获取ComponentLoader
            com::sun::star::uno::Reference<com::sun::star::frame::XComponentLoader> xLoader = LibreOfficeConnectionManager::getComponentLoader();
            if (!xLoader.is())
            {
                logger_log_error("Failed to get LibreOffice component loader");
                return nullptr;
            }

            // 加载文档
            rtl::OUString url = convertStringToOUString("file://") + filePath.replaceAll("\\", "/");
            com::sun::star::uno::Sequence<com::sun::star::beans::PropertyValue> args(0);
            com::sun::star::uno::Reference<com::sun::star::lang::XComponent> xComponent = xLoader->loadComponentFromURL(
                url, convertStringToOUString("_blank"), 0, args);
            xComp = xComponent;

            if (!xComp.is())
            {
                logger_log_error("Cannot load document: %s", filePath.getStr());
                return nullptr;
            }

            com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument> xDoc(xComp, com::sun::star::uno::UNO_QUERY);
            if (!xDoc.is())
            {
                logger_log_error("Cannot open spreadsheet: %s", filePath.getStr());
                closeDocument(xComp);
                return nullptr;
            }

            return xDoc;
        }
        catch (const com::sun::star::uno::Exception &e)
        {
            logger_log_error("UNO exception in loadSpreadsheetDocument: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            // 确保在异常情况下关闭文档
            if (xComp.is())
            {
                closeDocument(xComp);
            }
            return nullptr;
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in loadSpreadsheetDocument: %s", e.what());
            // 确保在异常情况下关闭文档
            if (xComp.is())
            {
                closeDocument(xComp);
            }
            return nullptr;
        }
        catch (...)
        {
            logger_log_error("Unknown exception in loadSpreadsheetDocument");
            // 确保在异常情况下关闭文档
            if (xComp.is())
            {
                closeDocument(xComp);
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
        try
        {
            if (!docIface.is())
            {
                logger_log_error("saveDocumentDirect error: Invalid document interface");
                return;
            }
            com::sun::star::uno::Reference<com::sun::star::frame::XStorable> xStorable(docIface, com::sun::star::uno::UNO_QUERY);
            if (!xStorable.is())
            {
                logger_log_error("saveDocumentDirect error: Invalid storable interface");
                return;
            }
            // 直接保存到原始位置（store 无参数）
            try
            {
                xStorable->store();
                logger_log_info("Document saved directly to original location.");
            }
            catch (const com::sun::star::uno::Exception &e)
            {
                rtl::OUString errorMessage = e.Message;
                if (errorMessage.getLength() > 0)
                {
                    logger_log_error("UNO Exception during direct save: %s", convertOUStringToString(errorMessage).c_str());
                }
                else
                {
                    logger_log_error("Unknown UNO Exception occurred during direct save.");
                }
            }
        }
        catch (const com::sun::star::uno::Exception &e)
        {
            logger_log_error("saveDocumentDirect UNO exception: %s", rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            logger_log_error("saveDocumentDirect std exception: %s", e.what());
        }
        catch (...)
        {
            logger_log_error("saveDocumentDirect unknown exception occurred");
        }
    }

    /// @brief 保存文档到指定路径
    void saveDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &docIface,
                      const rtl::OUString &filePath)
    {
        try
        {
            // 检查接口有效性
            if (!docIface.is())
            {
                logger_log_error("saveDocument error: Invalid document interface");
                return;
            }

            // 转换为可存储接口
            com::sun::star::uno::Reference<com::sun::star::frame::XStorable> xStorable(docIface, com::sun::star::uno::UNO_QUERY);
            if (!xStorable.is())
            {
                logger_log_error("saveDocument error: Invalid storable interface");
                return;
            }

            rtl::OUString absolutePath;
            getAbsolutePath(filePath, absolutePath);

            // 转换 UTF8 编码，防止保存文件出现异常
            rtl::OUString url = convertStringToOUString("file://") + absolutePath;
            logger_log_info("saveDocument: Full URL: %s", rtl::OUStringToOString(url, RTL_TEXTENCODING_UTF8).getStr());

            // 设置保存属性，确保包含 Overwrite 和 FilterName 属性
            com::sun::star::uno::Sequence<com::sun::star::beans::PropertyValue> props(2);
            props[0].Name = convertStringToOUString("Overwrite");
            props[0].Value <<= true; // 强制覆盖已有文件
            props[1].Name = convertStringToOUString("FilterName");
            props[1].Value <<= convertStringToOUString("calc8"); // 指定 ODS 文件格式

            try
            {
                // 直接调用 storeAsURL，依赖 UNO API 实现覆盖逻辑
                xStorable->storeAsURL(url, props);
                logger_log_info("Document saved successfully: %s", convertOUStringToString(url).c_str());
            }
            catch (const com::sun::star::uno::Exception &e)
            {
                // 记录详细的错误信息
                rtl::OUString errorMessage = e.Message;
                if (errorMessage.getLength() > 0)
                {
                    logger_log_error("UNO Exception during save: %s",
                                     convertOUStringToString(errorMessage).c_str());
                }
                else
                {
                    logger_log_error("Unknown UNO Exception occurred during save.");
                }
            }
        }
        catch (const com::sun::star::uno::Exception &e)
        {
            logger_log_error("saveDocument UNO exception: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            logger_log_error("saveDocument std exception: %s", e.what());
        }
        catch (...)
        {
            logger_log_error("saveDocument unknown exception occurred");
        }
    }

    /// @brief 关闭文档
    void closeDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &docIface)
    {
        try
        {
            com::sun::star::uno::Reference<com::sun::star::lang::XComponent> xComp(docIface, com::sun::star::uno::UNO_QUERY);
            if (xComp.is())
            {
                xComp->dispose();
                logger_log_info("closeDocument: Document closed successfully");
            }
            else
            {
                logger_log_error("closeDocument: Invalid component interface");
            }
        }
        catch (const com::sun::star::uno::Exception &e)
        {
            logger_log_error("closeDocument UNO exception: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            logger_log_error("closeDocument std exception: %s", e.what());
        }
        catch (...)
        {
            logger_log_error("closeDocument unknown exception occurred");
        }
    }

    /// @brief 读取整个电子表格文件为JSON
    cJSON *readSpreadsheetFile(const rtl::OUString &filePath)
    {
        uno::Reference<lang::XComponent> xComp; // 在函数作用域内声明，确保在任何情况下都能正确关闭
        try
        {
            // 获取ComponentLoader
            uno::Reference<frame::XComponentLoader> xLoader = LibreOfficeConnectionManager::getComponentLoader();
            if (!xLoader.is())
            {
                return nullptr;
            }

            rtl::OUString absolutePath;
            getAbsolutePath(filePath, absolutePath);

            rtl::OUString url = convertStringToOUString("file://") + absolutePath;
            std::string urlStr = std::string(rtl::OUStringToOString(url, RTL_TEXTENCODING_UTF8).getStr());
            std::cerr << "readSpreadsheetFile: Loading from URL: " << urlStr << std::endl;

            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<lang::XComponent> xComponent = xLoader->loadComponentFromURL(url, convertStringToOUString("_blank"), 0, args);
            xComp = xComponent;
            if (!xComp.is())
            {
                std::cerr << "readSpreadsheetFile: Failed to load component from URL" << std::endl;
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is())
            {
                std::cerr << "readSpreadsheetFile: Loaded component is not a spreadsheet document" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheets> sheets = xDoc->getSheets();
            if (!sheets.is())
            {
                std::cerr << "readSpreadsheetFile: Failed to get sheets" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);
            if (!nameAccess.is())
            {
                std::cerr << "readSpreadsheetFile: Failed to get name access" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            cJSON *root = cJSON_CreateObject();
            uno::Sequence<rtl::OUString> names = nameAccess->getElementNames();
            std::cerr << "readSpreadsheetFile: Found " << names.getLength() << " sheets" << std::endl;

            for (sal_Int32 i = 0; i < names.getLength(); ++i)
            {
                std::string sheetName = std::string(rtl::OUStringToOString(names[i], RTL_TEXTENCODING_UTF8).getStr());
                std::cerr << "readSpreadsheetFile: Processing sheet: " << sheetName << std::endl;

                uno::Any any = nameAccess->getByName(names[i]);
                uno::Reference<sheet::XSpreadsheet> sheet;
                any >>= sheet;

                if (!sheet.is())
                {
                    std::cerr << "readSpreadsheetFile: Failed to get sheet: " << sheetName << std::endl;
                    continue;
                }

                cJSON *sheetJson = cJSON_CreateArray();
                for (sal_Int32 row = 0; row < 100; ++row)
                { // 只导出前100行
                    cJSON *rowJson = cJSON_CreateArray();
                    for (sal_Int32 col = 0; col < 20; ++col)
                    { // 只导出前20列
                        uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                        if (!cell.is())
                        {
                            std::cerr << "readSpreadsheetFile: Failed to get cell at " << col << "," << row << " in sheet " << sheetName << std::endl;
                            // 创建一个空的单元格对象
                            cJSON *cellJson = cJSON_CreateObject();
                            cJSON_AddNumberToObject(cellJson, "value", 0.0);
                            cJSON_AddStringToObject(cellJson, "formula", "");
                            cJSON_AddItemToArray(rowJson, cellJson);
                            continue;
                        }

                        cJSON *cellJson = cJSON_CreateObject();
                        cJSON_AddNumberToObject(cellJson, "value", cell->getValue());

                        rtl::OUString formula = cell->getFormula();
                        std::string formulaStr = std::string(rtl::OUStringToOString(formula, RTL_TEXTENCODING_UTF8).getStr());
                        cJSON_AddStringToObject(cellJson, "formula", formulaStr.c_str());
                        cJSON_AddItemToArray(rowJson, cellJson);
                    }
                    cJSON_AddItemToArray(sheetJson, rowJson);
                }

                cJSON_AddItemToObject(root, sheetName.c_str(), sheetJson);
                std::cerr << "readSpreadsheetFile: Finished processing sheet: " << sheetName << std::endl;
            }

            // closeDocument(xComp);
            std::cerr << "readSpreadsheetFile: Successfully read file" << std::endl;
            return root;
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "readSpreadsheetFile UNO exception: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            if (xComp.is())
            {
                closeDocument(xComp); // 确保在异常情况下关闭文档
            }
            return nullptr;
        }
        catch (const std::exception &e)
        {
            std::cerr << "readSpreadsheetFile std exception: " << e.what() << std::endl;
            if (xComp.is())
            {
                closeDocument(xComp); // 确保在异常情况下关闭文档
            }
            return nullptr;
        }
        catch (...)
        {
            std::cerr << "readSpreadsheetFile unknown exception occurred" << std::endl;
            if (xComp.is())
            {
                closeDocument(xComp); // 确保在异常情况下关闭文档
            }
            return nullptr;
        }
        return cJSON_CreateString("success");
        ;
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

    rtl::OUString findCharPositions(const rtl::OUString &newValue, cJSON *sheetData)
    {
        // 创建一个OUStringBuffer来构建结果字符串
        rtl::OUStringBuffer resultBuffer;

        // 遍历输入字符串中的每个字符
        for (sal_Int32 i = 0; i < newValue.getLength(); ++i)
        {
            // 获取当前字符
            rtl::OUString charStr = newValue.copy(i, 1);
            // 将字符转换为UTF8字符串用于在sheetData中查找
            std::string charStdStr = rtl::OUStringToOString(charStr, RTL_TEXTENCODING_UTF8).getStr();

            // 在sheetData中查找该字符
            cJSON *positionItem = cJSON_GetObjectItem(sheetData, charStdStr.c_str());

            if (positionItem && positionItem->valuestring)
            {
                // 如果找到位置信息，添加到结果中
                if (i > 0)
                {
                    // 除了第一个字符外，其他字符前面都加&
                    resultBuffer.append(convertStringToOUString("&"));
                }
                else
                {
                    resultBuffer.append(convertStringToOUString("="));
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
                resultBuffer.append(convertStringToOUString(positionItem->valuestring));
            }
            else
            {
                // 如果未找到字符位置，可以添加一个占位符或跳过
                if (i > 0)
                {
                    resultBuffer.append(convertStringToOUString(""));
                }
                resultBuffer.append(convertStringToOUString("N/A"));
            }
        }

        // 返回组合的位置字符串
        return resultBuffer.makeStringAndClear();
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

    // 复制工作表内容
    void copySheetContent(const uno::Reference<sheet::XSpreadsheet> &srcSheet, const uno::Reference<sheet::XSpreadsheet> &dstSheet)
    {
        if (!srcSheet.is() || !dstSheet.is())
            return;
        int maxCols = json_config_get_int("maxCols");
        int maxRows = json_config_get_int("maxRows");
        for (int row = 0; row < maxRows; ++row)
        {
            for (int col = 0; col < maxCols; ++col)
            {
                uno::Reference<table::XCell> srcCell = srcSheet->getCellByPosition(col, row);
                uno::Reference<table::XCell> dstCell = dstSheet->getCellByPosition(col, row);
                if (srcCell.is() && dstCell.is())
                {
                    rtl::OUString formula = srcCell->getFormula();
                    double value = srcCell->getValue();
                    // 跳过空单元格（公式和数值都为空）
                    if (formula.getLength() == 0 && value == 0.0)
                        continue;
                    // 复制公式和数值
                    if (formula.getLength() > 0)
                    {
                        dstCell->setFormula(formula);
                    }
                    else
                    {
                        dstCell->setValue(value);
                    }
                }
            }
        }
    }

    int copySheetToAnotherFile(const uno::Reference<sheet::XSpreadsheetDocument> &sourceDoc,
                               const rtl::OUString &sourceSheetName,
                               const uno::Reference<sheet::XSpreadsheetDocument> &targetDoc)
    {
        try
        {
            // 获取源文档和目标文档的Sheets集合
            auto sourceSheets = sourceDoc->getSheets();
            auto targetSheets = targetDoc->getSheets();

            // 检查目标文档中是否存在同名工作表
            if (!targetSheets->hasByName(sourceSheetName))
            {
                // 如果不存在，则创建工作表
                targetSheets->insertByName(sourceSheetName, sourceSheets->getByName(sourceSheetName));
            }
            else
            {
                // 如果存在，清空目标工作表内容并覆盖
                uno::Any anyTargetSheet = targetSheets->getByName(sourceSheetName);
                uno::Reference<sheet::XSpreadsheet> targetSheet;
                anyTargetSheet >>= targetSheet;
                uno::Any anySourceSheet = sourceSheets->getByName(sourceSheetName);
                uno::Reference<sheet::XSpreadsheet> sourceSheet;
                anySourceSheet >>= sourceSheet;
                clearSheet(targetSheet);
                copySheetContent(sourceSheet, targetSheet);
            }
        }
        catch (const css::uno::Exception &e)
        {
            // 异常处理
            logger_log_error("Error copying sheet: %s", rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return -1;
        }
        return 0;
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

    // 判断字符属于哪种语言类型（简易实现，可扩展）
    std::string getLanguageType(char32_t ch)
    {
        // 0x0000-0xFFFF 0 基本多文種平面
        if (ch >= 0x0000 && ch <= 0x007F)
            return "Basic Latin alphabet";
        if (ch >= 0x0080 && ch <= 0x00FF)
            return "Latin-1 Supplement";
        if (ch >= 0x0100 && ch <= 0x017F)
            return "Latin Extended-A";
        if (ch >= 0x0180 && ch <= 0x024F)
            return "Latin Extended-B";
        if (ch >= 0x0250 && ch <= 0x02AF)
            return "IPA Extensions";
        if (ch >= 0x02B0 && ch <= 0x02FF)
            return "Spacing Modifier Letters";
        if (ch >= 0x0300 && ch <= 0x036F)
            return "Combining Diacritical Marks";
        if (ch >= 0x0370 && ch <= 0x03FF)
            return "Greek and Coptic";
        if (ch >= 0x0400 && ch <= 0x04FF)
            return "Cyrillic";
        if (ch >= 0x0500 && ch <= 0x052F)
            return "Cyrillic Supplement";
        if (ch >= 0x0530 && ch <= 0x058F)
            return "Armenian";
        if (ch >= 0x0590 && ch <= 0x05FF)
            return "Hebrew";
        if (ch >= 0x0600 && ch <= 0x06FF)
            return "Arabic";
        if (ch >= 0x0700 && ch <= 0x074F)
            return "Syriac";
        if (ch >= 0x0750 && ch <= 0x077F)
            return "Arabic Supplement";
        if (ch >= 0x0780 && ch <= 0x07BF)
            return "Thaana";
        if (ch >= 0x07C0 && ch <= 0x07FF)
            return "West African written language";
        if (ch >= 0x0800 && ch <= 0x083F)
            return "Samaritan";
        if (ch >= 0x0840 && ch <= 0x085F)
            return "Mandaic";
        if (ch >= 0x0860 && ch <= 0x086F)
            return "Syriac Supplement";
        if (ch >= 0x0870 && ch <= 0x089F)
            return "Arabic Extended-B";
        if (ch >= 0x08A0 && ch <= 0x08FF)
            return "Arabic Extended-A";
        if (ch >= 0x0900 && ch <= 0x097F)
            return "Devanagari";
        if (ch >= 0x0980 && ch <= 0x09FF)
            return "Bengali";
        if (ch >= 0x0A00 && ch <= 0x0A7F)
            return "Gurmukhi";
        if (ch >= 0x0A80 && ch <= 0x0AFF)
            return "Gujarati";
        if (ch >= 0x0B00 && ch <= 0x0B7F)
            return "Oriya";
        if (ch >= 0x0B80 && ch <= 0x0BFF)
            return "Tamil";
        if (ch >= 0x0C00 && ch <= 0x0C7F)
            return "Telugu";
        if (ch >= 0x0C80 && ch <= 0x0CFF)
            return "Kannada";
        if (ch >= 0x0D00 && ch <= 0x0D7F)
            return "Malayalam";
        if (ch >= 0x0D80 && ch <= 0x0DFF)
            return "Sinhala";
        if (ch >= 0x0E00 && ch <= 0x0E7F)
            return "Thai";
        if (ch >= 0x0E80 && ch <= 0x0EFF)
            return "Lao";
        if (ch >= 0x0F00 && ch <= 0x0FFF)
            return "Tibetan";
        if (ch >= 0x1000 && ch <= 0x109F)
            return "Myanmar";
        if (ch >= 0x10A0 && ch <= 0x10FF)
            return "Georgian";
        if (ch >= 0x1100 && ch <= 0x11FF)
            return "Hangul Jamo";
        if (ch >= 0x1200 && ch <= 0x137F)
            return "Ethiopic";
        if (ch >= 0x1380 && ch <= 0x139F)
            return "Ethiopic Supplement";
        if (ch >= 0x13A0 && ch <= 0x13FF)
            return "Cherokee";
        if (ch >= 0x1400 && ch <= 0x167F)
            return "Unified Canadian Aboriginal Syllabics";
        if (ch >= 0x1680 && ch <= 0x169F)
            return "Ogham";
        if (ch >= 0x16A0 && ch <= 0x16FF)
            return "Runic";
        if (ch >= 0x1700 && ch <= 0x171F)
            return "Tagalog";
        if (ch >= 0x1720 && ch <= 0x173F)
            return "Hanunoo";
        if (ch >= 0x1740 && ch <= 0x175F)
            return "Buhid";
        if (ch >= 0x1760 && ch <= 0x177F)
            return "Tagbanwa";
        if (ch >= 0x1780 && ch <= 0x17FF)
            return "Khmer";
        if (ch >= 0x1800 && ch <= 0x18AF)
            return "Mongolian";
        if (ch >= 0x18B0 && ch <= 0x18FF)
            return "Unified Canadian Aboriginal Syllabics Extended";
        if (ch >= 0x1900 && ch <= 0x194F)
            return "Limbu";
        if (ch >= 0x1950 && ch <= 0x197F)
            return "Tai Le";
        if (ch >= 0x1980 && ch <= 0x19DF)
            return "New Tai Lue";
        if (ch >= 0x19E0 && ch <= 0x19FF)
            return "Khmer Symbols";
        if (ch >= 0x1A00 && ch <= 0x1A1F)
            return "Buginese";
        if (ch >= 0x1A20 && ch <= 0x1AAF)
            return "Tai Tham";
        if (ch >= 0x1AB0 && ch <= 0x1AFF)
            return "Combining Diacritical Marks Extended";
        if (ch >= 0x1B00 && ch <= 0x1B7F)
            return "Balinese";
        if (ch >= 0x1B80 && ch <= 0x1BBF)
            return "Sundanese";
        if (ch >= 0x1BC0 && ch <= 0x1BFF)
            return "Batak";
        if (ch >= 0x1C00 && ch <= 0x1C4F)
            return "Lepcha";
        if (ch >= 0x1C50 && ch <= 0x1C7F)
            return "Ol Chiki";
        if (ch >= 0x1C80 && ch <= 0x1C8F)
            return "Cyrillic Extended-C";
        if (ch >= 0x1C90 && ch <= 0x1CBF)
            return "Georgian Extended";
        if (ch >= 0x1CC0 && ch <= 0x1CCF)
            return "Sundanese Supplement";
        if (ch >= 0x1CD0 && ch <= 0x1CFF)
            return "Vedic Extensions";
        if (ch >= 0x1D00 && ch <= 0x1D7F)
            return "Phonetic Extensions";
        if (ch >= 0x1D80 && ch <= 0x1DBF)
            return "Phonetic Extensions Supplement";
        if (ch >= 0x1DC0 && ch <= 0x1DFF)
            return "Combining Diacritical Marks Supplement";
        if (ch >= 0x1E00 && ch <= 0x1EFF)
            return "Latin Extended Additional";
        if (ch >= 0x1F00 && ch <= 0x1FFF)
            return "Greek Extended";
        if (ch >= 0x2000 && ch <= 0x206F)
            return "General Punctuation";
        if (ch >= 0x2070 && ch <= 0x209F)
            return "Superscripts and Subscripts";
        if (ch >= 0x20A0 && ch <= 0x20CF)
            return "Currency Symbols";
        if (ch >= 0x20D0 && ch <= 0x20FF)
            return "Combining Diacritical Marks for Symbols";
        if (ch >= 0x2100 && ch <= 0x214F)
            return "Letterlike Symbols";
        if (ch >= 0x2150 && ch <= 0x218F)
            return "Number Forms";
        if (ch >= 0x2190 && ch <= 0x21FF)
            return "Arrows";
        if (ch >= 0x2200 && ch <= 0x22FF)
            return "Mathematical Operators";
        if (ch >= 0x2300 && ch <= 0x23FF)
            return "Miscellaneous Technical";
        if (ch >= 0x2400 && ch <= 0x243F)
            return "Control Pictures";
        if (ch >= 0x2440 && ch <= 0x245F)
            return "Optical Character Recognition";
        if (ch >= 0x2460 && ch <= 0x24FF)
            return "Enclosed Alphanumerics";
        if (ch >= 0x2500 && ch <= 0x257F)
            return "Table Symbols";
        if (ch >= 0x2580 && ch <= 0x259F)
            return "Block Elements";
        if (ch >= 0x25A0 && ch <= 0x25FF)
            return "Geometric Shapes";
        if (ch >= 0x2600 && ch <= 0x26FF)
            return "Miscellaneous Symbols";
        if (ch >= 0x2700 && ch <= 0x27BF)
            return "Printing Punctuation";
        if (ch >= 0x27C0 && ch <= 0x27EF)
            return "Miscellaneous Mathematical Symbols-A";
        if (ch >= 0x27F0 && ch <= 0x27FF)
            return "Supplemental Arrows-A";
        if (ch >= 0x2800 && ch <= 0x28FF)
            return "Braille Patterns";
        if (ch >= 0x2900 && ch <= 0x297F)
            return "Supplemental Arrows-B";
        if (ch >= 0x2980 && ch <= 0x29FF)
            return "Miscellaneous Mathematical Symbols-B";
        if (ch >= 0x2A00 && ch <= 0x2AFF)
            return "Supplemental Mathematical Operators";
        if (ch >= 0x2B00 && ch <= 0x2BFF)
            return "Miscellaneous Symbols and Arrows";
        if (ch >= 0x2C00 && ch <= 0x2C5F)
            return "Glagolitic";
        if (ch >= 0x2C60 && ch <= 0x2C7F)
            return "Latin Extended-C";
        if (ch >= 0x2C80 && ch <= 0x2CFF)
            return "Coptic";
        if (ch >= 0x2D00 && ch <= 0x2D2F)
            return "Georgian Supplement";
        if (ch >= 0x2D30 && ch <= 0x2D7F)
            return "Tifinagh";
        if (ch >= 0x2D80 && ch <= 0x2DDF)
            return "Ethiopic Extended";
        if (ch >= 0x2DE0 && ch <= 0x2DFF)
            return "Cyrillic Extended-A";
        if (ch >= 0x2E00 && ch <= 0x2E7F)
            return "Supplemental Punctuation";
        if (ch >= 0x2E80 && ch <= 0x2EFF)
            return "CJK Radicals Supplement";
        if (ch >= 0x2F00 && ch <= 0x2FDF)
            return "Kangxi Radicals";
        if (ch >= 0x2FE0 && ch <= 0x2FEF)
            return "Undefined";
        if (ch >= 0x2FF0 && ch <= 0x2FFF)
            return "Ideographic Description Characters";
        if (ch >= 0x3000 && ch <= 0x303F)
            return "CJK Symbols and Punctuation";
        if (ch >= 0x3040 && ch <= 0x309F)
            return "Hiragana";
        if (ch >= 0x30A0 && ch <= 0x30FF)
            return "Katakana";
        if (ch >= 0x3100 && ch <= 0x312F)
            return "Bopomofo";
        if (ch >= 0x3130 && ch <= 0x318F)
            return "Hangul Compatibility Jamo";
        if (ch >= 0x3190 && ch <= 0x319F)
            return "Annotated pictographic symbols";
        if (ch >= 0x31A0 && ch <= 0x31BF)
            return "Bopomofo Extended";
        if (ch >= 0x31C0 && ch <= 0x31EF)
            return "CJK Strokes";
        if (ch >= 0x31F0 && ch <= 0x31FF)
            return "Katakana Phonetic Extensions";
        if (ch >= 0x3200 && ch <= 0x32FF)
            return "Enclosed CJK Letters and Months";
        if (ch >= 0x3300 && ch <= 0x33FF)
            return "CJK Compatibility";
        if (ch >= 0x3400 && ch <= 0x4DBF)
            return "CJK Unified Ideographs Extension A";
        if (ch >= 0x4DC0 && ch <= 0x4DFF)
            return "Yijing Hexagram Symbols";
        if (ch >= 0x4E00 && ch <= 0x9FFF)
            return "CJK Unified Ideographs";
        if (ch >= 0xA000 && ch <= 0xA48F)
            return "Yi Syllables";
        if (ch >= 0xA490 && ch <= 0xA4CF)
            return "Yi Radicals";
        if (ch >= 0xA4D0 && ch <= 0xA4FF)
            return "Lisu";
        if (ch >= 0xA500 && ch <= 0xA63F)
            return "Vai";
        if (ch >= 0xA640 && ch <= 0xA69F)
            return "Cyrillic Extended-B";
        if (ch >= 0xA6A0 && ch <= 0xA6FF)
            return "Bamum";
        if (ch >= 0xA700 && ch <= 0xA71F)
            return "Modifier Tone Letters";
        if (ch >= 0xA720 && ch <= 0xA7FF)
            return "Latin Extended-D";
        if (ch >= 0xA800 && ch <= 0xA82F)
            return "Syloti Nagri";
        if (ch >= 0xA830 && ch <= 0xA83F)
            return "Common Indic Number Forms";
        if (ch >= 0xA840 && ch <= 0xA87F)
            return "Phags-pa";
        if (ch >= 0xA880 && ch <= 0xA8DF)
            return "Saurashtra";
        if (ch >= 0xA8E0 && ch <= 0xA8FF)
            return "Devanagari";
        if (ch >= 0xA900 && ch <= 0xA92F)
            return "Kayah Li";
        if (ch >= 0xA930 && ch <= 0xA95F)
            return "Rejang";
        if (ch >= 0xA960 && ch <= 0xA97F)
            return "Hangul Jamo Extended-A";
        if (ch >= 0xA980 && ch <= 0xA9DF)
            return "Javanese";
        if (ch >= 0xA9E0 && ch <= 0xA9FF)
            return "Myanmar Extended-B";
        if (ch >= 0xAA00 && ch <= 0xAA5F)
            return "Cham";
        if (ch >= 0xAA60 && ch <= 0xAA7F)
            return "Myanmar Extended-A";
        if (ch >= 0xAA80 && ch <= 0xAADF)
            return "Tai Viet";
        if (ch >= 0xAAE0 && ch <= 0xAAFF)
            return "Meetei Mayek Extensions";
        if (ch >= 0xAB00 && ch <= 0xAB2F)
            return "Ethiopic Extended-A";
        if (ch >= 0xAB30 && ch <= 0xAB6F)
            return "Latin Extended-E";
        if (ch >= 0xAB70 && ch <= 0xABBF)
            return "Cherokee Supplement";
        if (ch >= 0xABC0 && ch <= 0xABFF)
            return "Meetei Mayek";
        if (ch >= 0xAC00 && ch <= 0xD7AF)
            return "Hangul Syllables";
        if (ch >= 0xD7B0 && ch <= 0xD7FF)
            return "Hangul Jamo Extended-B";
        if (ch >= 0xD800 && ch <= 0xDB7F)
            return "High Surrogates";
        if (ch >= 0xDB80 && ch <= 0xDBFF)
            return "High Private Use Surrogates";
        if (ch >= 0xDC00 && ch <= 0xDFFF)
            return "Low Surrogates";
        if (ch >= 0xE000 && ch <= 0xF8FF)
            return "Private Use Area";
        if (ch >= 0xF900 && ch <= 0xFAFF)
            return "CJK Compatibility Ideographs";
        if (ch >= 0xFB00 && ch <= 0xFB4F)
            return "Alphabetic Presentation Forms";
        if (ch >= 0xFB50 && ch <= 0xFDFF)
            return "Arabic Presentation Forms-A";
        if (ch >= 0xFE00 && ch <= 0xFE0F)
            return "Variation Selectors";
        if (ch >= 0xFE10 && ch <= 0xFE1F)
            return "Vertical Forms";
        if (ch >= 0xFE20 && ch <= 0xFE2F)
            return "Combining Half Marks";
        if (ch >= 0xFE30 && ch <= 0xFE4F)
            return "CJK Compatibility Forms";
        if (ch >= 0xFE50 && ch <= 0xFE6F)
            return "Small Form Variants";
        if (ch >= 0xFE70 && ch <= 0xFEFF)
            return "Arabic Presentation Forms-B";
        if (ch >= 0xFF00 && ch <= 0xFFEF)
            return "Halfwidth and Fullwidth Forms";
        if (ch >= 0xFFF0 && ch <= 0xFFFF)
            return "Specials";
        // 0x10000 - 0x1FFFF 1 多文種補充平面
        if (ch >= 0x10000 && ch <= 0x1007F)
            return "Linear B";
        if (ch >= 0x10080 && ch <= 0x100FF)
            return "Linear B Syllabary";
        if (ch >= 0x10100 && ch <= 0x1013F)
            return "Aegean Numbers";
        if (ch >= 0x10140 && ch <= 0x1018F)
            return "Ancient Greek Numbers";
        if (ch >= 0x10190 && ch <= 0x101CF)
            return "Ancient Symbols";
        if (ch >= 0x101D0 && ch <= 0x101FF)
            return "Phaistos Disc";
        if (ch >= 0x10200 && ch <= 0x1027F)
            return "Undefined";
        if (ch >= 0x10280 && ch <= 0x1029F)
            return "Lycian";
        if (ch >= 0x102A0 && ch <= 0x102DF)
            return "Carian";
        if (ch >= 0x102E0 && ch <= 0x102FF)
            return "Coptic Epact Numbers";
        if (ch >= 0x10300 && ch <= 0x1032F)
            return "Old Italic";
        if (ch >= 0x10330 && ch <= 0x1034F)
            return "Gothic";
        if (ch >= 0x10350 && ch <= 0x1037F)
            return "Old Permic";
        if (ch >= 0x10380 && ch <= 0x1039F)
            return "Ugaritic";
        if (ch >= 0x103A0 && ch <= 0x103DF)
            return "Old Persian";
        if (ch >= 0x103E0 && ch <= 0x103FF)
            return "Undefined";
        if (ch >= 0x10400 && ch <= 0x1044F)
            return "Deseret";
        if (ch >= 0x10450 && ch <= 0x1047F)
            return "Shavian";
        if (ch >= 0x10480 && ch <= 0x104AF)
            return "Osmanya";
        if (ch >= 0x104B0 && ch <= 0x104FF)
            return "Osage";
        if (ch >= 0x10500 && ch <= 0x1052F)
            return "Elbasan";
        if (ch >= 0x10530 && ch <= 0x1056F)
            return "Caucasian Albanian";
        if (ch >= 0x10570 && ch <= 0x105BF)
            return "Viskuchian";
        if (ch >= 0x105C0 && ch <= 0x105FF)
            return "Todhri";
        if (ch >= 0x10600 && ch <= 0x1077F)
            return "Linear A";
        if (ch >= 0x10780 && ch <= 0x107BF)
            return "Latin alphabet extension F";
        if (ch >= 0x107C0 && ch <= 0x107FF)
            return "Undefined";
        if (ch >= 0x10800 && ch <= 0x1083F)
            return "Cypriot Syllabary";
        if (ch >= 0x10840 && ch <= 0x1085F)
            return "Imperial Aramaic";
        if (ch >= 0x10860 && ch <= 0x1087F)
            return "Palmyrene";
        if (ch >= 0x10880 && ch <= 0x108AF)
            return "Nabataean";
        if (ch >= 0x108B0 && ch <= 0x108FF)
            return "Undefined";
        if (ch >= 0x108E0 && ch <= 0x108FF)
            return "Hatran";
        if (ch >= 0x10900 && ch <= 0x1091F)
            return "Phoenician";
        if (ch >= 0x10920 && ch <= 0x1093F)
            return "Lydian";
        if (ch >= 0x10940 && ch <= 0x1097F)
            return "Undefined";
        if (ch >= 0x10980 && ch <= 0x1099F)
            return "Meroitic Hieroglyphs";
        if (ch >= 0x109A0 && ch <= 0x109FF)
            return "Meroitic Cursive";
        if (ch >= 0x10A00 && ch <= 0x10A5F)
            return "Kharoshthi";
        if (ch >= 0x10A60 && ch <= 0x10A7F)
            return "Old South Arabian";
        if (ch >= 0x10A80 && ch <= 0x10A9F)
            return "Old North Arabian";
        if (ch >= 0x10AA0 && ch <= 0x10ABF)
            return "Undefined";
        if (ch >= 0x10AC0 && ch <= 0x10AFF)
            return "Manichaean";
        if (ch >= 0x10B00 && ch <= 0x10B3F)
            return "Avestan";
        if (ch >= 0x10B40 && ch <= 0x10B5F)
            return "Inscriptional Parthian";
        if (ch >= 0x10B60 && ch <= 0x10B7F)
            return "Inscriptional Pahlavi";
        if (ch >= 0x10B80 && ch <= 0x10BAF)
            return "Psalter Pahlavi";
        if (ch >= 0x10BB0 && ch <= 0x10BFF)
            return "Undefined";
        if (ch >= 0x10C00 && ch <= 0x10C4F)
            return "Old Turkic";
        if (ch >= 0x10C50 && ch <= 0x10C7F)
            return "Undefined";
        if (ch >= 0x10C80 && ch <= 0x10CFF)
            return "Old Hungarian";
        if (ch >= 0x10D00 && ch <= 0x10D3F)
            return "Hanifi Rohingya";
        if (ch >= 0x10D40 && ch <= 0x10D8F)
            return "Garay";
        if (ch >= 0x10D90 && ch <= 0x10E5F)
            return "Undefined";
        if (ch >= 0x10E60 && ch <= 0x10E7F)
            return "Rumi Numeral Symbols";
        if (ch >= 0x10E80 && ch <= 0x10EBF)
            return "Yezidi";
        if (ch >= 0x10EC0 && ch <= 0x10EFF)
            return "Arabic alphabet extension C";
        if (ch >= 0x10F00 && ch <= 0x10F2F)
            return "Old Sogdian";
        if (ch >= 0x10F30 && ch <= 0x10F6F)
            return "Sogdian";
        if (ch >= 0x10F70 && ch <= 0x10FAF)
            return "Uyghur alphabet";
        if (ch >= 0x10FB0 && ch <= 0x10FDF)
            return "Khwarezmia alphabet";
        if (ch >= 0x10FE0 && ch <= 0x10FFF)
            return "Ellie Maiwen";
        if (ch >= 0x11000 && ch <= 0x1107F)
            return "Brahmi";
        if (ch >= 0x11080 && ch <= 0x110CF)
            return "Kaithi";
        if (ch >= 0x110D0 && ch <= 0x110FF)
            return "Sora Sompeng";
        if (ch >= 0x11100 && ch <= 0x1114F)
            return "Chakma";
        if (ch >= 0x11150 && ch <= 0x1117F)
            return "Mahajani";
        if (ch >= 0x11180 && ch <= 0x111DF)
            return "Sharada";
        if (ch >= 0x111E0 && ch <= 0x111FF)
            return "Sinhala Archaic Numbers";
        if (ch >= 0x11200 && ch <= 0x1124F)
            return "Khojki";
        if (ch >= 0x11250 && ch <= 0x1127F)
            return "Undefined";
        if (ch >= 0x11280 && ch <= 0x112AF)
            return "Multani";
        if (ch >= 0x112B0 && ch <= 0x112FF)
            return "Khudawadi";
        if (ch >= 0x11300 && ch <= 0x1137F)
            return "Grantha";
        if (ch >= 0x11380 && ch <= 0x113FF)
            return "Tulu-Tigalari";
        if (ch >= 0x11400 && ch <= 0x1147F)
            return "Newa";
        if (ch >= 0x11480 && ch <= 0x114DF)
            return "Tirhuta";
        if (ch >= 0x114E0 && ch <= 0x1157F)
            return "Undefined";
        if (ch >= 0x11580 && ch <= 0x115FF)
            return "Siddham";
        if (ch >= 0x11600 && ch <= 0x1165F)
            return "Modi";
        if (ch >= 0x11660 && ch <= 0x1167F)
            return "Mongolian Supplement";
        if (ch >= 0x11680 && ch <= 0x116CF)
            return "Takri";
        if (ch >= 0x116D0 && ch <= 0x116FF)
            return "Myanmar Extended-C";
        if (ch >= 0x11700 && ch <= 0x1174F)
            return "Ahom";
        if (ch >= 0x11750 && ch <= 0x117FF)
            return "Undefined";
        if (ch >= 0x11800 && ch <= 0x1184F)
            return "Douglas";
        if (ch >= 0x11850 && ch <= 0x1189F)
            return "Undefined";
        if (ch >= 0x118A0 && ch <= 0x118FF)
            return "Warang Citi";
        if (ch >= 0x11900 && ch <= 0x1195F)
            return "Island letters";
        if (ch >= 0x11960 && ch <= 0x1199F)
            return "Undefined";
        if (ch >= 0x119A0 && ch <= 0x119FF)
            return "Nandinagari";
        if (ch >= 0x11A00 && ch <= 0x11A4F)
            return "Zanabazar Square";
        if (ch >= 0x11A50 && ch <= 0x11AAF)
            return "Pahawh Hmong";
        if (ch >= 0x11AB0 && ch <= 0x11ABF)
            return "Canadian Indigenous Syllable Script Extension A";
        if (ch >= 0x11AC0 && ch <= 0x11AFF)
            return "Pau Cin Hau";
        if (ch >= 0x11B00 && ch <= 0x11B5F)
            return "Devanagari Extended-A";
        if (ch >= 0x11B60 && ch <= 0x11BBF)
            return "Undefined";
        if (ch >= 0x11BC0 && ch <= 0x11BFF)
            return "Sunuwar";
        if (ch >= 0x11C00 && ch <= 0x11C6F)
            return "Bhaiksuki";
        if (ch >= 0x11C70 && ch <= 0x11CBF)
            return "Marchen";
        if (ch >= 0x11CC0 && ch <= 0x11CFF)
            return "Undefined";
        if (ch >= 0x11D00 && ch <= 0x11D5F)
            return "Masaram Gondi";
        if (ch >= 0x11D60 && ch <= 0x11DAF)
            return "Gunjala Gondi";
        if (ch >= 0x11DB0 && ch <= 0x11EDF)
            return "Undefined";
        if (ch >= 0x11EE0 && ch <= 0x11EFF)
            return "Wangjia Xiwen";
        if (ch >= 0x11F00 && ch <= 0x11F5F)
            return "Kavi Wen";
        if (ch >= 0x11F60 && ch <= 0x11FAF)
            return "Undefined";
        if (ch >= 0x11FB0 && ch <= 0x11FBF)
            return "Lisu";
        if (ch >= 0x11FC0 && ch <= 0x11FFF)
            return "Tamil language supplement";
        if (ch >= 0x12000 && ch <= 0x123FF)
            return "Cuneiform";
        if (ch >= 0x12400 && ch <= 0x1247F)
            return "Cuneiform Numbers and Punctuation";
        if (ch >= 0x12480 && ch <= 0x1254F)
            return "Early Dynastic Cuneiform";
        if (ch >= 0x12550 && ch <= 0x12F8F)
            return "Undefined";
        if (ch >= 0x12F90 && ch <= 0x12FFF)
            return "Cyprus Minoan script";
        if (ch >= 0x13000 && ch <= 0x1342F)
            return "Egyptian holy script";
        if (ch >= 0x13430 && ch <= 0x1345F)
            return "Format control of Egyptian holy books";
        if (ch >= 0x13460 && ch <= 0x143FF)
            return "Egyptian Hieroglyphs Extended-A";
        if (ch >= 0x14400 && ch <= 0x1467F)
            return "Anatolian Hieroglyphs";
        if (ch >= 0x14680 && ch <= 0x160FF)
            return "Undefined";
        if (ch >= 0x16100 && ch <= 0x1613F)
            return "Gurung Khema";
        if (ch >= 0x16140 && ch <= 0x167FF)
            return "Undefined";
        if (ch >= 0x16800 && ch <= 0x16A3F)
            return "Bamum";
        if (ch >= 0x16A40 && ch <= 0x16A6F)
            return "Mro";
        if (ch >= 0x16AD0 && ch <= 0x16AFF)
            return "Bassa Vah";
        if (ch >= 0x16B00 && ch <= 0x16B8F)
            return "Salvation Miao Wen";
        if (ch >= 0x16B90 && ch <= 0x16D3F)
            return "Undefined";
        if (ch >= 0x16D40 && ch <= 0x16D7F)
            return "Kirat Rai";
        if (ch >= 0x16D80 && ch <= 0x16E3F)
            return "Undefined";
        if (ch >= 0x16E40 && ch <= 0x16E9F)
            return "Medfa Idelin Wen";
        if (ch >= 0x16EA0 && ch <= 0x16EFF)
            return "Undefined";
        if (ch >= 0x16F00 && ch <= 0x16F9F)
            return "Boli Miao Wen";
        if (ch >= 0x16FA0 && ch <= 0x16FDF)
            return "Undefined";
        if (ch >= 0x16FE0 && ch <= 0x16FFF)
            return "Ideographic symbols and punctuation marks";
        if (ch >= 0x17000 && ch <= 0x187FF)
            return "Tangut";
        if (ch >= 0x18800 && ch <= 0x18AFF)
            return "Tangut Components";
        if (ch >= 0x18B00 && ch <= 0x18CFF)
            return "Khitai";
        if (ch >= 0x18D00 && ch <= 0x18D7F)
            return "Tangut Extension";
        if (ch >= 0x18D80 && ch <= 0x1AFEF)
            return "Undefined";
        if (ch >= 0x1AFF0 && ch <= 0x1AFFF)
            return "Kana Extended-B";
        if (ch >= 0x1B000 && ch <= 0x1B0FF)
            return "Kana supplement";
        if (ch >= 0x1B100 && ch <= 0x1B12F)
            return "kana Extended-A";
        if (ch >= 0x1B130 && ch <= 0x1B16F)
            return "Small Kana supplement";
        if (ch >= 0x1B170 && ch <= 0x1B2FF)
            return "Women's Script";
        if (ch >= 0x1B300 && ch <= 0x1BBFF)
            return "Undefined";
        if (ch >= 0x1BC00 && ch <= 0x1BC9F)
            return "Dupre shorthand";
        if (ch >= 0x1BCA0 && ch <= 0x1BCAF)
            return "Shorthand format control";
        if (ch >= 0x1BCB0 && ch <= 0x1CBFF)
            return "Undefined";
        if (ch >= 0x1CC00 && ch <= 0x1CEBF)
            return "Symbols for Legacy Computing Supplement";
        if (ch >= 0x1CEC0 && ch <= 0x1CEFF)
            return "Undefined";
        if (ch >= 0x1CF00 && ch <= 0x1CFCF)
            return "Zanmei Ni Song Music Symbols";
        if (ch >= 0x1CFD0 && ch <= 0x1CFFF)
            return "Undefined";
        if (ch >= 0x1D000 && ch <= 0x1D0FF)
            return "Geometric Shapes Extended";
        if (ch >= 0x1D100 && ch <= 0x1D1FF)
            return "Musical Symbols";
        if (ch >= 0x1D200 && ch <= 0x1D24F)
            return "Old Greek Musical Notation";
        if (ch >= 0x1D250 && ch <= 0x1D2BF)
            return "Undefined";
        if (ch >= 0x1D2C0 && ch <= 0x1D2DF)
            return "Kaktovik Numbers";
        if (ch >= 0x1D2E0 && ch <= 0x1D2FF)
            return "Maya numerals";
        if (ch >= 0x1D300 && ch <= 0x1D35F)
            return "Tai Xuan Jing symbols";
        if (ch >= 0x1D360 && ch <= 0x1D37F)
            return "Counting Rod Numerals";
        if (ch >= 0x1D380 && ch <= 0x1D3FF)
            return "Undefined";
        if (ch >= 0x1D400 && ch <= 0x1D7FF)
            return "Arabic Mathematical Alphabetic Symbols";
        if (ch >= 0x1D800 && ch <= 0x1DAAF)
            return "Sutton SignWriting";
        if (ch >= 0x1DAB0 && ch <= 0x1DEFF)
            return "Undefined";
        if (ch >= 0x1DF00 && ch <= 0x1DFFF)
            return "Latin Alphabetic Extended-G";
        if (ch >= 0x1E000 && ch <= 0x1E02F)
            return "Glagolitic Supplement";
        if (ch >= 0x1E030 && ch <= 0x1E08F)
            return "Cyrillic Extended-D";
        if (ch >= 0x1E090 && ch <= 0x1E0FF)
            return "Undefined";
        if (ch >= 0x1E100 && ch <= 0x1E14F)
            return "Genesis Miaowen";
        if (ch >= 0x1E150 && ch <= 0x1E28F)
            return "Undefined";
        if (ch >= 0x1E290 && ch <= 0x1E2BF)
            return "Investment article";
        if (ch >= 0x1E2C0 && ch <= 0x1E2FF)
            return "Wen Qiao alphabet";
        if (ch >= 0x1E300 && ch <= 0x1E4CF)
            return "Undefined";
        if (ch >= 0x1E4D0 && ch <= 0x1E4FF)
            return "Mundali alphabet";
        if (ch >= 0x1E500 && ch <= 0x1E5CF)
            return "Undefined";
        if (ch >= 0x1E5D0 && ch <= 0x1E5FF)
            return "Ol Onal";
        if (ch >= 0x1E600 && ch <= 0x1E7DF)
            return "Undefined";
        if (ch >= 0x1E7E0 && ch <= 0x1E7FF)
            return "Ethiopian Alphabet Extension B";
        if (ch >= 0x1E800 && ch <= 0x1E8DF)
            return "Makasar";
        if (ch >= 0x1E8E0 && ch <= 0x1E8FF)
            return "Undefined";
        if (ch >= 0x1E900 && ch <= 0x1E95F)
            return "Rumi Numeral Symbols";
        if (ch >= 0x1E960 && ch <= 0x1EC6F)
            return "Undefined";
        if (ch >= 0x1EC70 && ch <= 0x1ECBF)
            return "Indian Siamese numerals";
        if (ch >= 0x1ECC0 && ch <= 0x1ECFF)
            return "Undefined";
        if (ch >= 0x1ED00 && ch <= 0x1ED4F)
            return "Osmansiag numbers";
        if (ch >= 0x1ED50 && ch <= 0x1EDFF)
            return "Undefined";
        if (ch >= 0x1EE00 && ch <= 0x1EEFF)
            return "Arabic Mathematical Alphabetic Symbols";
        if (ch >= 0x1EF00 && ch <= 0x1EFFF)
            return "Undefined";
        if (ch >= 0x1F000 && ch <= 0x1F02F)
            return "Mahjong Tiles";
        if (ch >= 0x1F030 && ch <= 0x1F09F)
            return "Domino Tiles";
        if (ch >= 0x1F0A0 && ch <= 0x1F0FF)
            return "Playing Cards";
        if (ch >= 0x1F100 && ch <= 0x1F1FF)
            return "Enclosed Alphanumeric Supplement";
        if (ch >= 0x1F200 && ch <= 0x1F2FF)
            return "Enclosed Ideographic Supplement";
        if (ch >= 0x1F300 && ch <= 0x1F5FF)
            return "Miscellaneous Symbols and Pictographs";
        if (ch >= 0x1F600 && ch <= 0x1F64F)
            return "Emoticons";
        if (ch >= 0x1F650 && ch <= 0x1F67F)
            return "Ornamental Dingbats";
        if (ch >= 0x1F680 && ch <= 0x1F6FF)
            return "Transport and Map Symbols";
        if (ch >= 0x1F700 && ch <= 0x1F77F)
            return "Alchemical Symbols";
        if (ch >= 0x1F780 && ch <= 0x1F7FF)
            return "Geometric Shapes Extended";
        if (ch >= 0x1F800 && ch <= 0x1F8FF)
            return "Supplemental Arrows-C";
        if (ch >= 0x1F900 && ch <= 0x1F9FF)
            return "Supplemental Symbols and Pictographs";
        if (ch >= 0x1FA00 && ch <= 0x1FA6F)
            return "Chess Symbols";
        if (ch >= 0x1FA70 && ch <= 0x1FAFF)
            return "Symbols and Pictographs Extended-A";
        if (ch >= 0x1FB00 && ch <= 0x1FBFF)
            return "Symbols for Legacy Computing";
        if (ch >= 0x1FC00 && ch <= 0x1FFFF)
            return "Undefined";
        // 0x20000-0x2FFFF 2 表意文字補充平面
        if (ch >= 0x20000 && ch <= 0x2A6DF)
            return "CJK Unified Ideographs Extension B";
        if (ch >= 0x2A6E0 && ch <= 0x2A6FF)
            return "Undefined";
        if (ch >= 0x2A700 && ch <= 0x2B73F)
            return "CJK Unified Ideographs Extension C";
        if (ch >= 0x2B740 && ch <= 0x2B81F)
            return "CJK Unified Ideographs Extension D";
        if (ch >= 0x2B820 && ch <= 0x2CEAF)
            return "CJK Unified Ideographs Extension E";
        if (ch >= 0x2CEB0 && ch <= 0x2EBEF)
            return "CJK Unified Ideographs Extension F";
        if (ch >= 0x2EBF0 && ch <= 0x2EE5F)
            return "CJK Unified Ideographs Extension I";
        if (ch >= 0x2EE60 && ch <= 0x2F7FF)
            return "Undefined";
        if (ch >= 0x2F800 && ch <= 0x2FA1F)
            return "CJK Compatibility Ideographs Supplement";
        if (ch >= 0x2FA20 && ch <= 0x2FFFF)
            return "Undefined";
        // 0x30000-0x3FFFF 3 表意文字第三平面
        if (ch >= 0x30000 && ch <= 0x3134F)
            return "CJK Unified Ideographs Extension G";
        if (ch >= 0x31350 && ch <= 0x323AF)
            return "CJK Unified Ideographs Extension H";
        if (ch >= 0x323B0 && ch <= 0xDFFFF)
            return "Undefined";
        // 0x40000-0xDFFFF 4- 13 未使用
        // 0xE0000-0xEFFFF 14 特別用途補充平面
        if (ch >= 0xE0000 && ch <= 0xE007F)
            return "label";
        if (ch >= 0xE0080 && ch <= 0xE00FF)
            return "Undefined";
        if (ch >= 0xE0100 && ch <= 0xE01EF)
            return "Variant selector supplement";
        if (ch >= 0xE01F0 && ch <= 0xEFFFF)
            return "Undefined";
        // 0xF0000-0xFFFFF 15 保留作為私人使用區（A區）
        if (ch >= 0xF0000 && ch <= 0xFFFFF)
            return "Supplementary Private Use Area-A";
        // 0x100000-0x10FFFF 16 保留作為私人使用區（B區）
        if (ch >= 0x100000 && ch <= 0x10FFFF)
            return "Supplementary Private Use Area-B";
        return "Undefined";
    }

    // 将UTF-8字符串拆分为Unicode字符（返回char32_t列表）
    std::vector<char32_t> splitToUnicode(const std::string &text)
    {
        std::vector<char32_t> result;
        size_t i = 0;
        while (i < text.size())
        {
            unsigned char c = text[i];
            char32_t code = 0;
            if (c < 0x80)
            {
                code = c;
                i += 1;
            }
            else if ((c & 0xE0) == 0xC0)
            {
                code = ((c & 0x1F) << 6) | (text[i + 1] & 0x3F);
                i += 2;
            }
            else if ((c & 0xF0) == 0xE0)
            {
                code = ((c & 0x0F) << 12) | ((text[i + 1] & 0x3F) << 6) | (text[i + 2] & 0x3F);
                i += 3;
            }
            else if ((c & 0xF8) == 0xF0)
            {
                code = ((c & 0x07) << 18) | ((text[i + 1] & 0x3F) << 12) | ((text[i + 2] & 0x3F) << 6) | (text[i + 3] & 0x3F);
                i += 4;
            }
            else
            {
                // 非法字符，跳过
                i += 1;
                continue;
            }
            result.push_back(code);
        }
        return result;
    }

    // 主处理函数：拆分文本，分类，查找位置，返回每个字符的详细信息
    std::vector<TextCharInfo> splitAndClassifyText(const std::string &text, css::uno::Reference<css::sheet::XSpreadsheet> sheet)
    {
        std::vector<TextCharInfo> result;
        std::vector<char32_t> chars = splitToUnicode(text);
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
                std::vector<char32_t> chars = splitToUnicode(content);
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
        std::vector<char32_t> chars = splitToUnicode(text);
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
