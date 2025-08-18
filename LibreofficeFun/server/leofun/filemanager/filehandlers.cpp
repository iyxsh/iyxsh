#include "filehandlers.h"
#include "spreadsheet.h"
#include "filequeue.h"
#include "lofficeconn.h"
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
#include "lofficeconn.h"

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
        // 从缓存索引获取数据
        std::vector<TextCharInfo> infos = filemanager::TemplateIndexCacheManager::getInstance().getCharacterInfos(filePath + defaultFileName, sheetName);
        if (!writeCharacterInfosToSheet(absfilePath, sheetName, infos))
        {
            logger_log_error("Failed to write character infos to sheet");
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

        if (!fileInfo.xComponent.is())
        {
            logger_log_info("File %s is not opened, no need to close", filename.c_str());
            return -1;
        }

        try
        {
            // 关闭文档
            closeDocument(fileInfo.xComponent);

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
            if (!LibreOfficeConnectionManager::initialize())
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
                        rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
                        rtl::OUString absoluteFilePath;
                        getAbsolutePath(filePath, absoluteFilePath);
                        // 执行批量更新多个单元格内容
                        cJSON *result = batchUpdateSpreadsheetContent(absoluteFilePath, sheetName, updatecellsItem);
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
            cJSON *result = readSheetData(filePath, sheetName);
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

            cJSON *foundItems = findValueInSheet(filePath, sheetName, searchValue);
            if (foundItems)
            {
                cJSON_AddItemToObject(results, "results", foundItems);
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
            cJSON *contentMap = readSheetData(filePath, sheetName);
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

        try
        {
            rtl::OUString filePathStr = convertStringToOUString(ensureOdsExtension(std::string(filenameItem->valuestring)).c_str());
            // 获取绝对路径
            rtl::OUString filePath;
            getAbsolutePath(filePathStr, filePath);
            // 首先判断是否文件已经加载
            FileInfo fileInfo = filemanager::FileQueueManager::getInstance().getFileInfo(std::string(filenameItem->valuestring));
            uno::Reference<sheet::XSpreadsheetDocument> xDoc;
            if (!fileInfo.xComponent.is())
            {
                xDoc = loadSpreadsheetDocument(filePath, fileInfo.xComponent);
                if (!fileInfo.xComponent.is())
                {
                    logger_log_error("Failed to load document: %s", filenameItem->valuestring);
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

            // 获取工作表集合
            uno::Reference<sheet::XSpreadsheets> sheets = xDoc->getSheets();
            uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);

            // 检查工作表是否已存在
            rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
            if (nameAccess->hasByName(sheetName))
            {
                logger_log_info("Worksheet %s already exists in file: %s",
                                sheetnameItem->valuestring, filenameItem->valuestring);
                closeDocument(xComp);
                return 0; // 工作表已存在，返回成功
            }

            // 添加新工作表
            sheets->insertNewByName(sheetName, 0);

            // 保存文档
            saveDocumentDirect(xDoc);

            // 关闭文档
            // closeDocument(xComp);

            logger_log_info("Successfully added worksheet %s to file: %s",
                            std::string(sheetnameItem->valuestring).c_str(), ensureOdsExtension(std::string(filenameItem->valuestring)).c_str());
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
            uno::Reference<lang::XComponent> xComp;
            uno::Reference<sheet::XSpreadsheetDocument> xDoc = loadSpreadsheetDocument(filePath, xComp);
            if (!xDoc.is())
            {
                logger_log_error("Failed to load document: %s", filenameItem->valuestring);
                return -1;
            }

            // 获取工作表集合
            uno::Reference<sheet::XSpreadsheets> sheets = xDoc->getSheets();
            uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);

            // 检查工作表是否存在
            rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
            if (!nameAccess->hasByName(sheetName))
            {
                logger_log_info("Worksheet %s does not exist in file: %s",
                                sheetnameItem->valuestring, filenameItem->valuestring);
                closeDocument(xComp);
                return -1; // 工作表不存在
            }

            // 删除工作表
            sheets->removeByName(sheetName);

            // 保存文档
            rtl::OUString filenameOstr = convertStringToOUString(ensureOdsExtension(std::string(filenameItem->valuestring)).c_str());
            rtl::OUString absoluteFilePath;
            getAbsolutePath(filenameOstr, absoluteFilePath);
            saveDocumentDirect(xDoc);

            // 关闭文档
            // closeDocument(xComp);

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

        try
        {
            rtl::OUString filePathStr = convertStringToOUString(ensureOdsExtension(std::string(filenameItem->valuestring)).c_str());
            // 获取绝对路径
            rtl::OUString filePath;
            getAbsolutePath(filePathStr, filePath);
            uno::Reference<lang::XComponent> xComp;
            uno::Reference<sheet::XSpreadsheetDocument> xDoc = loadSpreadsheetDocument(filePath, xComp);

            if (!xDoc.is())
            {
                logger_log_error("Failed to load document: %s", filenameItem->valuestring);
                return -1;
            }

            uno::Reference<sheet::XSpreadsheets> sheets = xDoc->getSheets();
            uno::Reference<container::XNameContainer> nameContainer(sheets, uno::UNO_QUERY);
            if (!nameContainer.is())
            {
                logger_log_error("Failed to obtain XNameContainer for sheets");
                closeDocument(xComp);
                return -1;
            }

            rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
            rtl::OUString newSheetName = convertStringToOUString(newsheetnameItem->valuestring);

            if (!nameContainer->hasByName(sheetName))
            {
                logger_log_error("Worksheet %s does not exist in file: %s",
                                 sheetnameItem->valuestring, filenameItem->valuestring);
                closeDocument(xComp);
                return -1;
            }
            if (nameContainer->hasByName(newSheetName))
            {
                logger_log_error("New worksheet name %s already exists in file: %s",
                                 newsheetnameItem->valuestring, filenameItem->valuestring);
                closeDocument(xComp);
                return -1;
            }

            // 复制原工作表到新名称
            uno::Sequence<rtl::OUString> names = sheets->getElementNames();
            sal_Int32 insertIndex = names.getLength();
            sheets->copyByName(sheetName, newSheetName, insertIndex);
            sheets->removeByName(sheetName);

            logger_log_info("Successfully renamed worksheet %s to %s in file: %s",
                            sheetnameItem->valuestring, newsheetnameItem->valuestring, filenameItem->valuestring);

            // 保存文档
            saveDocumentDirect(xDoc);
            // closeDocument(xComp);
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

            // 获取工作表总记录数
            int totalRecords = getTotalRecordCount(filePath, sheetName);
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
            cJSON *sheetContent = readSheetDataRange(filePath, sheetName, startIndex, endIndex);
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
    // 处理单文件更换模板
    int fileTempleteChange(const std::string &filename)
    {
        // 加载文档
        rtl::OUString absfilePath;
        getAbsolutePath(convertStringToOUString(filename.c_str()), absfilePath);
        uno::Reference<lang::XComponent> xComp;
        uno::Reference<sheet::XSpreadsheetDocument> xDoc = loadSpreadsheetDocument(absfilePath, xComp);
        if (!xDoc.is())
        {
            return -1;
        }
        // 使用getDefaultData获取默认配置
        rtl::OUString defaultFilePath;
        rtl::OUString defaultFileName;
        rtl::OUString defaultSheetName;
        getDefaultData(defaultFilePath, defaultFileName, defaultSheetName);

        // 读取默认工作表的缓存数据
        std::vector<TextCharInfo> defaultInfos = filemanager::TemplateIndexCacheManager::getInstance().getCharacterInfos(defaultFilePath + defaultFileName, defaultSheetName);

        // 创建新的临时工作表
        rtl::OUString tempSheetName = defaultSheetName + "_temp";

        // 获取工作表集合
        uno::Reference<sheet::XSpreadsheets> sheets = xDoc->getSheets();
        uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);
        {
            // 检查工作表是否已存在
            if (nameAccess->hasByName(tempSheetName))
            {
                logger_log_info("Worksheet %s already exists in file: %s",
                                convertOUStringToString(tempSheetName).c_str(), filename.c_str());
                // 删除工作表
                sheets->removeByName(tempSheetName);
                saveDocumentDirect(xDoc);
            }

            // 1 添加新工作表
            sheets->insertNewByName(tempSheetName, 0);

            // 保存文档
            saveDocumentDirect(xDoc);
        }
        // 2 将默认工作表的数据写入新创建的工作表
        if (!writeCharacterInfosToSheet(absfilePath, tempSheetName, defaultInfos))
        {
            logger_log_error("Failed to write character infos to new temporary sheet");
            return -1;
        }
        saveDocumentDirect(xDoc);
        // 3 读取其它非默认工作表单元格的内容并重新写入新缓存
        uno::Sequence<rtl::OUString> sheetNames = sheets->getElementNames();
        cJSON *sheetArray = cJSON_CreateArray();
        for (sal_Int32 i = 0; i < sheetNames.getLength(); ++i)
        {
            // 过滤wordsSheetName
            if (sheetNames[i] == defaultSheetName || sheetNames[i] == tempSheetName)
            {
                continue;
            }
            int totalRecords = getTotalRecordCount(absfilePath, sheetNames[i]);                 // 获取总记录数
            cJSON *sheetData = readSheetDataRange(absfilePath, sheetNames[i], 0, totalRecords); // 读取所有数据
            cJSON *resjson = batchUpdateSpreadsheetContent(absfilePath, sheetNames[i], sheetData);
            if (cJSON_GetObjectItem(resjson, "result") != nullptr &&
                std::string(cJSON_GetObjectItem(resjson, "result")->valuestring) != "success")
            {
                logger_log_error("Failed to update cell contents in the temporary sheet");
                return -1;
            }
        }
        saveDocumentDirect(xDoc);
        // 4 重命名默认工作表为备份
        rtl::OUString backupSheetName = defaultSheetName + "_backup";
        sal_Int32 insertIndex = sheetNames.getLength();
        sheets->copyByName(defaultSheetName, backupSheetName, insertIndex);
        sheets->removeByName(defaultSheetName);

        logger_log_info("Successfully renamed worksheet %s to %s in file: %s",
                        convertOUStringToString(defaultSheetName).c_str(), convertOUStringToString(backupSheetName).c_str(), filename.c_str());
        saveDocumentDirect(xDoc);
        // 5 重命名新加的临时工作表为默认工作表
        sheets->copyByName(tempSheetName, defaultSheetName, insertIndex);
        sheets->removeByName(tempSheetName);

        logger_log_info("Successfully renamed worksheet %s to %s in file: %s",
                        convertOUStringToString(tempSheetName).c_str(), convertOUStringToString(defaultSheetName).c_str(), filename.c_str());
        saveDocumentDirect(xDoc);

        // 6 可选删除旧的备份工作表
        //  删除工作表
        // sheets->removeByName(backupSheetName);
        // saveDocumentDirect(xDoc);
        return 0;
    }
} // namespace filemanager
