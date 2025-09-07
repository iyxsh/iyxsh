#include "filehandlers.h"
#include "spreadsheet.h"
#include "filequeue.h"
#include "LibreOfficeService.h"
#include "utils.h"
#include "../cJSON/cJSON.h"
#include "../logger/logger.h"
#include "../config/json_config.h"
#include <iostream>
#include <ctime>
#include <string>
#include <mutex>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheets.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/container/XNameContainer.hpp>

namespace filemanager
{
    int createfile(std::string filename)
    {
        rtl::OUString absfilePath;
        getAbsolutePath(convertStringToOUString(filename.c_str()), absfilePath);
        // 创建文件
        rtl::OUString filePath;
        rtl::OUString defaultFileName;
        rtl::OUString sheetName;
        getDefaultData(filePath, defaultFileName, sheetName);

        // 获取文档ID
        std::string docId = filemanager::DocumentManager::getInstance().getDocumentIdByPath(
            convertOUStringToString(absfilePath));
        if (docId.empty())
        {
            logger_log_error("Failed to create document: %s", convertOUStringToString(absfilePath).c_str());
            return -1;
        }

        // 从缓存索引获取数据
        std::shared_ptr<CharacterIndex> idxPtr = filemanager::TemplateIndexCacheManager::getInstance().getCharacterInfos(filePath + defaultFileName, sheetName);
        if (!idxPtr)
        {
            logger_log_error("Failed to get character infos from cache: idxPtr is null");
            filemanager::DocumentManager::getInstance().closeDocument(docId);
            return -1;
        }
        std::vector<TextCharInfo> infos = idxPtr->getAll(); // 获取所有数据

        // 写入数据到工作表
        if (!writeCharacterInfosToSheet(docId, sheetName, infos))
        {
            logger_log_error("Failed to write character infos to sheet");
            filemanager::DocumentManager::getInstance().closeDocument(docId);
            return -1;
        }

        // 保存并关闭文档
        if (!filemanager::DocumentManager::getInstance().closeDocument(docId))
        {
            logger_log_error("Failed to close document: %s", convertOUStringToString(absfilePath).c_str());
            return -1;
        }
        return 0;
    }

