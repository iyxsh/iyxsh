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
    int createfile(std::string& filename)
    {
        // 实现创建文件的函数
        logger_log("Creating file: %s", filename.c_str());
        
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
        logger_log("Full file path: %s", filePathStr.c_str());
        
        // 构造默认模板文件路径
        std::string defaultFilePathStr = std::string(datapath) + "/" + std::string(defaultname);
        logger_log("Default template file path: %s", defaultFilePathStr.c_str());
        
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
        cJSON* cachedSheetData = getCachedSheetData(defaultFilePath, sheetName);
        
        // 创建新电子表格文件，使用缓存的数据
        cJSON *result = createNewSpreadsheetFile(filePath, sheetName, cachedSheetData);
        
        if (result)
        {
            cJSON_Delete(result);
            logger_log("File %s created successfully", filename.c_str());
            return 0;
        }
        else
        {
            logger_log("Failed to create file %s", filename.c_str());
            return -1;
        }
    }

    int updatefile(cJSON *root)
    {
        // 实现更新文件的函数
        logger_log("Updating file with data");
        
        if (!root)
        {
            logger_log("Error: Empty root data");
            return -1;
        }
        
        // 使用LibreOffice连接管理器
        if (!LibreOfficeConnectionManager::initialize())
        {
            logger_log("Error: Failed to initialize LibreOffice connection");
            return -1;
        }
        
        try
        {
            // 支持批量：body为数组，或单对象
            if (cJSON_IsArray(root))
            {
                // 对于批量更新，我们使用新的批量更新方法
                // 提取第一个项目获取文件路径和工作表名
                cJSON *firstItem = cJSON_GetArrayItem(root, 0);
                if (!firstItem || !cJSON_IsObject(firstItem))
                {
                    logger_log("Error: Empty or invalid batch update data");
                    return -1;
                }

                cJSON *filenameItem = cJSON_GetObjectItem(firstItem, "filename");
                cJSON *sheetnameItem = cJSON_GetObjectItem(firstItem, "sheetname");

                if (!filenameItem || !sheetnameItem || !cJSON_IsString(filenameItem) || !cJSON_IsString(sheetnameItem))
                {
                    logger_log("Error: Missing filename or sheetname in batch data");
                    return -1;
                }
                
                // 使用UTF-8编码处理文件名和sheet名
                rtl::OUString filePath = convertStringToOUString(filenameItem->valuestring);
                rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
                
                // 执行批量更新
                cJSON *result = batchUpdateSpreadsheetContent(filePath, sheetName, root);
                if (result)
                {
                    cJSON_Delete(result);
                    logger_log("Batch update completed successfully");
                    return 0;
                }
                else
                {
                    logger_log("Error: Batch update failed");
                    return -1;
                }
            }
            else if (cJSON_IsObject(root))
            {
                // 单个更新
                cJSON *filenameItem = cJSON_GetObjectItem(root, "filename");
                cJSON *sheetnameItem = cJSON_GetObjectItem(root, "sheetname");
                cJSON *celladdrItem = cJSON_GetObjectItem(root, "celladdr");
                cJSON *valueItem = cJSON_GetObjectItem(root, "value");
                cJSON *typeItem = cJSON_GetObjectItem(root, "type");

                if (!filenameItem || !sheetnameItem || !celladdrItem || !valueItem ||
                    !cJSON_IsString(filenameItem) || !cJSON_IsString(sheetnameItem) || 
                    !cJSON_IsString(celladdrItem) || !cJSON_IsString(valueItem))
                {
                    logger_log("Error: Missing required fields in update data");
                    return -1;
                }

                // 使用UTF-8编码处理文件名和sheet名
                rtl::OUString filePath = convertStringToOUString(filenameItem->valuestring);
                rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
                rtl::OUString cellAddr = convertStringToOUString(celladdrItem->valuestring);
                rtl::OUString value = convertStringToOUString(valueItem->valuestring);
                rtl::OUString type = typeItem && cJSON_IsString(typeItem) ? 
                                     convertStringToOUString(typeItem->valuestring) : 
                                     rtl::OUString::createFromAscii("string");

                // 执行更新
                cJSON *result = updateSpreadsheetContent(filePath, sheetName, cellAddr, value, type);
                if (result)
                {
                    cJSON_Delete(result);
                    logger_log("Single update completed successfully");
                    return 0;
                }
                else
                {
                    logger_log("Error: Single update failed");
                    return -1;
                }
            }
            else
            {
                logger_log("Error: Invalid JSON structure");
                return -1;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "updatefile std exception: " << e.what() << std::endl;
            logger_log("Error: Update failed with exception: %s", e.what());
            return -1;
        }
        catch (...)
        {
            std::cerr << "updatefile unknown exception occurred" << std::endl;
            logger_log("Error: Update failed with unknown exception");
            return -1;
        }
    }

    void findInSheet(cJSON *results, const char *body)
    {
        // 实现查找工作表内容的函数
        logger_log("Finding content in sheet");
        
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
        logger_log("Reading sheet contents");
        
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