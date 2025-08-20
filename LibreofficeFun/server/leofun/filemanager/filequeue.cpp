#include "../filemanager/filequeue.h"
#include "filehandlers.h"
#include "template_index_cache.h" // 如需直接使用索引缓存，可包含此头文件
#include "utils.h"
#include "../logger/logger.h"
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "spreadsheet.h"
namespace filemanager
{

    // 获取单例实例
    FileQueueManager &FileQueueManager::getInstance()
    {
        static FileQueueManager instance;
        return instance;
    }

    // 构造函数
    FileQueueManager::FileQueueManager() : shouldStop(false), templateLoading(false)
    {
        logger_log_info("FileQueueManager initialized");
    }

    // 析构函数
    FileQueueManager::~FileQueueManager()
    {
        stopTaskProcessor();
        logger_log_info("FileQueueManager destroyed");
    }

    // 添加文件状态
    void FileQueueManager::addFileStatus(const std::string &filename, FileStatus status, const std::string &errorMsg)
    {
        std::lock_guard<std::mutex> lock(statusMutex);
        FileInfo info;
        info.filename = filename;
        info.status = status;
        info.lastModified = std::time(nullptr);
        info.errorMessage = errorMsg;
        info.xComponent = nullptr; // 新增字段，初始为空
        info.refCount = 0;
        fileStatusMap[filename] = info;

        logger_log_info("Added file status: %s - %d",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(filename.c_str())).c_str(),
                        status);
    }

    // 添加文件状态（完整信息）
    void FileQueueManager::addFileStatus(const FileInfo &info)
    {
        std::lock_guard<std::mutex> lock(statusMutex);
        fileStatusMap[info.filename] = info;
    }

    // 更新文件状态
    void FileQueueManager::updateFileStatus(const std::string &filename, FileStatus status, const std::string &errorMsg)
    {
        std::lock_guard<std::mutex> lock(statusMutex);
        auto it = fileStatusMap.find(filename);
        if (it != fileStatusMap.end())
        {
            it->second.status = status;
            it->second.errorMessage = errorMsg;
            it->second.lastModified = std::time(nullptr);
        }
        else
        {
            // 如果文件不存在于状态映射表中，则添加新条目
            FileInfo info;
            info.filename = filename;
            info.status = status;
            info.errorMessage = errorMsg;
            info.lastModified = std::time(nullptr);
            info.xComponent = nullptr;
            info.refCount = 0;
            fileStatusMap[filename] = info;
        }
    }

    // 获取文件状态
    FileStatus FileQueueManager::getFileStatus(const std::string &filename)
    {
        std::lock_guard<std::mutex> lock(statusMutex);
        auto it = fileStatusMap.find(filename);
        if (it != fileStatusMap.end())
        {
            return it->second.status;
        }
        return FILE_STATUS_CLOSED; // 默认返回已关闭状态
    }

    // 获取文件信息
    FileInfo FileQueueManager::getFileInfo(const std::string &filename)
    {
        std::lock_guard<std::mutex> lock(statusMutex);
        auto it = fileStatusMap.find(filename);
        if (it != fileStatusMap.end())
        {
            return it->second;
        }

        FileInfo info;
        info.filename = filename;
        info.status = FILE_STATUS_CLOSED;
        info.lastModified = 0;
        info.xComponent = nullptr;
        info.refCount = 0;
        return info;
    }

    // 获取所有文件状态的副本
    std::unordered_map<std::string, FileInfo> FileQueueManager::getFileStatusMapCopy() const
    {
        std::lock_guard<std::mutex> lock(statusMutex);
        return fileStatusMap; // 返回副本，确保线程安全
    }

    std::unordered_map<std::string, FileInfo> FileQueueManager::getFileStatusMap() const
    {
        std::lock_guard<std::mutex> lock(statusMutex);
        return fileStatusMap; // 返回副本，确保线程安全
    }

    // 添加文件任务
    void FileQueueManager::addFileTask(const FileTask &task)
    {
        std::lock_guard<std::mutex> lock(taskMutex);
        taskQueue.push(task);
        taskCondition.notify_one();

        logger_log_info("Added file task: type=%d, filename=%s", task.type, task.filename.c_str());
    }

    // 获取下一个任务
    bool FileQueueManager::getNextTask(FileTask &task)
    {
        std::lock_guard<std::mutex> lock(taskMutex);
        if (!taskQueue.empty())
        {
            task = taskQueue.front();
            taskQueue.pop();
            return true;
        }
        return false;
    }

    // 启动任务处理线程
    void FileQueueManager::startTaskProcessor()
    {
        shouldStop = false;
        taskProcessorThread = std::thread(&FileQueueManager::processTasks, this);
        logger_log_info("File task processor started");
    }

    // 停止任务处理线程
    void FileQueueManager::stopTaskProcessor()
    {
        if (taskProcessorThread.joinable())
        {
            {
                std::lock_guard<std::mutex> lock(taskMutex);
                shouldStop = true;
            }
            taskCondition.notify_all();
            taskProcessorThread.join();
            logger_log_info("File task processor stopped");
        }
    }

    // 任务处理线程函数
    void FileQueueManager::processTasks()
    {
        while (!shouldStop)
        {
            FileTask task;
            {
                std::unique_lock<std::mutex> lock(taskMutex);
                taskCondition.wait(lock, [this]
                                   { return !taskQueue.empty() || shouldStop; });

                if (shouldStop)
                    break;

                task = taskQueue.front();
                taskQueue.pop();
            }

            switch (task.type)
            {
            case TASK_CREATE_FILE:
                taskProcessor.processCreateFileTask(task);
                break;
            case TASK_UPDATE_FILE:
                taskProcessor.processUpdateFileTask(task);
                break;
            case TASK_CLOSE_FILE:
                taskProcessor.processCloseFileTask(task);
                break;
            case TASK_DELETE_FILE:
                taskProcessor.processDeleteFileTask(task);
                break;
            case TASK_UPDATE_TEMPLATE:
                taskProcessor.processUpdateTemplateTask(task);
                break;
            case TASK_ADD_WORKSHEET:
                taskProcessor.processAddWorksheetTask(task);
                break;
            case TASK_REMOVE_WORKSHEET:
                taskProcessor.processRemoveWorksheetTask(task);
                break;
            case TASK_RENAME_WORKSHEET:
                taskProcessor.processRenameWorksheetTask(task);
                break;
            case TASK_RENAME_FILE:
                taskProcessor.processRenameFileTask(task);
                break;
            case TASK_SHEET_DATA:
                taskProcessor.processSheetDataTask(task);
                break;
            default:
                logger_log_error("Unknown task type for file: %s", task.filename.c_str());
                break;
            }
        }
    }

    // 等待文件状态变化
    bool FileQueueManager::waitForFileStatus(const std::string &filename, FileStatus targetStatus1, FileStatus targetStatus2, int timeoutSeconds)
    {
        try
        {
            std::unique_lock<std::mutex> lock(statusMutex);
            auto now = std::chrono::system_clock::now();
            auto timeout = now + std::chrono::seconds(timeoutSeconds);

            // 检查当前状态是否已经符合要求
            auto it = fileStatusMap.find(filename);
            if (it != fileStatusMap.end())
            {
                FileStatus currentStatus = it->second.status;
                if (currentStatus == targetStatus1 || currentStatus == targetStatus2)
                {
                    logger_log_info("File %s already in target status %d", filename.c_str(), currentStatus);
                    return true;
                }
            }
            else
            {
                // 文件不存在于状态映射表中，对于更新操作，我们将其视为可以处理的状态
                // 因为新文件或没有状态记录的文件可以被更新
                logger_log_info("File %s not found in status map, treating as ready for processing", filename.c_str());
                return true;
            }

            // 等待状态变化或超时
            while (true)
            {
                // 等待条件变量或超时
                auto cvStatus = statusCondition.wait_until(lock, timeout);

                // 检查是否超时
                if (cvStatus == std::cv_status::timeout)
                {
                    logger_log_error("Timeout waiting for file %s to reach status %d or %d",
                                     filename.c_str(), targetStatus1, targetStatus2);
                    return false;
                }

                // 检查文件状态是否符合要求
                auto it = fileStatusMap.find(filename);
                if (it != fileStatusMap.end())
                {
                    FileStatus currentStatus = it->second.status;
                    if (currentStatus == targetStatus1 || currentStatus == targetStatus2)
                    {
                        logger_log_info("File %s reached target status %d", filename.c_str(), currentStatus);
                        return true;
                    }

                    // 如果文件出错，也返回失败
                    if (currentStatus == FILE_STATUS_ERROR)
                    {
                        logger_log_info("File %s is in error state, stop waiting for status change", filename.c_str());
                        return false;
                    }
                }
                else
                {
                    // 文件在等待过程中被删除，这种情况很少见，但我们需要处理
                    logger_log_info("File %s no longer in status map during waiting", filename.c_str());
                    return true;
                }

                // 检查是否应该停止
                if (shouldStop)
                {
                    logger_log_info("Task processor stopping, stop waiting for file status");
                    return false;
                }
            }
        }
        catch (const std::system_error &e)
        {
            logger_log_error("System error in waitForFileStatus for %s: %s", filename.c_str(), e.what());
            logger_log_error("Error code: %d, Error category: %s", e.code().value(), e.code().category().name());
            return false;
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in waitForFileStatus for %s: %s", filename.c_str(), e.what());
            return false;
        }
        catch (...)
        {
            logger_log_error("Unknown exception in waitForFileStatus for %s", filename.c_str());
            return false;
        }

        return false;
    }

    // 处理文件创建任务
    void FileTaskProcessor::processCreateFileTask(const FileTask &task)
    {
        logger_log_info("Processing create file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 等待文件状态变为创建状态或关闭状态
            bool isReady = filemanager::FileQueueManager::getInstance().waitForFileStatus(task.filename, FILE_STATUS_CREATED, FILE_STATUS_CLOSED, 30);
            if (!isReady)
            {
                logger_log_error("File %s is not ready for creating, timeout or error occurred",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Timeout waiting for file to be ready");
                return;
            }

            // 更新文件状态为处理中
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            // 调用filehandlers中的处理函数
            int res = filemanager::createfile(task.filename);
            if (res == 0)
            {
                // 更新文件状态为就绪
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_READY);
                logger_log_info("Successfully created file: %s",
                                filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
            else
            {
                // 更新文件状态为错误
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to create file");
                logger_log_error("Failed to create file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processCreateFileTask for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str(),
                             e.what());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processCreateFileTask for %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing create file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理文件更新任务
    void FileTaskProcessor::processUpdateFileTask(const FileTask &task)
    {
        logger_log_info("Processing update file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            bool isReady = filemanager::FileQueueManager::getInstance().waitForFileStatus(task.filename, FILE_STATUS_READY, FILE_STATUS_CLOSED, 30);
            if (!isReady)
            {
                logger_log_error("File %s is not ready for updating, timeout or error occurred",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Timeout waiting for file to be ready");
                return;
            }

            // 更新文件状态为处理中
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            if (!task.taskData)
            {
                logger_log_error("Update file task has no data for file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Update file task has no data");
                return;
            }

            // 调用filehandlers中的处理函数
            int res = filemanager::fileupdate(task.taskData);
            if (res == 0)
            {
                // 更新文件状态为就绪
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_READY);
                logger_log_info("Successfully updated file: %s",
                                filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
            else
            {
                // 更新文件状态为错误
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to update file");
                logger_log_error("Failed to update file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processUpdateFileTask for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str(),
                             e.what());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processUpdateFileTask for %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing update file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理关闭文件任务
    void FileTaskProcessor::processCloseFileTask(const FileTask &task)
    {
        logger_log_info("Processing close file task: %s", task.filename.c_str());

        // 更新文件状态为处理中
        filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

        // 删除文件信息记状态记录
        int res = filemanager::fileclose(task.taskData);

        // 更新文件状态为已关闭
        filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_CLOSED);

        logger_log_info("Finished processing close file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理删除文件任务
    void FileTaskProcessor::processDeleteFileTask(const FileTask &task)
    {
        logger_log_info("Processing delete file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 更新文件状态为处理中
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            // 获取文件的绝对路径
            rtl::OUString fileNameOUString = convertStringToOUString(task.filename.c_str());
            rtl::OUString absoluteFilePath;
            filemanager::getAbsolutePath(fileNameOUString, absoluteFilePath);

            // 将OUString路径转换为std::string
            std::string filePath = filemanager::convertOUStringToString(absoluteFilePath);

            // 首先判断是否文件已经加载
            FileInfo fileInfo = filemanager::FileQueueManager::getInstance().getFileInfo(task.filename);
            uno::Reference<sheet::XSpreadsheetDocument> xDoc;
            if (!fileInfo.xComponent.is())
            {
                xDoc = loadSpreadsheetDocument(absoluteFilePath, fileInfo.xComponent);
                if (!fileInfo.xComponent.is())
                {
                    logger_log_error("processDeleteFileTask -- Failed to load document: %s", task.filename.c_str());
                    return;
                }
            }
            else
            {
                xDoc = uno::Reference<sheet::XSpreadsheetDocument>(fileInfo.xComponent, uno::UNO_QUERY); // 类型转换
            }
            uno::Reference<lang::XComponent> xComp(fileInfo.xComponent); // 声明并初始化 xComp
            closeDocument(xComp);                                        // 关闭文档
            // 删除文件
            if (unlink(filePath.c_str()) == 0)
            {
                logger_log_info("Successfully deleted file: %s",
                                filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
            else
            {
                logger_log_error("Failed to delete file: %s, errno: %d",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str(),
                                 errno);
            }
            // 从状态映射表中移除文件
            filemanager::FileQueueManager::getInstance().eraseFileStatus(task.filename);
        }
        catch (const std::system_error &e)
        {
            logger_log_error("System error in processDeleteFileTask for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str(),
                             e.what());
            logger_log_error("Error code: %d, Error category: %s", e.code().value(), e.code().category().name());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("System error: ") + e.what());
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processDeleteFileTask for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str(),
                             e.what());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processDeleteFileTask for %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing delete file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理更新模板任务
    void FileTaskProcessor::processUpdateTemplateTask(const FileTask &task)
    {
        logger_log_info("Processing update template task");
        // 更新文件状态为处理中
        filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_PROCESSING);
        try
        {
            //直接重新初始化文件
            int res = fileTempleteChange(task.filename);
            if(res != 0)
            {
                logger_log_error("Failed to update template: %s", task.filename.c_str());
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Failed to update template: ") + task.filename);
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception while updating template: %s", e.what());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception while updating template");
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + "Unknown exception");
        }
        filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_READY);
        logger_log_info("Finished processing update template task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理新增工作表任务
    void FileTaskProcessor::processAddWorksheetTask(const FileTask &task)
    {
        logger_log_info("Processing add worksheet task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            bool isReady = filemanager::FileQueueManager::getInstance().waitForFileStatus(task.filename, FILE_STATUS_READY, FILE_STATUS_CLOSED, 30);
            if (!isReady)
            {
                logger_log_error("File %s is not ready for adding worksheet, timeout or error occurred",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Timeout waiting for file to be ready");
                return;
            }

            // 更新文件状态为处理中
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            if (!task.taskData)
            {
                logger_log_error("Add worksheet task has no data for file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Add worksheet task has no data");
                return;
            }

            // 调用filehandlers中的处理函数
            int res = filemanager::worksheetAdd(task.taskData);
            if (res == 0)
            {
                // 更新文件状态为就绪
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_READY);
                logger_log_info("Successfully added worksheet for file: %s",
                                filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
            else
            {
                // 更新文件状态为错误
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to add worksheet");
                logger_log_error("Failed to add worksheet for file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processAddWorksheetTask for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str(),
                             e.what());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processAddWorksheetTask for %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing add worksheet task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理删除工作表任务
    void FileTaskProcessor::processRemoveWorksheetTask(const FileTask &task)
    {
        logger_log_info("Processing remove worksheet task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            bool isReady = filemanager::FileQueueManager::getInstance().waitForFileStatus(task.filename, FILE_STATUS_READY, FILE_STATUS_CLOSED, 30);
            if (!isReady)
            {
                logger_log_error("File %s is not ready for removing worksheet, timeout or error occurred",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Timeout waiting for file to be ready");
                return;
            }

            // 更新文件状态为处理中
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            if (!task.taskData)
            {
                logger_log_error("Remove worksheet task has no data for file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Remove worksheet task has no data");
                return;
            }

            // 调用filehandlers中的处理函数
            int res = filemanager::worksheetRemove(task.taskData);
            if (res == 0)
            {
                // 更新文件状态为就绪
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_READY);
                logger_log_info("Successfully removed worksheet for file: %s",
                                filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
            else
            {
                // 更新文件状态为错误
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to remove worksheet");
                logger_log_error("Failed to remove worksheet for file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processRemoveWorksheetTask for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str(),
                             e.what());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processRemoveWorksheetTask for %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing remove worksheet task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理重命名工作表任务
    void FileTaskProcessor::processRenameWorksheetTask(const FileTask &task)
    {
        logger_log_info("Processing rename worksheet task: %s", task.filename.c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            bool isReady = filemanager::FileQueueManager::getInstance().waitForFileStatus(task.filename, FILE_STATUS_READY, FILE_STATUS_CLOSED, 30);
            if (!isReady)
            {
                logger_log_error("File %s is not ready for renaming worksheet, timeout or error occurred", task.filename.c_str());
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Timeout waiting for file to be ready");
                return;
            }

            // 更新文件状态为处理中
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            // 确保taskData不为空
            if (!task.taskData)
            {
                logger_log_error("Rename worksheet task has no data");
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Rename worksheet task has no data");
                return;
            }

            // 调用filehandlers中的处理函数
            int res = filemanager::worksheetRename(task.taskData);
            if (res == 0)
            {
                // 更新文件状态为就绪
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_READY);
                logger_log_info("Successfully renamed worksheet for file: %s", task.filename.c_str());
                return;
            }
            else
            {
                // 更新文件状态为错误
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to rename worksheet");
                logger_log_error("Failed to rename worksheet for file: %s", task.filename.c_str());
                return;
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processRenameWorksheetTask for %s: %s", task.filename.c_str(), e.what());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
            return;
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processRenameWorksheetTask for %s", task.filename.c_str());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
            return;
        }

        logger_log_info("Finished processing rename worksheet task: %s", task.filename.c_str());
    }
    // 处理重命名文件任务
    void FileTaskProcessor::processRenameFileTask(const FileTask &task)
    {
        logger_log_info("Processing rename file task: %s", ensureOdsExtension(task.filename).c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            bool isReady = filemanager::FileQueueManager::getInstance().waitForFileStatus(task.filename, FILE_STATUS_READY, FILE_STATUS_CLOSED, 30);
            if (!isReady)
            {
                logger_log_error("File %s is not ready for renaming, timeout or error occurred", ensureOdsExtension(task.filename).c_str());
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Timeout waiting for file to be ready");
                return;
            }

            // 更新文件状态为处理中
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            // 获取新文件名
            std::string newFilename;
            if (task.taskData)
            {
                cJSON *newFilenameItem = cJSON_GetObjectItem(task.taskData, "newFilename");
                if (newFilenameItem && cJSON_IsString(newFilenameItem))
                {
                    newFilename = std::string(newFilenameItem->valuestring);
                }
            }

            if (newFilename.empty())
            {
                logger_log_error("New filename is empty for file: %s", ensureOdsExtension(task.filename).c_str());
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "New filename is empty");
                return;
            }

            // 获取旧文件和新文件的绝对路径
            rtl::OUString oldFileNameOUString = convertStringToOUString(ensureOdsExtension(task.filename).c_str());
            rtl::OUString newFileNameOUString = convertStringToOUString(ensureOdsExtension(newFilename).c_str());

            rtl::OUString oldAbsoluteFilePath, newAbsoluteFilePath;
            filemanager::getAbsolutePath(oldFileNameOUString, oldAbsoluteFilePath);
            filemanager::getAbsolutePath(newFileNameOUString, newAbsoluteFilePath);

            // 将OUString路径转换为std::string
            std::string oldFilePath = filemanager::convertOUStringToString(oldAbsoluteFilePath);
            std::string newFilePath = filemanager::convertOUStringToString(newAbsoluteFilePath);

            // 检查源文件是否存在
            if (access(oldFilePath.c_str(), F_OK) != 0)
            {
                logger_log_error("Source file does not exist: %s", oldFilePath.c_str());
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Source file does not exist");
                return;
            }

            // 重命名文件
            if (rename(oldFilePath.c_str(), newFilePath.c_str()) == 0)
            {
                // 通过公有接口实现文件状态映射表的重命名逻辑
                FileInfo fileInfo = filemanager::FileQueueManager::getInstance().getFileInfo(task.filename);
                if (fileInfo.filename == task.filename) {
                    filemanager::FileQueueManager::getInstance().removeFileInfo(task.filename);
                    fileInfo.filename = newFilename;
                    filemanager::FileQueueManager::getInstance().addFileStatus(fileInfo);
                }
                filemanager::FileQueueManager::getInstance().updateFileStatus(newFilename, FILE_STATUS_READY);
                logger_log_info("Successfully renamed file from %s to %s", oldFilePath.c_str(), newFilePath.c_str());
            }
            else
            {
                logger_log_error("Failed to rename file from %s to %s, errno: %d", oldFilePath.c_str(), newFilePath.c_str(), errno);
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to rename file");
                return;
            }
        }
        catch (const std::system_error &e)
        {
            logger_log_error("System error in processRenameFileTask for %s: %s", ensureOdsExtension(task.filename).c_str(), e.what());
            logger_log_error("Error code: %d, Error category: %s", e.code().value(), e.code().category().name());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("System error: ") + e.what());
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processRenameFileTask for %s: %s", ensureOdsExtension(task.filename).c_str(), e.what());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processRenameFileTask for %s", ensureOdsExtension(task.filename).c_str());
            filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing rename file task: %s", ensureOdsExtension(task.filename).c_str());
    }

    // 处理获取工作表数据任务
    void FileTaskProcessor::processSheetDataTask(const FileTask &task)
    {
        logger_log_info("Processing sheet data task");

        try
        {
            // 确保taskData不为空
            if (!task.taskData)
            {
                logger_log_error("Sheet data task has no data");
                return;
            }

            // 调用filehandlers中的处理函数
            cJSON *results = cJSON_CreateObject();
            int res = filemanager::querysheetdata(task.taskData, results);
            if (res == 0)
            {
                logger_log_info("Successfully processed sheet data task");
            }
            else
            {
                logger_log_error("Failed to process sheet data task");
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processSheetDataTask: %s", e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processSheetDataTask");
        }

        logger_log_info("Finished processing sheet data task");
    }

    // 移除文件信息
    void FileQueueManager::removeFileInfo(const std::string &filename)
    {
    FileInfo info = getFileInfo(filename);
    if (info.filename == filename) {
        if (info.xComponent.is()) {
            try {
                closeDocument(info.xComponent);
            } catch (...) {
                logger_log_error("Exception when closing xComponent for file: %s", filename.c_str());
            }
        }
        eraseFileStatus(filename);
        logger_log_info("Removed file info for: %s", filename.c_str());
    }
    }

    void FileQueueManager::eraseFileStatus(const std::string &filename)
    {
    std::lock_guard<std::mutex> lock(statusMutex);
    fileStatusMap.erase(filename);
    }

     // 获取队列和错误统计信息
    QueueStats FileQueueManager::getQueueStats() const {
        QueueStats stats{};
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            for (const auto& kv : fileStatusMap) {
                switch (kv.second.status) {
                    case FILE_STATUS_ERROR: stats.errorCount++; break;
                    case FILE_STATUS_READY: stats.readyCount++; break;
                    case FILE_STATUS_PROCESSING: stats.processingCount++; break;
                    case FILE_STATUS_CLOSED: stats.closedCount++; break;
                    default: break;
                }
            }
        }
        {
            std::lock_guard<std::mutex> lock(taskMutex);
            stats.queueLength = taskQueue.size();
        }
        return stats;
    }

} // namespace filemanager