    int fileclose(cJSON *taskData)
    {
        cJSON *filenameItem = cJSON_GetObjectItem(taskData, "filename");
        if (!filenameItem || !cJSON_IsString(filenameItem))
        {
            logger_log_error("Invalid or missing filename in close file task data");
            return -1;
        }

        std::string filename = filenameItem->valuestring;
        FileInfo fileInfo = FileQueueManager::getInstance().getFileInfo(filename);

        if (!fileInfo.xDoc.is())
        {
            logger_log_info("File %s is not opened, no need to close", filename.c_str());
            return -1;
        }

        try
        {
            // 从DocumentManager获取文档ID并关闭文档
            std::string docId = filemanager::DocumentManager::getInstance().getDocumentIdByPath(filename);
            if (!docId.empty())
            {
                filemanager::DocumentManager::getInstance().closeDocument(docId);
            }

            // 删除文件信息和状态信息
            FileQueueManager::getInstance().removeFileInfo(filename);
            logger_log_info("Removed file info and status for: %s", filename.c_str());
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO exception while closing file %s: %s",
                             filename.c_str(),
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        catch (const std::exception &e)
        {
            logger_log_error("STD exception while closing file %s: %s", filename.c_str(), e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception while closing file %s", filename.c_str());
        }
        return 0;
    }

    int fileupdate(cJSON *root)
    {
        // 实现更新文件的函数
        logger_log_info("Updating file with data");

        if (!root)
        {
            logger_log_error("Error: Empty root data");
            return -1;
        }

        try
        {
            // 使用LibreOffice连接管理器
            if (!LibreOfficeService::initialize())
            {
                logger_log_error("Error: Failed to initialize LibreOffice connection");
                return -1;
            }

            // 支持批量：body为数组(单个文件多个工作表多个单元格,常用只更新一个单元格时只给一个对象即可)
            if (cJSON_IsObject(root))
            {
                cJSON *filenameItem = cJSON_GetObjectItem(root, "filename");
                cJSON *updatedataItem = cJSON_GetObjectItem(root, "updatedata");

                // 兼容大小写不敏感的字段名
                if (!filenameItem)
                    filenameItem = cJSON_GetObjectItem(root, "fileName");
                if (!updatedataItem)
                    updatedataItem = cJSON_GetObjectItem(root, "updateData");

                if (!filenameItem || !updatedataItem || !cJSON_IsString(filenameItem) || !cJSON_IsArray(updatedataItem))
                {
                    logger_log_error("Error: Missing filename or updatedata in batch data");
                    return -1;
                }
                cJSON *item = nullptr;
                bool hasError = false;
                cJSON_ArrayForEach(item, updatedataItem)
                {
                    if (cJSON_IsObject(item))
                    {
                        cJSON *sheetnameItem = cJSON_GetObjectItem(item, "sheetname");
                        cJSON *updatecellsItem = cJSON_GetObjectItem(item, "updatecells");
                        if (!sheetnameItem)
                            sheetnameItem = cJSON_GetObjectItem(item, "sheetName");
                        if (!updatecellsItem)
                            updatecellsItem = cJSON_GetObjectItem(item, "updateCells");
                        if (!sheetnameItem || !updatecellsItem || !cJSON_IsString(sheetnameItem) || !cJSON_IsObject(updatecellsItem))
                        {
                            logger_log_error("Error: Missing sheetname or updatecells in batch data");
                            hasError = true;
                            continue;
                        }
                        // 使用UTF-8编码处理文件名和sheet名
                        rtl::OUString filePath = convertStringToOUString(ensureOdsExtension(std::string(filenameItem->valuestring)).c_str());
                        rtl::OUString absoluteFilePath;
                        getAbsolutePath(filePath, absoluteFilePath);
                        rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
                        // 获取文档ID
                        std::string docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(absoluteFilePath), filemanager::DocumentType::SPREADSHEET);
                        if (docId.empty())
                        {
                            logger_log_error("Failed to open document for update: %s", convertOUStringToString(absoluteFilePath).c_str());
                            hasError = true;
                            continue;
                        }
                        // 执行批量更新多个单元格内容
                        cJSON *result = batchUpdateSpreadsheetContent(docId, sheetName, updatecellsItem);
                        if (!result)
                        {
                            logger_log_error("Error: Batch update failed for file: %s", filenameItem->valuestring);
                            hasError = true;
                        }
                        else
                        {
                            cJSON_Delete(result);
                            logger_log_info("Batch update completed successfully for file: %s", filenameItem->valuestring);
                        }
                    }
                    else
                    {
                        logger_log_error("Error: Invalid data in batch update");
                        hasError = true;
                    }
                }

                if (hasError)
                {
                    return -1;
                }

                logger_log_info("All batch updates completed successfully");
                return 0;
            }
            else
            {
                logger_log_error("Error: Invalid JSON structure");
                return -1;
            }
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO exception in fileupdate: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return -1;
        }
        catch (const std::exception &e)
        {
            logger_log_error("STD exception in fileupdate: %s", e.what());
            return -1;
        }
        catch (...)
        {
            logger_log_error("Unknown exception in fileupdate");
            return -1;
        }
        return 0;
    }

    int readfile(cJSON *root)
    {
        logger_log_info("Reading file data");

        if (!root)
        {
            logger_log_error("Error: Empty root data");
            return -1;
        }

        try
        {
            cJSON *filenameItem = cJSON_GetObjectItem(root, "filename");
            cJSON *sheetnameItem = cJSON_GetObjectItem(root, "sheetname");

            if (!filenameItem || !cJSON_IsString(filenameItem) ||
                !sheetnameItem || !cJSON_IsString(sheetnameItem))
            {
                logger_log_error("Error: Missing filename or sheetname");
                return -1;
            }

            // 使用UTF-8编码处理文件名和sheet名
            rtl::OUString filePathStr = convertStringToOUString(ensureOdsExtension(std::string(filenameItem->valuestring)).c_str());
            rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
            rtl::OUString filePath;
            getAbsolutePath(filePathStr, filePath);
            // 获取文档ID
            std::string docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(filePath), filemanager::DocumentType::SPREADSHEET);
            if (docId.empty())
            {
                logger_log_error("Failed to open document for reading: %s", convertOUStringToString(filePath).c_str());
                return -1;
            }
            cJSON *result = readSheetData(docId, sheetName);
            if (result)
            {
                cJSON_Delete(result);
                logger_log_info("File read successfully");
                return 0;
            }
            else
            {
                logger_log_error("Error: Failed to read file");
                return -1;
            }
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO exception in readfile: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return -1;
        }
        catch (const std::exception &e)
        {
            logger_log_error("STD exception in readfile: %s", e.what());
            return -1;
        }
        catch (...)
        {
            logger_log_error("Unknown exception in readfile");
            return -1;
        }

        return 0;
    }

