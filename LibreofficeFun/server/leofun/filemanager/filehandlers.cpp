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

namespace filemanager
{
    int createfile(std::string &filename)
    {
        // 实现创建文件的函数
        logger_log_info("Creating file: %s", filename.c_str());

        // 从配置中获取数据路径
        const char *datapath = json_config_get_string("datapath");
        if (!datapath)
        {
            datapath = "../data"; // 默认数据路径
        }

        // 从配置中获取默认文件名
        const char *defaultname = json_config_get_string("defaultname");
        if (!defaultname)
        {
            defaultname = "default.xlsx"; // 默认文件名
        }

        // 构造完整文件路径
        std::string filePathStr = std::string(datapath) + "/" + filename;
        logger_log_info("Full file path: %s", filePathStr.c_str());

        // 构造默认模板文件路径
        std::string defaultFilePathStr = std::string(datapath) + "/" + std::string(defaultname);
        logger_log_info("Default template file path: %s", defaultFilePathStr.c_str());

        // 转换为绝对路径
        std::string absolutePath = convertToAbsolutePath(filePathStr);
        rtl::OUString filePath = convertStringToOUString(absolutePath.c_str());

        // 转换默认模板文件为绝对路径
        std::string absoluteDefaultPath = convertToAbsolutePath(defaultFilePathStr);
        rtl::OUString defaultFilePath = convertStringToOUString(absoluteDefaultPath.c_str());

        // 获取默认工作表名
        const char *defaultSheetName = json_config_get_string("WordsSheet");
        if (!defaultSheetName)
        {
            defaultSheetName = "WordsSheet"; // 默认值
        }
        rtl::OUString sheetName = convertStringToOUString(defaultSheetName);

        // 从默认模板文件获取缓存的表数据
        cJSON *cachedSheetData = getCachedSheetData(defaultFilePath, sheetName);

        // 创建新电子表格文件，使用缓存的数据
        cJSON *result = createNewSpreadsheetFile(filePath, sheetName, cachedSheetData);

        if (result)
        {
            cJSON_Delete(result);
            logger_log_info("File %s created successfully", filename.c_str());
            return 0;
        }
        else
        {
            logger_log_error("Failed to create file %s", filename.c_str());
            return -1;
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

        // 使用LibreOffice连接管理器
        if (!LibreOfficeConnectionManager::initialize())
        {
            logger_log_error("Error: Failed to initialize LibreOffice connection");
            return -1;
        }

        try
        {
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
                        if (!sheetnameItem || !updatecellsItem || !cJSON_IsString(sheetnameItem) || !cJSON_IsArray(updatecellsItem))
                        {
                            logger_log_error("Error: Missing sheetname or updatecells in batch data");
                            hasError = true;
                            continue;
                        }
                        // 使用UTF-8编码处理文件名和sheet名
                        rtl::OUString filePath = convertStringToOUString(filenameItem->valuestring);
                        rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
                        rtl::OUString absoluteFilePath;
                        getAbsolutePath(filePath,absoluteFilePath);
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
        catch (const std::exception &e)
        {
            std::cerr << "updatefile std exception: " << e.what() << std::endl;
            logger_log_error("Error: Update failed with exception: %s", e.what());
            return -1;
        }
        catch (...)
        {
            std::cerr << "updatefile unknown exception occurred" << std::endl;
            logger_log_error("Error: Update failed with unknown exception");
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

        cJSON *filenameItem = cJSON_GetObjectItem(root, "filename");
        cJSON *sheetnameItem = cJSON_GetObjectItem(root, "sheetname");

        if (!filenameItem || !cJSON_IsString(filenameItem) ||
            !sheetnameItem || !cJSON_IsString(sheetnameItem))
        {
            logger_log_error("Error: Missing filename or sheetname");
            return -1;
        }

        // 使用UTF-8编码处理文件名和sheet名
        rtl::OUString filePath = convertStringToOUString(filenameItem->valuestring);
        rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);

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
        rtl::OUString filePath = convertStringToOUString(filenameItem->valuestring);
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
        rtl::OUString filePath = convertStringToOUString(filenameItem->valuestring);
        rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);

        cJSON *contentMap = readSheetData(filePath, sheetName);
        if (contentMap)
        {
            cJSON_AddItemToObject(results, "contents", contentMap);
        }
        else
        {
            cJSON_AddStringToObject(results, "error", "Failed to read spreadsheet contents");
        }

        cJSON_Delete(root);
    }
}