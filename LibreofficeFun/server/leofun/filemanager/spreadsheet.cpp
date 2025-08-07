#include "spreadsheet.h"
#include "lofficeconn.h"
#include "filequeue.h"
#include "utils.h"
#include "../cJSON/cJSON.h"
#include "../logger/logger.h"
#include "../config/json_config.h"
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
#include <thread>
#include <chrono>
#include <algorithm>
#include <mutex>

// 定义缓存大小限制
#define MAX_TEMPLATE_CACHE_SIZE 100
#define MAX_SHEET_DATA_CACHE_SIZE 100

namespace filemanager
{
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

    /// @brief 保存文档到指定路径
    void saveDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &docIface, 
                      const rtl::OUString &filePath)
    {
        try
        {
            com::sun::star::uno::Reference<com::sun::star::frame::XStorable> xStorable(docIface, com::sun::star::uno::UNO_QUERY);
            if (!xStorable.is())
            {
                std::cerr << "saveDocument error: Invalid storable interface" << std::endl;
                return;
            }

            // 规范路径，自动创建父目录
            std::string nativePath = std::string(rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
            std::cerr << "saveDocument: Saving to native path: " << nativePath << std::endl;

            // 标准化路径分隔符
            std::replace(nativePath.begin(), nativePath.end(), '\\', '/');

            // 处理相对路径
            std::string absolutePath = convertToAbsolutePath(nativePath);
            std::cerr << "saveDocument: Absolute path: " << absolutePath << std::endl;

            // 创建父目录
            size_t lastSlash = absolutePath.find_last_of('/');
            if (lastSlash != std::string::npos)
            {
                std::string dir = absolutePath.substr(0, lastSlash);
                std::cerr << "saveDocument: Creating directories for: " << dir << std::endl;
                make_dirs(dir);
            }

#if __cplusplus >= 201703L
            std::string urlPath = std::filesystem::path(absolutePath).generic_string();
#else
            std::string urlPath = absolutePath;
#endif
            std::cerr << "saveDocument: URL path: " << urlPath << std::endl;

            rtl::OUString url = rtl::OUString::createFromAscii("file:///") + rtl::OUString::createFromAscii(urlPath.c_str());
            std::cerr << "saveDocument: Full URL: " << rtl::OUStringToOString(url, RTL_TEXTENCODING_UTF8).getStr() << std::endl;

            com::sun::star::uno::Sequence<com::sun::star::beans::PropertyValue> props(1);
            props[0].Name = rtl::OUString::createFromAscii("Overwrite");
            props[0].Value <<= true;

            xStorable->storeAsURL(url, props);
            std::cerr << "saveDocument: Successfully saved to: " << rtl::OUStringToOString(url, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
        }
        catch (const com::sun::star::uno::Exception &e)
        {
            std::cerr << "saveDocument UNO exception: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "saveDocument std exception: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "saveDocument unknown exception occurred" << std::endl;
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
                std::cerr << "closeDocument: Document closed successfully" << std::endl;
            }
            else
            {
                std::cerr << "closeDocument: Invalid component interface" << std::endl;
            }
        }
        catch (const com::sun::star::uno::Exception &e)
        {
            std::cerr << "closeDocument UNO exception: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "closeDocument std exception: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "closeDocument unknown exception occurred" << std::endl;
        }
    }

    /// @brief 读取整个电子表格文件为JSON
    cJSON *readSpreadsheetFile(const rtl::OUString &filePath)
    {
        uno::Reference<lang::XComponent> xComp; // 在函数作用域内声明，确保在任何情况下都能正确关闭
        try
        {
            std::string filePathStr = std::string(rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
            std::cerr << "readSpreadsheetFile: Attempting to read file: " << filePathStr << std::endl;

            // 处理相对路径，转换为绝对路径
            std::string absolutePath = convertToAbsolutePath(filePathStr);
            std::cerr << "readSpreadsheetFile: Absolute path: " << absolutePath << std::endl;

            // 获取ComponentLoader
            uno::Reference<frame::XComponentLoader> xLoader = LibreOfficeConnectionManager::getComponentLoader();
            if (!xLoader.is())
            {
                return nullptr;
            }

            rtl::OUString url = rtl::OUString::createFromAscii("file:///") + rtl::OUString::createFromAscii(absolutePath.c_str());
            std::string urlStr = std::string(rtl::OUStringToOString(url, RTL_TEXTENCODING_UTF8).getStr());
            std::cerr << "readSpreadsheetFile: Loading from URL: " << urlStr << std::endl;

            uno::Sequence<beans::PropertyValue> args(0);
            xComp = uno::Reference<lang::XComponent>(xLoader->loadComponentFromURL(url, rtl::OUString::createFromAscii("_blank"), 0, args), uno::UNO_QUERY);
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

            closeDocument(xComp);
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
    }

    /// @brief 创建新电子表格文件
    cJSON *createNewSpreadsheetFile(const rtl::OUString &filePath,
                                    const rtl::OUString &sheetName,
                                    const cJSON *contentData)
    {
        uno::Reference<lang::XComponent> xComp; // 在函数作用域内声明，确保在任何情况下都能正确关闭
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
                rtl::OUString::createFromAscii("private:factory/scalc"), rtl::OUString::createFromAscii("_blank"), 0, args);
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

            // 写入内容
            if (sheet.is() && contentData && cJSON_IsObject(contentData))
            {
                printf("Writing content to sheet...\n");

                // 获取contentData中的所有键值对
                int contentSize = cJSON_GetArraySize(const_cast<cJSON *>(contentData));
                printf("Content has %d items\n", contentSize);

                // 遍历对象中的每个键值对
                cJSON *item = contentData->child;
                while (item)
                {
                    // 键是字符，值是位置（如"A1"）
                    const char *character = item->string;     // 字符作为键
                    const char *position = item->valuestring; // 位置作为值

                    if (character && position)
                    {
                        printf("Writing character '%s' to position %s\n", character, position);

                        // 解析位置字符串，如"A1" -> 列=0 (A), 行=0 (1-based, 实际是0)
                        sal_Int32 col = 0, row = 0;
                        parseCellAddress(rtl::OUString::createFromAscii(position), col, row);

                        // 将字符写入指定单元格
                        rtl::OUString charOUString = convertStringToOUString(character);
                        uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                        if (cell.is())
                        {
                            cell->setFormula(charOUString);
                        }
                        else
                        {
                            std::cerr << "createNewSpreadsheetFile: Cannot get cell at " << col << "," << row << std::endl;
                        }
                    }

                    item = item->next;
                }
                printf("Finished writing content to sheet\n");
            }
            else if (sheet.is() && contentData && cJSON_IsArray(contentData))
            {
                // 保持原有的数组处理逻辑，以确保向后兼容
                printf("Writing content to sheet (array format)...\n");
                // 注意：cJSON_GetArraySize需要非const指针，所以我们需要一个非const副本
                cJSON *nonConstContent = const_cast<cJSON *>(contentData);
                int contentSize = cJSON_GetArraySize(nonConstContent);
                printf("Content has %d rows\n", contentSize);

                int row = 0;
                cJSON *rowItem = nullptr;
                cJSON_ArrayForEach(rowItem, nonConstContent)
                {
                    if (row >= 100)
                        break; // 限制行数

                    int col = 0;
                    cJSON *cellItem = nullptr;
                    cJSON_ArrayForEach(cellItem, rowItem)
                    {
                        if (col >= 20)
                            break; // 限制列数

                        // 获取单元格引用
                        uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                        if (cell.is() && cellItem)
                        {
                            if (cJSON_IsString(cellItem))
                            {
                                // 字符串类型
                                rtl::OUString cellValue = rtl::OUString::createFromAscii(cellItem->valuestring);
                                cell->setFormula(cellValue);
                            }
                            else if (cJSON_IsNumber(cellItem))
                            {
                                // 数值类型
                                cell->setValue(cellItem->valuedouble);
                            }
                            else if (cJSON_IsObject(cellItem))
                            {
                                // 对象类型，可能包含类型信息
                                cJSON *type = cJSON_GetObjectItem(cellItem, "type");
                                cJSON *value = cJSON_GetObjectItem(cellItem, "value");
                                
                                if (type && value && cJSON_IsString(type))
                                {
                                    const char *typeStr = type->valuestring;
                                    if (strcmp(typeStr, "string") == 0 && cJSON_IsString(value))
                                    {
                                        rtl::OUString cellValue = rtl::OUString::createFromAscii(value->valuestring);
                                        cell->setFormula(cellValue);
                                    }
                                    else if (strcmp(typeStr, "number") == 0 && cJSON_IsNumber(value))
                                    {
                                        cell->setValue(value->valuedouble);
                                    }
                                    else if (strcmp(typeStr, "formula") == 0 && cJSON_IsString(value))
                                    {
                                        rtl::OUString formula = rtl::OUString::createFromAscii(value->valuestring);
                                        cell->setFormula(formula);
                                    }
                                }
                            }
                        }
                        ++col;
                    }
                    ++row;
                }
                printf("Finished writing content to sheet\n");
            }
            else if (!sheet.is())
            {
                printf("Sheet is not valid\n");
            }
            else if (!contentData)
            {
                printf("No content data to write\n");
            }
            else
            {
                printf("Content data format is not supported\n");
                // 注意：cJSON_Print需要非const指针，所以我们需要一个非const副本
                cJSON *nonConstContent = const_cast<cJSON *>(contentData);
                char *contentStr = cJSON_Print(nonConstContent);
                printf("Content data: %s\n", contentStr);
                free(contentStr); // 修复：释放 cJSON_Print 返回的内存
            }

            // 保存文档前输出文件路径信息
            std::string filePathStr = rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr();
            std::cerr << "createNewSpreadsheetFile: Saving document to: " << filePathStr << std::endl;
            saveDocument(xDoc, filePath);
            closeDocument(xComp);
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

            // 构造文件URL
            rtl::OUString fileUrl = rtl::OUString::createFromAscii("file:///") + filePath.replaceAll("\\", "/");
            
            // 加载文档
            uno::Sequence<beans::PropertyValue> args(0);
            xComp = uno::Reference<lang::XComponent>(xLoader->loadComponentFromURL(fileUrl, rtl::OUString::createFromAscii("_blank"), 0, args), uno::UNO_QUERY);
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

            // 根据单元格类型设置值
            if (cellType.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("formula")))
            {
                cell->setFormula(newValue);
            }
            else if (cellType.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("string")))
            {
                uno::Reference<beans::XPropertySet> xCellProps(cell, uno::UNO_QUERY);
                if (xCellProps.is())
                {
                    xCellProps->setPropertyValue(rtl::OUString::createFromAscii("CharColor"), uno::makeAny(static_cast<sal_Int32>(0xFF0000)));
                }
                cell->setFormula(newValue);
            }
            else
            {
                // 默认作为数值处理
                double value = newValue.toDouble();
                cell->setValue(value);
            }

            // 保存文档
            saveDocument(xDoc, filePath);
            closeDocument(xComp);
            
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
    cJSON *batchUpdateSpreadsheetContent(const rtl::OUString &filePath,
                                         const rtl::OUString &sheetName,
                                         const cJSON *updateData)
    {
        uno::Reference<lang::XComponent> xComp;
        try
        {
            // 获取ComponentLoader
            uno::Reference<frame::XComponentLoader> xLoader = LibreOfficeConnectionManager::getComponentLoader();
            if (!xLoader.is())
            {
                std::cerr << "batchUpdateSpreadsheetContent: Failed to get component loader" << std::endl;
                return cJSON_CreateString("Failed to get component loader");
            }

            // 构造文件URL
            rtl::OUString fileUrl = rtl::OUString::createFromAscii("file:///") + filePath.replaceAll("\\", "/");
            
            // 加载文档
            uno::Sequence<beans::PropertyValue> args(0);
            xComp = uno::Reference<lang::XComponent>(xLoader->loadComponentFromURL(fileUrl, rtl::OUString::createFromAscii("_blank"), 0, args), uno::UNO_QUERY);
            if (!xComp.is())
            {
                std::cerr << "batchUpdateSpreadsheetContent: Failed to load document" << std::endl;
                return cJSON_CreateString("Failed to load document");
            }

            // 获取文档和工作表
            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is())
            {
                std::cerr << "batchUpdateSpreadsheetContent: Loaded component is not a spreadsheet document" << std::endl;
                closeDocument(xComp);
                return cJSON_CreateString("Loaded component is not a spreadsheet document");
            }

            uno::Reference<sheet::XSpreadsheet> sheet = getSheet(xDoc, sheetName);
            if (!sheet.is())
            {
                std::cerr << "batchUpdateSpreadsheetContent: Cannot get sheet" << std::endl;
                closeDocument(xComp);
                return cJSON_CreateString("Cannot get sheet");
            }

            // 处理批量更新数据
            if (updateData && cJSON_IsArray(const_cast<cJSON*>(updateData)))
            {
                cJSON *item = nullptr;
                cJSON_ArrayForEach(item, const_cast<cJSON*>(updateData))
                {
                    if (cJSON_IsObject(item))
                    {
                        cJSON *celladdrItem = cJSON_GetObjectItem(item, "celladdr");
                        cJSON *valueItem = cJSON_GetObjectItem(item, "value");
                        cJSON *typeItem = cJSON_GetObjectItem(item, "type");

                        if (celladdrItem && valueItem && cJSON_IsString(celladdrItem) && cJSON_IsString(valueItem))
                        {
                            rtl::OUString cellAddr = convertStringToOUString(celladdrItem->valuestring);
                            rtl::OUString value = convertStringToOUString(valueItem->valuestring);
                            rtl::OUString type = typeItem && cJSON_IsString(typeItem) ? 
                                                convertStringToOUString(typeItem->valuestring) : 
                                                rtl::OUString::createFromAscii("string");

                            // 解析单元格地址
                            sal_Int32 col = 0, row = 0;
                            parseCellAddress(cellAddr, col, row);

                            // 获取单元格
                            uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                            if (cell.is())
                            {
                                // 根据单元格类型设置值
                                if (type.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("formula")))
                                {
                                    cell->setFormula(value);
                                }
                                else if (type.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("string")))
                                {
                                    uno::Reference<beans::XPropertySet> xCellProps(cell, uno::UNO_QUERY);
                                    if (xCellProps.is())
                                    {
                                        xCellProps->setPropertyValue(rtl::OUString::createFromAscii("CharColor"), uno::makeAny(static_cast<sal_Int32>(0xFF0000)));
                                    }
                                    cell->setFormula(value);
                                }
                                else
                                {
                                    // 默认作为数值处理
                                    double numValue = value.toDouble();
                                    cell->setValue(numValue);
                                }
                            }
                            else
                            {
                                std::cerr << "batchUpdateSpreadsheetContent: Cannot get cell at " << col << "," << row << std::endl;
                            }
                        }
                    }
                }
            }

            // 保存文档
            saveDocument(xDoc, filePath);
            closeDocument(xComp);
            
            return cJSON_CreateString("success");
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
        // logger_log("filePath: %s", rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
        // logger_log("sheetName: %s", rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr());
        try
        {
            // 获取ComponentLoader
            uno::Reference<frame::XComponentLoader> xLoader = LibreOfficeConnectionManager::getComponentLoader();
            if (!xLoader.is())
            {
                std::cerr << "readSheetData: Failed to get component loader" << std::endl;
                return nullptr;
            }

            rtl::OUString url = rtl::OUString::createFromAscii("file:///") + filePath.replaceAll("\\", "/");
            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<lang::XComponent> xComp(xLoader->loadComponentFromURL(url, rtl::OUString::createFromAscii("_blank"), 0, args), uno::UNO_QUERY);
            if (!xComp.is()) {
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
            logger_log("maxRows: %d", maxRows);
            logger_log("maxCols: %d", maxCols);
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
                        // 尝试获取单元格
                        cell = sheet->getCellByPosition(col, row);
                    }
                    catch (const uno::Exception &)
                    {
                        // 如果获取单元格失败，认为此列结束
                        break;
                    }

                    if (!cell.is())
                    {
                        // 单元格无效，此列结束
                        break;
                    }

                    // 获取单元格的值和公式
                    double cellValue = cell->getValue();
                    rtl::OUString cellFormula = cell->getFormula();

                    // 检查单元格是否为空
                    if (cellValue == 0.0 && cellFormula.getLength() == 0)
                    {
                        // 遇到空单元格，此列结束
                        break;
                    }

                    // 标记此列有数据
                    foundDataInColumn = true;

                    // 获取单元格内容
                    rtl::OUString cellContent;
                    if (cellFormula.getLength() > 0)
                    {
                        // 如果有公式，使用公式文本
                        cellContent = cellFormula;
                    }
                    else
                    {
                        // 否则使用数值或文本值
                        cellContent = rtl::OUString::number(cellValue);
                    }

                    // 将内容转换为字符串用于比较
                    std::string contentStr = rtl::OUStringToOString(cellContent, RTL_TEXTENCODING_UTF8).getStr();

                    // 检查内容是否已经添加过（避免重复）
                    if (cJSON_GetObjectItem(addedContents, contentStr.c_str()) != nullptr)
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
                    cJSON_AddStringToObject(contentMap, contentStr.c_str(), position.c_str());

                    // 标记内容已添加
                    cJSON_AddStringToObject(addedContents, contentStr.c_str(), "added");
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

            // 在sheetData中查找该字符的位置
            cJSON *positionItem = cJSON_GetObjectItem(sheetData, charStdStr.c_str());
            if (positionItem && cJSON_IsString(positionItem))
            {
                // 如果找到，添加位置到结果中
                rtl::OUString positionStr = rtl::OStringToOUString(
                    rtl::OString(positionItem->valuestring),
                    RTL_TEXTENCODING_UTF8);
                resultBuffer.append(positionStr);
            }
            else
            {
                // 如果未找到，添加原始字符
                resultBuffer.append(charStr);
            }

            // 如果不是最后一个字符，添加空格分隔符
            if (i < newValue.getLength() - 1)
            {
                resultBuffer.append(sal_Unicode(' '));
            }
        }

        // 返回组合的位置字符串
        return resultBuffer.makeStringAndClear();
    }
}