    void findInSheet(cJSON *results, const char *body)
    {
        // 实现查找工作表内容的函数
        logger_log_info("Finding content in sheet");

        if (!body)
        {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }

        cJSON *root = cJSON_Parse(body);
        if (!root)
        {
            cJSON_AddStringToObject(results, "error", "Invalid JSON body");
            return;
        }

        try
        {
            cJSON *filenameItem = cJSON_GetObjectItem(root, "filename");
            cJSON *sheetnameItem = cJSON_GetObjectItem(root, "sheetname");
            cJSON *searchValueItem = cJSON_GetObjectItem(root, "searchvalue");

            if (!filenameItem || !cJSON_IsString(filenameItem) ||
                !sheetnameItem || !cJSON_IsString(sheetnameItem) ||
                !searchValueItem || !cJSON_IsString(searchValueItem))
            {
                cJSON_AddStringToObject(results, "error", "Missing filename, sheetname or searchvalue");
                cJSON_Delete(root);
                return;
            }

            // 使用UTF-8编码处理文件名和sheet名
            rtl::OUString filePath = convertStringToOUString(ensureOdsExtension(std::string(filenameItem->valuestring)).c_str());
            rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
            rtl::OUString searchValue = convertStringToOUString(searchValueItem->valuestring);

            // 获取文档ID
            std::string docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(filePath), filemanager::DocumentType::SPREADSHEET);
            if (docId.empty())
            {
                logger_log_error("Failed to open document for searching: %s", convertOUStringToString(filePath).c_str());
                cJSON_AddStringToObject(results, "error", "Failed to open document");
                return;
            }
            std::string res = findValueInSheet(docId, sheetName, searchValue);
            if (!res.empty())
            {
                cJSON_AddStringToObject(results, "results", res.c_str());
            }
            else
            {
                cJSON_AddStringToObject(results, "error", "Failed to search in spreadsheet");
            }
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO exception in findInSheet: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            cJSON_AddStringToObject(results, "error", "UNO exception occurred during search");
        }
        catch (const std::exception &e)
        {
            logger_log_error("STD exception in findInSheet: %s", e.what());
            cJSON_AddStringToObject(results, "error", "Exception occurred during search");
        }
        catch (...)
        {
            logger_log_error("Unknown exception in findInSheet");
            cJSON_AddStringToObject(results, "error", "Unknown error occurred during search");
        }

