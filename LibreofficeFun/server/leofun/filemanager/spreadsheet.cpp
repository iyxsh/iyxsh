#include "spreadsheet.h"
#include "LibreOfficeService.h"
#include "ErrorHandler.h"
#include "filequeue.h"
#include "template_index_cache.h"
#include "unicodetable.h"
#include "utils.h"
#include "../cJSON/cJSON.h"
#include "../logger/logger.h"
#include "../config/json_config.h"
#include "DocumentManager.h"
#include <map>
#include <utility> // std::pair
#include <filesystem>

namespace filemanager
{
    // 加载电子表格文档
    std::string loadSpreadsheetDocument(const rtl::OUString &filePath)
    {
        try
        {
            std::string docId = filemanager::DocumentManager::getInstance().openDocument(
                convertOUStringToString(filePath), filemanager::DocumentType::SPREADSHEET);

            if (docId.empty())
            {
                logger_log_error("loadSpreadsheetDocument,Failed to load spreadsheet document: %s", convertOUStringToString(filePath).c_str());
                return "";
            }

            logger_log_info("Successfully loaded spreadsheet document: %s with docId: %s",
                            convertOUStringToString(filePath).c_str(), docId.c_str());
            return docId;
        }
        catch (const uno::Exception &e)
        {
            ErrorHandler::handleUnoException(e, "loadSpreadsheetDocument");
            return "";
        }
        catch (const std::exception &e)
        {
            ErrorHandler::handleStdException(e, "loadSpreadsheetDocument");
            return "";
        }
        catch (...)
        {
            ErrorHandler::handleUnknownException("loadSpreadsheetDocument");
            return "";
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
    cJSON *readCellToJson(const std::string &docId, const std::string &sheetName,
                          const rtl::OUString &cellAddress)
    {
        try
        {
            // 从DocumentManager获取文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("readCellToJson: Document not found with id: %s", docId.c_str());
                return nullptr;
            }

            // 转换为SpreadsheetDocument
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("readCellToJson: Document is not a spreadsheet: %s", docId.c_str());
                return nullptr;
            }

            // 获取工作表
            auto sheet = spreadsheetDoc->getSheet(sheetName);
            if (!sheet.is())
            {
                logger_log_error("readCellToJson: Sheet not found: %s", sheetName.c_str());
                return nullptr;
            }

            // 解析单元格地址
            sal_Int32 col = 0, row = 0;
            parseCellAddress(cellAddress, col, row);

            // 获取单元格
            auto cell = sheet->getCellByPosition(col, row);
            if (!cell.is())
            {
                logger_log_error("readCellToJson: Cannot get cell at %s",
                                 rtl::OUStringToOString(cellAddress, RTL_TEXTENCODING_UTF8).getStr());
                return nullptr;
            }

            // 创建返回的JSON对象
            cJSON *result = cJSON_CreateObject();

            // 获取单元格内容
            try
            {
                uno::Reference<com::sun::star::text::XText> xText(cell, uno::UNO_QUERY);
                if (xText.is())
                {
                    rtl::OUString cellString = xText->getString();
                    cJSON_AddStringToObject(result, "content",
                                            rtl::OUStringToOString(cellString, RTL_TEXTENCODING_UTF8).getStr());
                }
                else
                {
                    // 获取数值或公式
                    double cellValue = cell->getValue();
                    rtl::OUString cellFormula = cell->getFormula();

                    if (cellFormula.getLength() > 0)
                    {
                        cJSON_AddStringToObject(result, "content",
                                                rtl::OUStringToOString(cellFormula, RTL_TEXTENCODING_UTF8).getStr());
                        cJSON_AddStringToObject(result, "type", "formula");
                    }
                    else
                    {
                        cJSON_AddNumberToObject(result, "content", cellValue);
                        cJSON_AddStringToObject(result, "type", "number");
                    }
                }
                cJSON_AddStringToObject(result, "result", "success");
                return result;
            }
            catch (const uno::Exception &e)
            {
                cJSON_Delete(result);
                logger_log_error("readCellToJson exception: %s",
                                 rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
                return nullptr;
            }
        }
        catch (const std::exception &e)
        {
            ErrorHandler::handleStdException(e, "readCellToJson");
            return nullptr;
        }
        catch (...)
        {
            ErrorHandler::handleUnknownException("readCellToJson");
            return nullptr;
        }
    }

    /// @brief 直接保存当前文档（不指定路径，保存到原始位置）
    bool saveDocumentDirect(const std::string &docId)
    {
        try
        {
            // 从DocumentManager获取文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("saveDocumentDirect: Document not found with id: %s", docId.c_str());
                return false;
            }

            // 调用文档对象的save方法
            return doc->save();
        }
        catch (const uno::Exception &e)
        {
            ErrorHandler::handleUnoException(e, "saveDocumentDirect");
            return false;
        }
        catch (const std::exception &e)
        {
            ErrorHandler::handleStdException(e, "saveDocumentDirect");
            return false;
        }
        catch (...)
        {
            ErrorHandler::handleUnknownException("saveDocumentDirect");
            return false;
        }
    }

