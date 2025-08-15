#include "filequeue.h"
#include "filehandlers.h"
#include "cache.h"
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
        fileStatusMap[filename] = info;

        logger_log_info("Added file status: %s - %d",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(filename.c_str())).c_str(),
                        status);
    }

    // 添加文件状态（完整信息）
    void FileQueueManager::addFileStatus(const FileInfo &fileInfo)
    {
        std::lock_guard<std::mutex> lock(statusMutex);
        fileStatusMap[fileInfo.filename] = fileInfo;
        logger_log_info("Added file status: %s - %d with mtime: %ld",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(fileInfo.filename.c_str())).c_str(),
                        fileInfo.status,
                        (long)fileInfo.lastModified);
    }

    // 更新文件状态
    void FileQueueManager::updateFileStatus(const std::string &filename, FileStatus status, const std::string &errorMsg)
    {
        try
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            auto it = fileStatusMap.find(filename);
            if (it != fileStatusMap.end())
            {
                it->second.status = status;
                it->second.lastModified = std::time(nullptr);
                if (!errorMsg.empty())
                {
                    it->second.errorMessage = errorMsg;
                }
                logger_log_info("Updated file status: %s - %d",
                                filemanager::convertOUStringToString(filemanager::convertStringToOUString(filename.c_str())).c_str(),
                                status);
            }
            else
            {
                // 如果文件不存在，直接添加新条目（避免调用addFileStatus导致死锁）
                FileInfo info;
                info.filename = filename;
                info.status = status;
                info.lastModified = std::time(nullptr);
                info.errorMessage = errorMsg;
                fileStatusMap[filename] = info;
                logger_log_info("Added file status: %s - %d",
                                filemanager::convertOUStringToString(filemanager::convertStringToOUString(filename.c_str())).c_str(),
                                status);
            }

            // 通知等待状态变化的线程
            statusCondition.notify_all();
        }
        catch (const std::system_error &e)
        {
            logger_log_error("System error in updateFileStatus for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(filename.c_str())).c_str(),
                             e.what());
            logger_log_error("Error code: %d, Error category: %s", e.code().value(), e.code().category().name());
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in updateFileStatus for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(filename.c_str())).c_str(),
                             e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in updateFileStatus for %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(filename.c_str())).c_str());
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
        return info;
    }

    // 获取所有文件状态的副本
    std::unordered_map<std::string, FileInfo> FileQueueManager::getFileStatusMapCopy() const
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

    // 检查模板是否正在加载
    bool FileQueueManager::isTemplateLoading() const
    {
        try
        {
            // 使用新的缓存管理器检查模板加载状态
            return filemanager::TemplateCacheManager::getInstance().isTemplateLoading();
        }
        catch (...)
        {
            logger_log_error("Exception in isTemplateLoading, returning false by default");
            return false;
        }
    }

    void FileQueueManager::setTemplateLoading(bool loading)
    {
        try
        {
            // 使用新的缓存管理器设置模板加载状态
            filemanager::TemplateCacheManager::getInstance().setTemplateLoading(loading);
            logger_log_info("Template loading status set to: %s", loading ? "true" : "false");
        }
        catch (...)
        {
            logger_log_error("Exception in setTemplateLoading, failed to set status to: %s", loading ? "true" : "false");
        }
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
        logger_log_info("File task processor thread started");

        while (!shouldStop)
        {
            std::unique_lock<std::mutex> lock(taskMutex);
            taskCondition.wait(lock, [this]
                               { return !taskQueue.empty() || shouldStop; });

            if (shouldStop)
            {
                break;
            }

            if (!taskQueue.empty())
            {
                FileTask task = taskQueue.front();
                taskQueue.pop();
                lock.unlock();

                try
                {
                    // 根据任务类型处理任务
                    switch (task.type)
                    {
                    case TASK_CREATE_FILE:
                        processCreateFileTask(task);
                        break;
                    case TASK_UPDATE_FILE:
                        processUpdateFileTask(task);
                        break;
                    case TASK_CLOSE_FILE:
                        processCloseFileTask(task);
                        break;
                    case TASK_DELETE_FILE:
                        processDeleteFileTask(task);
                        break;
                    case TASK_UPDATE_TEMPLATE:
                        processUpdateTemplateTask(task);
                        break;
                    case TASK_ADD_WORKSHEET:
                        processAddWorksheetTask(task);
                        break;
                    case TASK_REMOVE_WORKSHEET:
                        processRemoveWorksheetTask(task);
                        break;
                    case TASK_RENAME_WORKSHEET:
                        processRenameWorksheetTask(task);
                        break;
                    case TASK_RENAME_FILE:
                        processRenameFileTask(task);
                        break;
                    case TASK_SHEET_DATA:
                        processSheetDataTask(task);
                        break;
                    default:
                        logger_log_error("Unknown task type: %d", task.type);
                        break;
                    }
                }
                catch (const std::system_error &e)
                {
                    logger_log_error("System error in processTasks: %s", e.what());
                    logger_log_error("Error code: %d, Error category: %s", e.code().value(), e.code().category().name());
                }
                catch (const std::exception &e)
                {
                    logger_log_error("Exception in processTasks: %s", e.what());
                }
                catch (...)
                {
                    logger_log_error("Unknown exception in processTasks");
                }
            }
        }

        logger_log_info("File task processor thread stopped");
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

    // 处理创建文件任务
    void FileQueueManager::processCreateFileTask(const FileTask &task)
    {
        logger_log_info("Processing create file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 等待文件状态变为创建状态或关闭状态
            bool isReady = waitForFileStatus(task.filename, FILE_STATUS_CREATED, FILE_STATUS_CLOSED, 30);
            if (!isReady)
            {
                logger_log_error("File %s is not ready for creating, timeout or error occurred",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Timeout waiting for file to be ready");
                return;
            }

            // 更新文件状态为处理中
            updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            // 调用filehandlers中的处理函数
            int res = filemanager::createfile(task.filename);
            if (res == 0)
            {
                // 更新文件状态为就绪
                updateFileStatus(task.filename, FILE_STATUS_READY);
                logger_log_info("Successfully created file: %s",
                                filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
            else
            {
                // 更新文件状态为错误
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to create file");
                logger_log_error("Failed to create file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processCreateFileTask for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str(),
                             e.what());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processCreateFileTask for %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing create file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理更新文件任务
    void FileQueueManager::processUpdateFileTask(const FileTask &task)
    {
        logger_log_info("Processing update file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            bool isReady = waitForFileStatus(task.filename, FILE_STATUS_READY, FILE_STATUS_CLOSED, 30);
            if (!isReady)
            {
                logger_log_error("File %s is not ready for updating, timeout or error occurred",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Timeout waiting for file to be ready");
                return;
            }

            // 更新文件状态为处理中
            updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            if (!task.taskData)
            {
                logger_log_error("Update file task has no data for file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Update file task has no data");
                return;
            }

            // 调用filehandlers中的处理函数
            int res = filemanager::fileupdate(task.taskData);
            if (res == 0)
            {
                // 更新文件状态为就绪
                updateFileStatus(task.filename, FILE_STATUS_READY);
                logger_log_info("Successfully updated file: %s",
                                filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
            else
            {
                // 更新文件状态为错误
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to update file");
                logger_log_error("Failed to update file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processUpdateFileTask for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str(),
                             e.what());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processUpdateFileTask for %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing update file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理关闭文件任务
    void FileQueueManager::processCloseFileTask(const FileTask &task)
    {
        logger_log_info("Processing close file task: %s", task.filename.c_str());

        // 更新文件状态为处理中
        updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

        // 模拟处理时间
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        // 更新文件状态为已关闭
        updateFileStatus(task.filename, FILE_STATUS_CLOSED);

        logger_log_info("Finished processing close file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理删除文件任务
    void FileQueueManager::processDeleteFileTask(const FileTask &task)
    {
        logger_log_info("Processing delete file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 更新文件状态为处理中
            updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            // 从状态映射表中移除文件
            {
                std::lock_guard<std::mutex> lock(statusMutex);
                fileStatusMap.erase(task.filename);
            }

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
                    return ;
                }
            }
            else
            {
                xDoc = uno::Reference<sheet::XSpreadsheetDocument>(fileInfo.xComponent, uno::UNO_QUERY); // 类型转换
            }
            uno::Reference<lang::XComponent> xComp(fileInfo.xComponent); // 声明并初始化 xComp
            closeDocument(xComp); // 关闭文档
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
        }
        catch (const std::system_error &e)
        {
            logger_log_error("System error in processDeleteFileTask for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str(),
                             e.what());
            logger_log_error("Error code: %d, Error category: %s", e.code().value(), e.code().category().name());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("System error: ") + e.what());
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processDeleteFileTask for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str(),
                             e.what());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processDeleteFileTask for %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing delete file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理更新模板任务
    void FileQueueManager::processUpdateTemplateTask(const FileTask &task)
    {
        logger_log_info("Processing update template task");

        try
        {
            // 确保taskData不为空
            if (!task.taskData)
            {
                logger_log_error("Update template task has no data");
                return;
            }

            updateAllFilesWithTemplateData(task.taskData);
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception while updating template: %s", e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception while updating template");
        }

        logger_log_info("Finished processing update template task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理新增工作表任务
    void FileQueueManager::processAddWorksheetTask(const FileTask &task)
    {
        logger_log_info("Processing add worksheet task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            bool isReady = waitForFileStatus(task.filename, FILE_STATUS_READY, FILE_STATUS_CLOSED, 30);
            if (!isReady)
            {
                logger_log_error("File %s is not ready for adding worksheet, timeout or error occurred",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Timeout waiting for file to be ready");
                return;
            }

            // 更新文件状态为处理中
            updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            if (!task.taskData)
            {
                logger_log_error("Add worksheet task has no data for file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Add worksheet task has no data");
                return;
            }

            // 调用filehandlers中的处理函数
            int res = filemanager::worksheetAdd(task.taskData);
            if (res == 0)
            {
                // 更新文件状态为就绪
                updateFileStatus(task.filename, FILE_STATUS_READY);
                logger_log_info("Successfully added worksheet for file: %s",
                                filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
            else
            {
                // 更新文件状态为错误
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to add worksheet");
                logger_log_error("Failed to add worksheet for file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processAddWorksheetTask for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str(),
                             e.what());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processAddWorksheetTask for %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing add worksheet task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理删除工作表任务
    void FileQueueManager::processRemoveWorksheetTask(const FileTask &task)
    {
        logger_log_info("Processing remove worksheet task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            bool isReady = waitForFileStatus(task.filename, FILE_STATUS_READY, FILE_STATUS_CLOSED, 30);
            if (!isReady)
            {
                logger_log_error("File %s is not ready for removing worksheet, timeout or error occurred",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Timeout waiting for file to be ready");
                return;
            }

            // 更新文件状态为处理中
            updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            if (!task.taskData)
            {
                logger_log_error("Remove worksheet task has no data for file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Remove worksheet task has no data");
                return;
            }

            // 调用filehandlers中的处理函数
            int res = filemanager::worksheetRemove(task.taskData);
            if (res == 0)
            {
                // 更新文件状态为就绪
                updateFileStatus(task.filename, FILE_STATUS_READY);
                logger_log_info("Successfully removed worksheet for file: %s",
                                filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
            else
            {
                // 更新文件状态为错误
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to remove worksheet");
                logger_log_error("Failed to remove worksheet for file: %s",
                                 filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processRemoveWorksheetTask for %s: %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str(),
                             e.what());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processRemoveWorksheetTask for %s",
                             filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing remove worksheet task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理重命名工作表任务
    void FileQueueManager::processRenameWorksheetTask(const FileTask &task)
    {
        logger_log_info("Processing rename worksheet task: %s", task.filename.c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            bool isReady = waitForFileStatus(task.filename, FILE_STATUS_READY, FILE_STATUS_CLOSED, 30);
            if (!isReady)
            {
                logger_log_error("File %s is not ready for renaming worksheet, timeout or error occurred", task.filename.c_str());
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Timeout waiting for file to be ready");
                return;
            }

            // 更新文件状态为处理中
            updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            // 确保taskData不为空
            if (!task.taskData)
            {
                logger_log_error("Rename worksheet task has no data");
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Rename worksheet task has no data");
                return;
            }

            // 调用filehandlers中的处理函数
            int res = filemanager::worksheetRename(task.taskData);
            if (res == 0)
            {
                // 更新文件状态为就绪
                updateFileStatus(task.filename, FILE_STATUS_READY);
                logger_log_info("Successfully renamed worksheet for file: %s", task.filename.c_str());
                return;
            }
            else
            {
                // 更新文件状态为错误
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to rename worksheet");
                logger_log_error("Failed to rename worksheet for file: %s", task.filename.c_str());
                return;
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processRenameWorksheetTask for %s: %s", task.filename.c_str(), e.what());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
            return;
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processRenameWorksheetTask for %s", task.filename.c_str());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
            return;
        }

        logger_log_info("Finished processing rename worksheet task: %s", task.filename.c_str());
    }
    // 处理重命名文件任务
    void FileQueueManager::processRenameFileTask(const FileTask &task)
    {
        logger_log_info("Processing rename file task: %s",ensureOdsExtension(task.filename).c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            bool isReady = waitForFileStatus(task.filename, FILE_STATUS_READY, FILE_STATUS_CLOSED, 30);
            if (!isReady)
            {
                logger_log_error("File %s is not ready for renaming, timeout or error occurred",ensureOdsExtension(task.filename).c_str());
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Timeout waiting for file to be ready");
                return;
            }

            // 更新文件状态为处理中
            updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

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
                logger_log_error("New filename is empty for file: %s",ensureOdsExtension(task.filename).c_str());
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "New filename is empty");
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
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Source file does not exist");
                return;
            }

            // 重命名文件
            if (rename(oldFilePath.c_str(), newFilePath.c_str()) == 0)
            {
                // 更新文件状态映射表中的文件名
                {
                    std::lock_guard<std::mutex> lock(statusMutex);
                    auto it = fileStatusMap.find(task.filename);
                    if (it != fileStatusMap.end())
                    {
                        FileInfo fileInfo = it->second;
                        fileInfo.filename = newFilename;
                        fileStatusMap.erase(it);
                        fileStatusMap[newFilename] = fileInfo;
                    }
                }

                updateFileStatus(newFilename, FILE_STATUS_READY);
                logger_log_info("Successfully renamed file from %s to %s",oldFilePath.c_str(), newFilePath.c_str());
            }
            else
            {
                logger_log_error("Failed to rename file from %s to %s, errno: %d",oldFilePath.c_str(),newFilePath.c_str(),errno);
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to rename file");
                return;
            }
        }
        catch (const std::system_error &e)
        {
            logger_log_error("System error in processRenameFileTask for %s: %s",ensureOdsExtension(task.filename).c_str(),e.what());
            logger_log_error("Error code: %d, Error category: %s", e.code().value(), e.code().category().name());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("System error: ") + e.what());
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processRenameFileTask for %s: %s",ensureOdsExtension(task.filename).c_str(),e.what());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processRenameFileTask for %s",ensureOdsExtension(task.filename).c_str());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing rename file task: %s",ensureOdsExtension(task.filename).c_str());
    }

    // 处理获取工作表数据任务
    void FileQueueManager::processSheetDataTask(const FileTask &task)
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
            int res = filemanager::querysheetdata(task.taskData,results);
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
} // namespace filemanager