        cJSON_Delete(root);
    }

    void readSheetContents(cJSON *results, const char *body)
    {
        // 实现读取工作表内容的函数
        logger_log_info("Reading sheet contents");

        if (!body)
        {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }

        cJSON *root = cJSON_Parse(body);
        if (!root)
        {
            cJSON_AddStringToObject(results, "error", "Invalid JSON body");
            return;
        }

        try
        {
            cJSON *filenameItem = cJSON_GetObjectItem(root, "filename");
            cJSON *sheetnameItem = cJSON_GetObjectItem(root, "sheetname");

            if (!filenameItem || !cJSON_IsString(filenameItem) ||
                !sheetnameItem || !cJSON_IsString(sheetnameItem))
            {
                cJSON_AddStringToObject(results, "error", "Missing filename or sheetname");
                cJSON_Delete(root);
                return;
            }

            // 使用UTF-8编码处理文件名和sheet名
            rtl::OUString filePathStr = convertStringToOUString(ensureOdsExtension(std::string(filenameItem->valuestring)).c_str());
            rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
            rtl::OUString filePath;
            getAbsolutePath(filePathStr, filePath);
            // 获取文档ID
            std::string docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(filePath), filemanager::DocumentType::SPREADSHEET);
            if (docId.empty())
            {
                logger_log_error("Failed to open document for reading: %s", convertOUStringToString(filePath).c_str());
                cJSON_AddStringToObject(results, "error", "Failed to open document");
                return;
            }
            cJSON *contentMap = readSheetData(docId, sheetName);
            if (contentMap)
            {
                cJSON_AddItemToObject(results, "contents", contentMap);
            }
            else
            {
                cJSON_AddStringToObject(results, "error", "Failed to read spreadsheet contents");
            }
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO exception in readSheetContents: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            cJSON_AddStringToObject(results, "error", "UNO exception occurred while reading contents");
        }
        catch (const std::exception &e)
        {
            logger_log_error("STD exception in readSheetContents: %s", e.what());
            cJSON_AddStringToObject(results, "error", "Exception occurred while reading contents");
        }
        catch (...)
        {
            logger_log_error("Unknown exception in readSheetContents");
            cJSON_AddStringToObject(results, "error", "Unknown error occurred while reading contents");
        }

        cJSON_Delete(root);
    }

    int worksheetAdd(cJSON *taskData)
    {
        logger_log_info("Adding worksheet...........................");

        if (!taskData)
        {
            logger_log_error("Add worksheet task has no data");
            return -1;
        }

        cJSON *filenameItem = cJSON_GetObjectItem(taskData, "filename");
        cJSON *sheetnameItem = cJSON_GetObjectItem(taskData, "sheetname");

        if (!filenameItem || !sheetnameItem)
        {
            logger_log_error("Missing filename or sheetname in add worksheet data");
            return -1;
        }
        rtl::OUString filePath, sheetName;
        getAbsolutePath(convertStringToOUString(filenameItem->valuestring), filePath);
        try
        {
            // 获取文档ID
            std::string docId = filemanager::DocumentManager::getInstance().getDocumentIdByPath(convertOUStringToString(filePath));
            if (docId.empty())
            {
                // 如果文档尚未打开，使用DocumentManager打开它
                docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(filePath), filemanager::DocumentType::SPREADSHEET);
                if (docId.empty())
                {
                    logger_log_error("Failed to open document: %s", filenameItem->valuestring);
                    return -1;
                }
            }

            // 从DocumentManager获取文档对象
            std::shared_ptr<filemanager::Document> doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("Failed to get document object: %s", filenameItem->valuestring);
                return -1;
            }

            // 转换为SpreadsheetDocument
            std::shared_ptr<filemanager::SpreadsheetDocument> spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("Document is not a spreadsheet: %s", filenameItem->valuestring);
                return -1;
            }

            // 检查工作表是否存在
            rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
            css::uno::Reference<css::sheet::XSpreadsheet> sheet = spreadsheetDoc->getSheet(convertOUStringToString(sheetName));
            if (sheet.is())
            {
                logger_log_info("Worksheet %s already exists in file: %s",
                                sheetnameItem->valuestring, filenameItem->valuestring);
                return 0; // 工作表已存在，返回成功
            }

            // 添加新工作表
            if (!spreadsheetDoc->createSheet(convertOUStringToString(sheetName)))
            {
                logger_log_error("Failed to create sheet: %s", sheetnameItem->valuestring);
                return -1;
            }

            // 保存文档
            if (!spreadsheetDoc->save())
            {
                logger_log_error("Failed to save document after adding sheet: %s", filenameItem->valuestring);
                return -1;
            }

            logger_log_info("Successfully added worksheet %s to file: %s",
                            sheetnameItem->valuestring, filenameItem->valuestring);
            return 0;
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO exception in worksheetAdd: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return -1;
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in worksheetAdd: %s", e.what());
            return -1;
        }
        catch (...)
        {
            logger_log_error("Unknown exception in worksheetAdd");
            return -1;
        }
    }

    int worksheetRemove(cJSON *taskData)
    {
        logger_log_info("Removing worksheet");

        if (!taskData)
        {
            logger_log_error("Remove worksheet task has no data");
            return -1;
        }

        cJSON *filenameItem = cJSON_GetObjectItem(taskData, "filename");
        cJSON *sheetnameItem = cJSON_GetObjectItem(taskData, "sheetname");

        if (!filenameItem || !sheetnameItem)
        {
            logger_log_error("Missing filename or sheetname in remove worksheet data");
            return -1;
        }

        try
        {
            rtl::OUString filePathStr = convertStringToOUString(ensureOdsExtension(std::string(filenameItem->valuestring)).c_str());
            // 获取绝对路径
            rtl::OUString filePath;
            getAbsolutePath(filePathStr, filePath);
            // 获取文档ID
            std::string docId = filemanager::DocumentManager::getInstance().getDocumentIdByPath(convertOUStringToString(filePath));
            if (docId.empty())
            {
                // 如果文档尚未打开，使用DocumentManager打开它
                docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(filePath), filemanager::DocumentType::SPREADSHEET);
                if (docId.empty())
                {
                    logger_log_error("Failed to open document: %s", filenameItem->valuestring);
                    return -1;
                }
            }

            // 从DocumentManager获取文档对象
            std::shared_ptr<filemanager::Document> doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("Failed to get document object: %s", filenameItem->valuestring);
                return -1;
            }

            // 转换为SpreadsheetDocument
            std::shared_ptr<filemanager::SpreadsheetDocument> spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("Document is not a spreadsheet: %s", filenameItem->valuestring);
                return -1;
            }

            // 检查工作表是否存在
            rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
            css::uno::Reference<css::sheet::XSpreadsheet> sheet = spreadsheetDoc->getSheet(convertOUStringToString(sheetName));
            if (!sheet.is())
            {
                logger_log_info("Worksheet %s does not exist in file: %s",
                                sheetnameItem->valuestring, filenameItem->valuestring);
                return 0; // 工作表不存在,只给报错信息，不返回错误
            }

            // 删除工作表
            if (!spreadsheetDoc->deleteSheet(convertOUStringToString(sheetName)))
            {
                logger_log_error("Failed to delete sheet: %s", sheetnameItem->valuestring);
                return -1;
            }

            // 保存文档
            if (!spreadsheetDoc->save())
            {
                logger_log_error("Failed to save document after removing sheet: %s", filenameItem->valuestring);
                return -1;
            }

            logger_log_info("Successfully removed worksheet %s from file: %s",
                            sheetnameItem->valuestring, ensureOdsExtension(std::string(filenameItem->valuestring)).c_str());
            return 0;
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO exception in worksheetRemove: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return -1;
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in worksheetRemove: %s", e.what());
            return -1;
        }
        catch (...)
        {
            logger_log_error("Unknown exception in worksheetRemove");
            return -1;
        }
    }
    int worksheetRename(cJSON *taskData)
    {
        logger_log_info("Renaming worksheet");

        if (!taskData)
        {
            logger_log_error("Rename worksheet task has no data");
            return -1;
        }

        cJSON *filenameItem = cJSON_GetObjectItem(taskData, "filename");
        cJSON *sheetnameItem = cJSON_GetObjectItem(taskData, "sheetname");
        cJSON *newsheetnameItem = cJSON_GetObjectItem(taskData, "newsheetname");

        if (!filenameItem || !sheetnameItem || !newsheetnameItem)
        {
            logger_log_error("Missing required parameters for renaming worksheet");
            return -1;
        }
        rtl::OUString filePath, sheetName, newSheetName;
        getAbsolutePath(convertStringToOUString(filenameItem->valuestring), filePath);
        try
        {
            // 获取文档ID
            std::string docId = filemanager::DocumentManager::getInstance().getDocumentIdByPath(convertOUStringToString(filePath));
            if (docId.empty())
            {
                // 如果文档尚未打开，使用DocumentManager打开它
                docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(filePath), filemanager::DocumentType::SPREADSHEET);
                if (docId.empty())
                {
                    logger_log_error("Failed to open document: %s", filenameItem->valuestring);
                    return -1;
                }
            }

            // 从DocumentManager获取文档对象
            std::shared_ptr<filemanager::Document> doc = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!doc)
            {
                logger_log_error("Failed to get document object: %s", filenameItem->valuestring);
                return -1;
            }

            // 转换为SpreadsheetDocument
            std::shared_ptr<filemanager::SpreadsheetDocument> spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(doc);
            if (!spreadsheetDoc)
            {
                logger_log_error("Document is not a spreadsheet: %s", filenameItem->valuestring);
                return -1;
            }

            // 获取工作表容器
            css::uno::Reference<css::sheet::XSpreadsheetDocument> xDoc = spreadsheetDoc->getSpreadsheetDocument();
            if (!xDoc.is())
            {
                logger_log_error("Failed to get spreadsheet document");
                return -1;
            }

            css::uno::Reference<css::sheet::XSpreadsheets> sheets = xDoc->getSheets();
            css::uno::Reference<css::container::XNameContainer> nameContainer(sheets, css::uno::UNO_QUERY);
            if (!nameContainer.is())
            {
                logger_log_error("Failed to obtain XNameContainer for sheets");
                return -1;
            }

            rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
            rtl::OUString newSheetName = convertStringToOUString(newsheetnameItem->valuestring);

            if (!nameContainer->hasByName(sheetName))
            {
                logger_log_error("Worksheet %s does not exist in file: %s",
                                 sheetnameItem->valuestring, filenameItem->valuestring);
                return -1;
            }
            if (nameContainer->hasByName(newSheetName))
            {
                logger_log_error("New worksheet name %s already exists in file: %s",
                                 newsheetnameItem->valuestring, filenameItem->valuestring);
                return -1;
            }

            // 工作表集合已经在前面获取了

            // 复制原工作表到新名称
            uno::Sequence<rtl::OUString> names = sheets->getElementNames();
            sal_Int32 insertIndex = names.getLength();
            sheets->copyByName(sheetName, newSheetName, insertIndex);
            sheets->removeByName(sheetName);

            logger_log_info("Successfully renamed worksheet %s to %s in file: %s",
                            sheetnameItem->valuestring, newsheetnameItem->valuestring, filenameItem->valuestring);

            // 保存文档
            if (!spreadsheetDoc->save())
            {
                logger_log_error("Failed to save document after renaming sheet: %s", filenameItem->valuestring);
                return -1;
            }

            return 0;
        }
        catch (const uno::Exception &e)
        {
            rtl::OString message = rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8);
            logger_log_error("UNO exception in worksheetRename: %s",
                             message.getLength() > 0 ? message.getStr() : "Unknown error in UNO Exception");
            return -1;
        }
        catch (const std::exception &e)
        {
            logger_log_error("STD exception in worksheetRename: %s", e.what());
            return -1;
        }
        catch (...)
        {
            logger_log_error("Unknown exception in worksheetRename");
            return -1;
        }
    }
    int querysheetdata(cJSON *root, cJSON *results)
    {
        logger_log_info("Processing sheetdata with pagination support");

        // 检查输入参数
        cJSON *filenameItem = cJSON_GetObjectItem(root, "filename");
        cJSON *sheetnameItem = cJSON_GetObjectItem(root, "sheetname");
        cJSON *pageSizeItem = cJSON_GetObjectItem(root, "pageSize");
        cJSON *pageIndexItem = cJSON_GetObjectItem(root, "pageIndex");

        if (!filenameItem || !cJSON_IsString(filenameItem) ||
            !sheetnameItem || !cJSON_IsString(sheetnameItem) ||
            !pageSizeItem || !cJSON_IsNumber(pageSizeItem) ||
            !pageIndexItem || !cJSON_IsNumber(pageIndexItem))
        {
            logger_log_error("Invalid or missing parameters in sheetdata task data");
            return -1;
        }

        try
        {
            // 转换参数
            rtl::OUString filePathStr = convertStringToOUString(ensureOdsExtension(std::string(filenameItem->valuestring)).c_str());
            rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
            int pageSize = pageSizeItem->valueint;
            int pageIndex = pageIndexItem->valueint;

            // 获取绝对路径
            rtl::OUString filePath;
            getAbsolutePath(filePathStr, filePath);

            // 获取文档ID
            std::string docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(filePath), filemanager::DocumentType::SPREADSHEET);
            if (docId.empty())
            {
                logger_log_error("Failed to open document: %s", convertOUStringToString(filePath).c_str());
                return -1;
            }

            // 获取工作表总记录数
            int totalRecords = getTotalRecordCount(docId, sheetName);
            if (totalRecords < 0)
            {
                logger_log_error("Failed to get total record count for file: %s, sheet: %s",
                                 filenameItem->valuestring, sheetnameItem->valuestring);
                return -1;
            }

            // 计算分页范围
            int startIndex = pageIndex * pageSize;
            int endIndex = std::min(startIndex + pageSize, totalRecords);

            // 读取指定范围的数据
            cJSON *sheetContent = readSheetDataRange(docId, sheetName, startIndex, endIndex);
            if (!sheetContent)
            {
                logger_log_error("Failed to read spreadsheet contents for file: %s, sheet: %s",
                                 filenameItem->valuestring, sheetnameItem->valuestring);
                return -1;
            }

            logger_log_info("Sheet content: %s", cJSON_Print(sheetContent));

            // 构造返回结果
            cJSON_AddNumberToObject(results, "totalRecords", totalRecords);
            cJSON_AddNumberToObject(results, "pageSize", pageSize);
            cJSON_AddNumberToObject(results, "pageIndex", pageIndex);
            cJSON_AddItemToObject(results, "data", sheetContent);

            logger_log_info("Successfully processed sheetdata for file: %s, sheet: %s",
                            filenameItem->valuestring, sheetnameItem->valuestring);
            return 0;
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO exception in sheetdata: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return -1;
        }
        catch (const std::exception &e)
        {
            logger_log_error("STD exception in sheetdata: %s", e.what());
            return -1;
        }
        catch (...)
        {
            logger_log_error("Unknown exception in sheetdata");
            return -1;
        }
    }
    // 辅助函数：生成时间戳
    std::string generateTimestamp()
    {
        time_t now = time(0);
        tm *ltm = localtime(&now);
        if (!ltm)
        {
            logger_log_error("Failed to get local time");
            return "";
        }

        char timestamp[20];
        sprintf(timestamp, "%04d%02d%02d_%02d%02d%02d",
                1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday,
                ltm->tm_hour, ltm->tm_min, ltm->tm_sec);

        return std::string(timestamp);
    }

    // 辅助函数：处理非默认工作表
    bool processNonDefaultSheets(const std::string &docId,
                                 const uno::Sequence<rtl::OUString> &sheetNames,
                                 const rtl::OUString &defaultSheetName,
                                 const rtl::OUString &tempSheetName,
                                 cJSON */*sheetArray*/)
    {
        for (sal_Int32 i = 0; i < sheetNames.getLength(); ++i)
        {
            // 跳过默认工作表和临时工作表
            if (sheetNames[i] == defaultSheetName || sheetNames[i] == tempSheetName)
            {
                continue;
            }

            try
            {
                std::string currentSheetName = convertOUStringToString(sheetNames[i]);
                logger_log_info("Processing sheet: %s", currentSheetName.c_str());

                // 获取总记录数
                int totalRecords = getTotalRecordCount(docId, sheetNames[i]);
                if (totalRecords < 0)
                {
                    logger_log_error("Failed to get total record count for sheet: %s", currentSheetName.c_str());
                    return false;
                }

                // 读取所有数据
                cJSON *sheetData = readSheetDataRange(docId, sheetNames[i], 0, totalRecords);
                if (!sheetData)
                {
                    logger_log_error("Failed to read data for sheet: %s", currentSheetName.c_str());
                    return false;
                }

                // 更新电子表格内容
                cJSON *resjson = batchUpdateSpreadsheetContent(docId, sheetNames[i], sheetData);
                if (!resjson)
                {
                    logger_log_error("Failed to update content for sheet: %s", currentSheetName.c_str());
                    cJSON_Delete(sheetData);
                    return false;
                }

                // 检查更新结果
                if (cJSON_GetObjectItem(resjson, "result") != nullptr &&
                    std::string(cJSON_GetObjectItem(resjson, "result")->valuestring) != "success")
                {
                    logger_log_error("Failed to update cell contents in sheet: %s", currentSheetName.c_str());
                    cJSON_Delete(sheetData);
                    cJSON_Delete(resjson);
                    return false;
                }

                // 释放资源
                cJSON_Delete(sheetData);
                cJSON_Delete(resjson);
            }
            catch (const std::exception &e)
            {
                logger_log_error("Exception processing sheet: %s, error: %s",
                                 convertOUStringToString(sheetNames[i]).c_str(), e.what());
                return false;
            }
        }

        return true;
    }

    // 辅助函数：重命名和替换工作表
    bool renameAndReplaceSheets(uno::Reference<sheet::XSpreadsheets> &sheets,
                                const rtl::OUString &defaultSheetName,
                                const rtl::OUString &tempSheetName,
                                const rtl::OUString &backupSheetName)
    {
        try
        {
            // 复制原工作表到备份名称
            sal_Int32 insertIndex = sheets->getElementNames().getLength();
            sheets->copyByName(defaultSheetName, backupSheetName, insertIndex);
            sheets->removeByName(defaultSheetName);

            logger_log_info("Original sheet renamed to backup: %s -> %s",
                            convertOUStringToString(defaultSheetName).c_str(),
                            convertOUStringToString(backupSheetName).c_str());

            // 复制临时工作表为默认工作表
            sal_Int32 insertIndex2 = sheets->getElementNames().getLength();
            sheets->copyByName(tempSheetName, defaultSheetName, insertIndex2);
            sheets->removeByName(tempSheetName);

            logger_log_info("Temporary sheet renamed to default: %s -> %s",
                            convertOUStringToString(tempSheetName).c_str(),
                            convertOUStringToString(defaultSheetName).c_str());

            return true;
        }
        catch (const uno::Exception &e)
        {
            std::string errorMsg = rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr();
            logger_log_error("UNO exception during sheet rename operations: %s", errorMsg.c_str());
            return false;
        }
    }

    // 处理单文件更换模板
    int fileTemplateChange(const std::string &filename)
    {
        logger_log_info("Starting file template change for: %s", filename.c_str());

        // 生成唯一时间戳作为临时工作表和备份工作表的标识
        std::string timestamp = generateTimestamp();
        if (timestamp.empty())
        {
            logger_log_error("Failed to generate timestamp");
            return -1;
        }

        try
        {
            // 处理文件路径
            std::string odsFilename = ensureOdsExtension(filename);
            rtl::OUString filePathStr = convertStringToOUString(odsFilename.c_str());

            // 获取绝对路径
            rtl::OUString filePath;
            getAbsolutePath(filePathStr, filePath);

            // 首先通过DocumentManager加载文档
            std::string docId = filemanager::DocumentManager::getInstance().openDocument(
                convertOUStringToString(filePath), filemanager::DocumentType::SPREADSHEET);
            if (docId.empty())
            {
                logger_log_error("Failed to load document: %s", odsFilename.c_str());
                return -1;
            }

            // 获取文档对象
            std::shared_ptr<filemanager::Document> docObj = filemanager::DocumentManager::getInstance().getDocument(docId);
            if (!docObj)
            {
                logger_log_error("Failed to get document object: %s", odsFilename.c_str());
                return -1;
            }

            // 转换为电子表格文档对象
            auto spreadsheetDoc = std::dynamic_pointer_cast<filemanager::SpreadsheetDocument>(docObj);
            if (!spreadsheetDoc)
            {
                logger_log_error("Failed to convert to spreadsheet document: %s", odsFilename.c_str());
                return -1;
            }

            // 获取UNO文档接口
            uno::Reference<sheet::XSpreadsheetDocument> doc = spreadsheetDoc->getSpreadsheetDocument();
            if (!doc.is())
            {
                logger_log_error("Failed to get XSpreadsheetDocument: %s", odsFilename.c_str());
                return -1;
            }

            // 获取工作表集合
            uno::Reference<sheet::XSpreadsheets> sheets = doc->getSheets();
            uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);

            // 使用getDefaultData获取默认配置
            rtl::OUString defaultFilePath;
            rtl::OUString defaultFileName;
            rtl::OUString defaultSheetName;
            getDefaultData(defaultFilePath, defaultFileName, defaultSheetName);

            // 检查默认工作表是否存在
            if (!nameAccess->hasByName(defaultSheetName))
            {
                logger_log_error("Default sheet %s not found in document",
                                 convertOUStringToString(defaultSheetName).c_str());
                return -1;
            }

            // 读取默认工作表的缓存数据
            std::shared_ptr<CharacterIndex> idxPtr = filemanager::TemplateIndexCacheManager::getInstance()
                                                         .getCharacterInfos(defaultFilePath + defaultFileName, defaultSheetName);
            if (!idxPtr)
            {
                logger_log_error("Failed to get character infos from cache");
                return -1;
            }

            std::vector<TextCharInfo> defaultInfos = idxPtr->getAll(); // 获取所有数据

            if (defaultInfos.empty())
            {
                logger_log_error("No character information found in default template");
                return -1;
            }

            // 创建临时工作表名称
            std::string tempSheetNameStr = convertOUStringToString(defaultSheetName) + "_temp_" + timestamp;
            rtl::OUString tempSheetName = convertStringToOUString(tempSheetNameStr.c_str());
            logger_log_info("Temporary sheet name: %s", tempSheetNameStr.c_str());

            // 检查工作表是否已存在并删除
            if (nameAccess->hasByName(tempSheetName))
            {
                logger_log_info("Temporary worksheet %s already exists, deleting it", tempSheetNameStr.c_str());
                // 删除工作表
                if (!spreadsheetDoc->deleteSheet(tempSheetNameStr))
                {
                    logger_log_error("Failed to delete existing temporary sheet: %s", tempSheetNameStr.c_str());
                    return -1;
                }
                logger_log_info("Existing temporary sheet deleted: %s", tempSheetNameStr.c_str());
            }

            // 创建工作表
            if (!spreadsheetDoc->createSheet(tempSheetNameStr))
            {
                logger_log_error("Failed to create temporary sheet: %s", tempSheetNameStr.c_str());
                return -1;
            }
            logger_log_info("Temporary sheet created: %s", tempSheetNameStr.c_str());

            // 将默认工作表的数据写入新创建的临时工作表
            if (!writeCharacterInfosToSheet(docId, tempSheetName, defaultInfos))
            {
                logger_log_error("Failed to write character infos to temporary sheet");
                // 尝试清理已创建的临时工作表
                spreadsheetDoc->deleteSheet(tempSheetNameStr);
                return -1;
            }
            logger_log_info("Content written to temporary sheet: %s", tempSheetNameStr.c_str());

            // 读取其它非默认工作表单元格的内容并重新写入新缓存
            uno::Sequence<rtl::OUString> sheetNames = sheets->getElementNames();
            cJSON *sheetArray = cJSON_CreateArray();
            if (!processNonDefaultSheets(docId, sheetNames, defaultSheetName, tempSheetName, sheetArray))
            {
                logger_log_error("Failed to process non-default sheets");
                // 释放资源并清理
                cJSON_Delete(sheetArray);
                spreadsheetDoc->deleteSheet(tempSheetNameStr);
                return -1;
            }

            // 释放JSON资源
            cJSON_Delete(sheetArray);

            // 创建备份工作表名称
            std::string backupSheetNameStr = convertOUStringToString(defaultSheetName) + "_backup_" + timestamp;
            rtl::OUString backupSheetName = convertStringToOUString(backupSheetNameStr.c_str());
            logger_log_info("Backup sheet name: %s", backupSheetNameStr.c_str());

            // 执行工作表重命名和替换操作
            if (!renameAndReplaceSheets(sheets, defaultSheetName, tempSheetName, backupSheetName))
            {
                logger_log_error("Failed to rename and replace sheets");
                spreadsheetDoc->deleteSheet(tempSheetNameStr);
                return -1;
            }

            // 可选删除备份工作表（设置为false可以保留备份用于调试）
            bool deleteBackup = true;
            if (deleteBackup)
            {
                if (!spreadsheetDoc->deleteSheet(backupSheetNameStr))
                {
                    logger_log_warn("Failed to delete backup sheet: %s, but operation continues",
                                    backupSheetNameStr.c_str());
                    // 即使删除备份失败，也继续执行，因为主要功能已经完成
                }
                else
                {
                    logger_log_info("Backup sheet deleted: %s", backupSheetNameStr.c_str());
                }
            }

            // 保存文档以确保所有更改都被保存
            if (!docObj->save())
            {
                logger_log_error("Failed to save document after template change");
                return -1;
            }

            logger_log_info("File template change completed successfully for: %s", odsFilename.c_str());
            return 0;
        }
        catch (const uno::Exception &e)
        {
            std::string errorMsg = rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr();
            logger_log_error("UNO exception during file template change: %s", errorMsg.c_str());
            return -1;
        }
        catch (const std::exception &e)
        {
            logger_log_error("Standard exception during file template change: %s", e.what());
            return -1;
        }
        catch (...)
        {
            logger_log_error("Unknown exception during file template change");
            return -1;
        }
    }

} // namespace filemanager