    /// @brief 保存文档到指定路径
    bool saveDocument(const std::string &docId, const rtl::OUString &filePath)
    {
        try
        {
            // 从DocumentManager获取文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("saveDocument: Document not found with id: %s", docId.c_str());
                return false;
            }

            // 调用文档对象的saveAs方法
            return doc->saveAs(convertOUStringToString(filePath));
        }
        catch (const uno::Exception &e)
        {
            ErrorHandler::handleUnoException(e, "saveDocument");
            return false;
        }
        catch (const std::exception &e)
        {
            ErrorHandler::handleStdException(e, "saveDocument");
            return false;
        }
        catch (...)
        {
            ErrorHandler::handleUnknownException("saveDocument");
            return false;
        }
    }

    /// @brief 关闭文档
    bool closeDocument(const std::string &docId)
    {
        try
        {
            // 调用DocumentManager的closeDocument方法
            return filemanager::DocumentManager::getInstance().closeDocument(docId);
        }
        catch (const uno::Exception &e)
        {
            ErrorHandler::handleUnoException(e, "closeDocument");
            return false;
        }
        catch (const std::exception &e)
        {
            ErrorHandler::handleStdException(e, "closeDocument");
            return false;
        }
        catch (...)
        {
            ErrorHandler::handleUnknownException("closeDocument");
            return false;
        }
    }

    /// @brief 创建新电子表格文件
    cJSON *createNewSpreadsheetFile(const rtl::OUString &filePath,
                                    const rtl::OUString &sheetName)
    {
        try
        {
            // 检查文件是否已存在
            std::filesystem::path filePathStr(convertOUStringToString(filePath));
            if (std::filesystem::exists(filePathStr))
            {
                logger_log_error("createNewSpreadsheetFile: File already exists: %s",
                                 convertOUStringToString(filePath).c_str());
                return cJSON_CreateString("File already exists");
            }

            // 使用DocumentManager创建新文档
            std::string docId = filemanager::DocumentManager::getInstance().createDocument(
                convertOUStringToString(filePath), filemanager::DocumentType::SPREADSHEET);

            if (docId.empty())
            {
                logger_log_error("createNewSpreadsheetFile: Failed to create spreadsheet document");
                return cJSON_CreateString("Failed to create document");
            }

            // 获取创建的文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("createNewSpreadsheetFile: Cannot get created document");
                return cJSON_CreateString("Failed to get document");
            }

            // 转换为SpreadsheetDocument
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("createNewSpreadsheetFile: Document is not a spreadsheet");
                filemanager::DocumentManager::getInstance().closeDocument(docId);
                return cJSON_CreateString("Document is not a spreadsheet");
            }

            // 如果指定了工作表名称，检查并创建
            if (sheetName.getLength() > 0)
            {
                std::string sheetNameStr = rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr();

                // 获取工作表名称列表
                auto sheetNames = spreadsheetDoc->getSheetNames();

                // 检查工作表是否已存在
                bool sheetExists = false;
                for (const auto &name : sheetNames)
                {
                    if (name == sheetNameStr)
                    {
                        sheetExists = true;
                        break;
                    }
                }

                // 如果不存在，创建新工作表
                if (!sheetExists)
                {
                    if (!spreadsheetDoc->createSheet(sheetNameStr))
                    {
                        logger_log_error("createNewSpreadsheetFile: Failed to create sheet: %s",
                                         sheetNameStr.c_str());
                        // 仍然返回成功，因为文档创建成功了
                    }
                }
            }

            // 保存文档
            if (!spreadsheetDoc->save())
            {
                logger_log_error("createNewSpreadsheetFile: Failed to save document");
                // 仍然返回成功，因为文档创建成功了
            }

            // 创建返回的JSON对象，包含文档ID
            cJSON *result = cJSON_CreateObject();
            cJSON_AddStringToObject(result, "result", "success");
            cJSON_AddStringToObject(result, "docId", docId.c_str());
            return result;
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("createNewSpreadsheetFile UNO error: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return cJSON_CreateString(rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            logger_log_error("createNewSpreadsheetFile std exception: %s", e.what());
            return cJSON_CreateString(e.what());
        }
        catch (...)
        {
            logger_log_error("createNewSpreadsheetFile unknown exception occurred");
            return cJSON_CreateString("Unknown error occurred");
        }
    }

    /// @brief 更新电子表格内容
    cJSON *updateSpreadsheetContent(const std::string &docId,
                                    const rtl::OUString &sheetName,
                                    const rtl::OUString &cellAddress,
                                    const rtl::OUString &newValue,
                                    const rtl::OUString &cellType)
    {
        try
        {
            // 通过DocumentManager获取文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("updateSpreadsheetContent: Document not found for docId: %s", docId.c_str());
                return cJSON_CreateString("Document not found");
            }

            // 转换为SpreadsheetDocument
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("updateSpreadsheetContent: Document is not a spreadsheet for docId: %s", docId.c_str());
                return cJSON_CreateString("Document is not a spreadsheet");
            }

            // 获取工作表
            std::string sheetNameStr = rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr();
            auto sheet = spreadsheetDoc->getSheet(sheetNameStr);
            if (!sheet.is())
            {
                logger_log_error("updateSpreadsheetContent: Cannot get sheet: %s", sheetNameStr.c_str());
                return cJSON_CreateString("Cannot get sheet");
            }

            // 解析单元格地址
            sal_Int32 col = 0, row = 0;
            parseCellAddress(cellAddress, col, row);

            // 获取单元格
            uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
            if (!cell.is())
            {
                logger_log_error("updateSpreadsheetContent: Cannot get cell at %d,%d", col, row);
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
                std::string tmp = findStringInDocIdSpreadsheet(docId, sheetName,newValue);
                logger_log_info("updateSpreadsheetContent: %s", tmp.c_str());
                cell->setFormula(convertStringToOUString(tmp.c_str()));
            }

            // 保存文档
            if (!spreadsheetDoc->save())
            {
                logger_log_error("updateSpreadsheetContent: Failed to save document");
                // 仍然返回成功，因为单元格更新成功了
            }

            return cJSON_CreateString("success");
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("updateSpreadsheetContent UNO exception: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return cJSON_CreateString(rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            logger_log_error("updateSpreadsheetContent std exception: %s", e.what());
            return cJSON_CreateString(e.what());
        }
        catch (...)
        {
            logger_log_error("updateSpreadsheetContent unknown exception occurred");
            return cJSON_CreateString("Unknown error occurred");
        }
    }

    /// @brief 获取工作表 - 旧版本（保留兼容性）
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

    /// @brief 获取工作表 - 新版本（使用DocumentManager）
    com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet> getSheet(
        const std::string &docId,
        const rtl::OUString &sheetName)
    {
        try
        {
            // 通过DocumentManager获取文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("getSheet: Document not found for docId: %s", docId.c_str());
                return nullptr;
            }

            // 转换为SpreadsheetDocument
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("getSheet: Document is not a spreadsheet for docId: %s", docId.c_str());
                return nullptr;
            }

            // 获取工作表
            std::string sheetNameStr = rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr();
            auto sheet = spreadsheetDoc->getSheet(sheetNameStr);
            if (!sheet.is())
            {
                logger_log_error("getSheet: Sheet not found: %s", sheetNameStr.c_str());
            }

            return sheet;
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("getSheet UNO error: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return nullptr;
        }
        catch (const std::exception &e)
        {
            logger_log_error("getSheet std exception: %s", e.what());
            return nullptr;
        }
        catch (...)
        {
            logger_log_error("getSheet unknown exception occurred");
            return nullptr;
        }
    }

    // 批量更新函数
    cJSON *batchUpdateSpreadsheetContent(const std::string &docId,
                                         const rtl::OUString &sheetName,
                                         const cJSON *updatecells)
    {
        try
        {
            // 通过DocumentManager获取文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("batchUpdateSpreadsheetContent: Document not found for docId: %s", docId.c_str());
                return cJSON_CreateString("Document not found");
            }

            // 转换为SpreadsheetDocument
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("batchUpdateSpreadsheetContent: Document is not a spreadsheet for docId: %s", docId.c_str());
                return cJSON_CreateString("Document is not a spreadsheet");
            }

            // 获取工作表
            std::string sheetNameStr = rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr();
            auto sheet = spreadsheetDoc->getSheet(sheetNameStr);
            if (!sheet.is())
            {
                logger_log_error("batchUpdateSpreadsheetContent: Cannot get sheet: %s", sheetNameStr.c_str());
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
                                filemanager::DocumentManager::getInstance().closeDocument(docId);
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
                                filemanager::DocumentManager::getInstance().closeDocument(docId);
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
            if (!spreadsheetDoc->save())
            {
                logger_log_error("batchUpdateSpreadsheetContent: Failed to save document");
                // 仍然返回成功，因为批量更新已经完成
            }

            cJSON *jobj = cJSON_CreateObject();
            cJSON_AddStringToObject(jobj, "result", "success");
            return jobj;
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "batchUpdateSpreadsheetContent UNO exception: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            return cJSON_CreateString(rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            std::cerr << "batchUpdateSpreadsheetContent std exception: " << e.what() << std::endl;
            return cJSON_CreateString(e.what());
        }
        catch (...)
        {
            std::cerr << "batchUpdateSpreadsheetContent unknown exception occurred" << std::endl;
            return cJSON_CreateString("Unknown error occurred");
        }
    }

    // 内部辅助函数
    std::string findValueInSheet(const std::string &docId, const rtl::OUString &sheetName, const rtl::OUString &searchValue)
    {
        // 实现查找值的函数
        return findStringInDocIdSpreadsheet(docId, sheetName, searchValue);
    }

    cJSON *readSheetData(const std::string &docId, const rtl::OUString &sheetName)
    {
        // 减少日志输出以提高性能
        try
        {
            // 通过DocumentManager获取文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("readSheetData: Document not found for docId: %s", docId.c_str());
                return cJSON_CreateString("Document not found");
            }

            // 转换为SpreadsheetDocument
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("readSheetData: Document is not a spreadsheet for docId: %s", docId.c_str());
                return cJSON_CreateString("Document is not a spreadsheet");
            }

            // 获取工作表
            std::string sheetNameStr = rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr();
            auto sheet = spreadsheetDoc->getSheet(sheetNameStr);
            if (!sheet.is())
            {
                logger_log_error("readSheetData: Cannot get sheet: %s", sheetNameStr.c_str());
                return cJSON_CreateString("Cannot get sheet");
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

            // 不需要关闭文档，DocumentManager会管理文档生命周期
            return contentMap;
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("readSheetData UNO error: %s", rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return nullptr;
        }
        catch (const std::exception &e)
        {
            logger_log_error("readSheetData std exception: %s", e.what());
            return nullptr;
        }
        catch (...)
        {
            logger_log_error("readSheetData unknown exception occurred");
            return nullptr;
        }
    }

    // 获取工作表总行数（持续读取行直到遇到第一个空白行）
    int getTotalRecordCount(const std::string &docId, const rtl::OUString &sheetName)
    {
        try
        {
            // 通过DocumentManager获取文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("getTotalRecordCount: Document not found for docId: %s", docId.c_str());
                return -1;
            }

            // 转换为SpreadsheetDocument
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("getTotalRecordCount: Document is not a spreadsheet for docId: %s", docId.c_str());
                return -1;
            }

            // 获取工作表
            std::string sheetNameStr = rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr();
            auto sheet = spreadsheetDoc->getSheet(sheetNameStr);
            if (!sheet.is())
            {
                logger_log_error("getTotalRecordCount: Cannot get sheet: %s", sheetNameStr.c_str());
                return -1;
            }

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

            // 不需要关闭文档，DocumentManager会管理文档生命周期
            return validRowCount;
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("getTotalRecordCount UNO error: %s", rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return -1;
        }
        catch (const std::exception &e)
        {
            logger_log_error("getTotalRecordCount std exception: %s", e.what());
            return -1;
        }
        catch (...)
        {
            logger_log_error("getTotalRecordCount unknown exception occurred");
            return -1;
        }
    }

    // 分页读取工作表内容（简单实现：返回 startIndex 到 endIndex 行的内容）
    cJSON *readSheetDataRange(const std::string &docId, const rtl::OUString &sheetName, int startIndex, int endIndex)
    {
        cJSON *result = cJSON_CreateArray();
        try
        {
            // 通过DocumentManager获取文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("readSheetDataRange: Document not found for docId: %s", docId.c_str());
                return cJSON_CreateString("Document not found");
            }

            // 转换为SpreadsheetDocument
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("readSheetDataRange: Document is not a spreadsheet for docId: %s", docId.c_str());
                return cJSON_CreateString("Document is not a spreadsheet");
            }

            // 获取工作表
            std::string sheetNameStr = rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr();
            auto sheet = spreadsheetDoc->getSheet(sheetNameStr);
            if (!sheet.is())
            {
                logger_log_error("readSheetDataRange: Cannot get sheet: %s", sheetNameStr.c_str());
                return cJSON_CreateString("Cannot get sheet");
            }
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
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("readSheetDataRange UNO error: %s", rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            logger_log_error("readSheetDataRange std exception: %s", e.what());
        }
        catch (...)
        {
            logger_log_error("readSheetDataRange unknown exception occurred");
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
    bool writeCharacterInfosToSheet(const std::string &docId,
                                    const rtl::OUString &sheetName,
                                    const std::vector<TextCharInfo> &infos)
    {
        try
        {
            // 通过DocumentManager获取文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("writeCharacterInfosToSheet: Document not found for docId: %s", docId.c_str());
                return false;
            }

            // 转换为SpreadsheetDocument
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("writeCharacterInfosToSheet: Document is not a spreadsheet for docId: %s", docId.c_str());
                return false;
            }

            // 获取工作表
            std::string sheetNameStr = rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr();
            auto sheet = spreadsheetDoc->getSheet(sheetNameStr);
            if (!sheet.is())
            {
                logger_log_error("writeCharacterInfosToSheet: Cannot get sheet: %s", sheetNameStr.c_str());
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
                catch (const uno::Exception &e)
                {
                    logger_log_error("writeCharacterInfosToSheet: UNO exception writing %s to %s: %s",
                                     info.character.c_str(),
                                     info.pos.c_str(),
                                     rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
                }
                catch (const std::exception &e)
                {
                    logger_log_error("writeCharacterInfosToSheet: std exception writing %s to %s: %s",
                                     info.character.c_str(),
                                     info.pos.c_str(),
                                     e.what());
                }
                catch (...)
                {
                    logger_log_error("writeCharacterInfosToSheet: unknown exception writing %s to %s",
                                     info.character.c_str(),
                                     info.pos.c_str());
                }
            }

            // 使用DocumentManager保存文档
            if (!spreadsheetDoc->save())
            {
                logger_log_error("writeCharacterInfosToSheet: Failed to save document for docId: %s", docId.c_str());
                return false;
            }

            return true;
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("writeCharacterInfosToSheet: UNO exception: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            logger_log_error("writeCharacterInfosToSheet: std exception: %s", e.what());
        }
        catch (...)
        {
            logger_log_error("writeCharacterInfosToSheet: unknown exception occurred");
        }
        return false;
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

    /**
     * @brief 通过UNO API删除文件，确保连接状态与文件系统一致
     * @param filePath 文件路径
     * @return bool 删除是否成功
     */
    bool DeleteFileWithUNO(const rtl::OUString &filePath)
    {
        try
        {
            // 获取组件上下文
            uno::Reference<uno::XComponentContext> xContext = LibreOfficeService::getContext();
            if (!xContext.is())
            {
                logger_log_error("Failed to get UNO context for file deletion");
                return false;
            }

            // 获取服务管理器
            uno::Reference<lang::XMultiComponentFactory> xServiceManager = xContext->getServiceManager();
            if (!xServiceManager.is())
            {
                logger_log_error("Failed to get UNO service manager for file deletion");
                return false;
            }

            // 获取 XSimpleFileAccess
            uno::Reference<ucb::XSimpleFileAccess> fileAccess(xServiceManager->createInstanceWithContext(
                                                                  "com.sun.star.ucb.SimpleFileAccess", xContext),
                                                              uno::UNO_QUERY);
            if (!fileAccess.is())
            {
                logger_log_error("Failed to get SimpleFileAccess service for file deletion");
                return false;
            }

            // 检查文件是否存在
            if (fileAccess->exists(filePath))
            {
                // 删除文件
                fileAccess->kill(filePath);
                logger_log_info("File deleted successfully via UNO API: %s", convertOUStringToString(filePath).c_str());
                return true;
            }
            else
            {
                logger_log_error("File does not exist, no need to delete: %s", convertOUStringToString(filePath).c_str());
                return true; // 文件不存在视为删除成功，避免后续不必要的错误处理
            }
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO Exception occurred while deleting file: %s, Error: %s",
                             convertOUStringToString(filePath).c_str(),
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            // 抛出异常，让上层函数能够捕获并处理
            throw;
        }
        catch (const std::exception &e)
        {
            logger_log_error("Standard exception occurred while deleting file: %s, Error: %s",
                             convertOUStringToString(filePath).c_str(),
                             std::string(e.what()).c_str());
            throw;
        }
        catch (...)
        {
            logger_log_error("Unknown exception occurred while deleting file: %s",
                             convertOUStringToString(filePath).c_str());
            throw;
        }
        return false;
    }

    /**
     * @brief 安全删除文件，调用UNO API删除并处理异常
     * @param filePath 文件路径
     * @return bool 删除是否成功
     */
    bool SafeDeleteFile(const rtl::OUString &filePath)
    {
        try
        {
            // 直接调用UNO API删除文件，它内部已包含文件存在性检查
            return DeleteFileWithUNO(filePath);
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO Exception occurred while safely deleting file: %s, Error: %s",
                             convertOUStringToString(filePath).c_str(),
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            logger_log_error("Standard exception occurred while safely deleting file: %s, Error: %s",
                             convertOUStringToString(filePath).c_str(),
                             std::string(e.what()).c_str());
        }
        catch (...)
        {
            logger_log_error("Unknown exception occurred while safely deleting file: %s",
                             convertOUStringToString(filePath).c_str());
        }

        // 如果发生异常，返回删除失败
        return false;
    }

    // 清空指定文档和工作表的内容
    void clearSheet(const std::string &docId, const rtl::OUString &sheetName)
    {
        try
        {
            // 通过DocumentManager获取文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("clearSheet: Document not found for docId: %s", docId.c_str());
                return;
            }

            // 转换为SpreadsheetDocument
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("clearSheet: Document is not a spreadsheet for docId: %s", docId.c_str());
                return;
            }

            // 获取工作表
            std::string sheetNameStr = rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr();
            auto sheet = spreadsheetDoc->getSheet(sheetNameStr);
            if (!sheet.is())
            {
                logger_log_error("clearSheet: Cannot get sheet: %s", sheetNameStr.c_str());
                return;
            }

            // 调用内部函数清空工作表
            clearSheet(sheet);

            // 使用DocumentManager保存文档
            if (!spreadsheetDoc->save())
            {
                logger_log_error("clearSheet: Failed to save document for docId: %s", docId.c_str());
            }
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("clearSheet: UNO exception: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            logger_log_error("clearSheet: std exception: %s", e.what());
        }
        catch (...)
        {
            logger_log_error("clearSheet: unknown exception occurred");
        }
    }

    // 在指定文档和工作表中查找字符串
    std::string findStringInDocIdSpreadsheet(const std::string &docId, const rtl::OUString &sheetName, const rtl::OUString &targetString)
    {
        try
        {
            // 通过DocumentManager获取文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("findStringInDocIdSpreadsheet: Document not found for docId: %s", docId.c_str());
                return "";
            }

            // 转换为SpreadsheetDocument
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("findStringInDocIdSpreadsheet: Document is not a spreadsheet for docId: %s", docId.c_str());
                return "";
            }

            // 获取工作表
            std::string sheetNameStr = rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr();
            auto sheet = spreadsheetDoc->getSheet(sheetNameStr);
            if (!sheet.is())
            {
                logger_log_error("findStringInDocIdSpreadsheet: Cannot get sheet: %s", sheetNameStr.c_str());
                return "";
            }

            // 调用内部函数查找字符串
            return findStringInSpreadsheet(targetString, sheet);
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("findStringInDocIdSpreadsheet: UNO exception: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            logger_log_error("findStringInDocIdSpreadsheet: std exception: %s", e.what());
        }
        catch (...)
        {
            logger_log_error("findStringInDocIdSpreadsheet: unknown exception occurred");
        }
        return "";
    }

    // 读取工作表并按语言分组
    std::vector<LanguageGroup> readSheetAndGroupByLanguage(const std::string &docId, const rtl::OUString &sheetName)
    {
        std::vector<LanguageGroup> result;
        try
        {
            // 通过DocumentManager获取文档
            auto doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("readSheetAndGroupByLanguage: Document not found for docId: %s", docId.c_str());
                return result;
            }

            // 转换为SpreadsheetDocument
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("readSheetAndGroupByLanguage: Document is not a spreadsheet for docId: %s", docId.c_str());
                return result;
            }

            // 获取工作表
            std::string sheetNameStr = rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr();
            auto sheet = spreadsheetDoc->getSheet(sheetNameStr);
            if (!sheet.is())
            {
                logger_log_error("readSheetAndGroupByLanguage: Cannot get sheet: %s", sheetNameStr.c_str());
                return result;
            }

            // 调用内部函数读取工作表并按语言分组
            result = readSheetAndGroupByLanguage(sheet);
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("readSheetAndGroupByLanguage: UNO exception: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            logger_log_error("readSheetAndGroupByLanguage: std exception: %s", e.what());
        }
        catch (...)
        {
            logger_log_error("readSheetAndGroupByLanguage: unknown exception occurred");
        }
        return result;
    }
}
