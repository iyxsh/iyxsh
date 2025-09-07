#include "../filemanager/filequeue.h"
#include "filehandlers.h"
#include "template_index_cache.h" // 如需直接使用索引缓存，可包含此头文件
#include "utils.h"
#include "../logger/logger.h"
#include <iostream>
#include <chrono>
#include <stdio.h>
#include <fcntl.h>
#include "spreadsheet.h"
#include "LibreOfficeService.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <errno.h>
#endif
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
        info.xDoc = nullptr; // 新增字段，初始为空
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
    void FileQueueManager::updateFilexDoc(const std::string &filename, uno::Reference<sheet::XSpreadsheetDocument> &xDoc, const std::string &errorMsg)
    {
        std::lock_guard<std::mutex> lock(statusMutex);
        auto it = fileStatusMap.find(filename);
        if (it != fileStatusMap.end())
        {
            it->second.xDoc = xDoc;
        }
        else
        {
            // 如果文件不存在于状态映射表中，则添加新条目
            FileInfo info;
            info.filename = filename;
            info.status = FILE_STATUS_READY;
            info.errorMessage = errorMsg;
            info.lastModified = std::time(nullptr);
            info.xDoc = xDoc;
            info.refCount = 0;
            fileStatusMap[filename] = info;
        }
    }

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
            info.xDoc = nullptr;
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
        info.status = FILE_STATUS_NOT_FOUND;
        info.lastModified = 0;
        info.xDoc = nullptr;
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
        try
        {
            shouldStop = false;
            taskProcessorThread = std::thread(&FileQueueManager::processTasks, this);
            logger_log_info("File task processor started");
        }
        catch (const std::system_error& e)
        {
            logger_log_error("System error creating task processor thread: %s", e.what());
            logger_log_error("Error code: %d, Category: %s", e.code().value(), e.code().category().name());
        }
        catch (const std::exception& e)
        {
            logger_log_error("Exception creating task processor thread: %s", e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown error creating task processor thread");
        }
    }

    // 停止任务处理线程
    void FileQueueManager::stopTaskProcessor()
    {
        logger_log_info("Stopping task processor...");
        
        // 设置停止标志并通知条件变量，避免在FreeBSD上出现线程队列错误
        {   
            // 首先锁定任务互斥量
            std::unique_lock<std::mutex> lock(taskMutex);
            
            // 设置停止标志
            shouldStop = true;
            
            // 在持有锁的情况下通知条件变量
            // 这确保了通知不会在设置shouldStop之前发出，避免竞态条件
            logger_log_info("Notifying all waiting threads and setting stop flag...");
            taskCondition.notify_all();
        }
        // 锁在此处自动释放，以避免在join期间持有锁
        
        // 关闭所有未关闭的文件
        logger_log_info("Closing all open files...");
        
        // 复制文件状态映射，避免在锁定状态下进行耗时操作
        std::unordered_map<std::string, FileInfo> tempFileStatusMap;
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            tempFileStatusMap = fileStatusMap;
        }
        
        int closedFileCount = 0;
        for (const auto& kv : tempFileStatusMap)
        {
            const std::string& filename = kv.first;
            const FileInfo& fileInfo = kv.second;
            
            // 检查文件是否未关闭（状态不是CLOSED）
            if (fileInfo.status != FILE_STATUS_CLOSED)
            {
                try
                {
                    // 尝试关闭文件
                    std::string filePath = filemanager::convertOUStringToString(
                        filemanager::convertStringToOUString(ensureOdsExtension(filename).c_str()));
                    
                    // 检查文件是否存在
                    if (access(filePath.c_str(), F_OK) == 0)
                    {
                        // 通过DocumentManager关闭文件
                        std::string docId = filemanager::DocumentManager::getInstance().openDocument(
                            filePath, filemanager::DocumentType::SPREADSHEET);
                        if (!docId.empty())
                        {
                            filemanager::DocumentManager::getInstance().closeDocument(docId);
                            closedFileCount++;
                            logger_log_info("Closed file: %s", filename.c_str());
                        }
                    }
                    
                    // 更新文件状态为已关闭
                    updateFileStatus(filename, FILE_STATUS_CLOSED);
                }
                catch (const std::exception& e)
                {
                    logger_log_error("Failed to close file %s: %s", filename.c_str(), e.what());
                }
                catch (...)
                {
                    logger_log_error("Unknown error when closing file %s", filename.c_str());
                }
            }
        }
        
        logger_log_info("Closed %d files during task processor shutdown", closedFileCount);
        
        // 等待任务处理线程结束
        try
        {
            if (taskProcessorThread.joinable())
            {
                logger_log_info("Waiting for task processor thread to join...");
                taskProcessorThread.join();
                logger_log_info("Task processor thread joined successfully");
            }
        }
        catch (const std::system_error& e)
        {
            logger_log_error("System error when joining thread: %s", e.what());
            logger_log_error("Error code: %d, Category: %s", e.code().value(), e.code().category().name());
            // 记录错误但继续执行，避免程序崩溃
        }
        catch (const std::exception& e)
        {
            logger_log_error("Exception when joining thread: %s", e.what());
            // 记录错误但继续执行，避免程序崩溃
        }
        catch (...)
        {
            logger_log_error("Unknown error when joining thread");
            // 记录错误但继续执行，避免程序崩溃
        }
        
        logger_log_info("File task processor stopped");
    }

    // 任务处理线程函数
    void FileQueueManager::processTasks()
    {
        logger_log_info("Task processor thread started");
        
        try
        {
            while (true)
            {
                FileTask task;
                bool taskAvailable = false;
                
                {
                    std::unique_lock<std::mutex> lock(taskMutex);
                    
                    // 检查是否应该停止（在获取锁之后立即检查）
                    if (shouldStop)
                    {
                        logger_log_info("Task processor received stop signal, exiting...");
                        break;
                    }
                    
                    // 等待任务或停止信号
                    // 使用wait的谓词形式确保只在条件满足时返回，避免虚假唤醒
                    taskCondition.wait(lock, [this] { return !taskQueue.empty() || shouldStop; });
                    
                    // 再次检查是否应该停止（防止虚假唤醒）
                    if (shouldStop)
                    {
                        logger_log_info("Task processor received stop signal after wait, exiting...");
                        break;
                    }
                    
                    // 如果队列非空，获取任务
                    if (!taskQueue.empty())
                    {
                        task = taskQueue.front();
                        taskQueue.pop();
                        taskAvailable = true;
                    }
                }
                
                // 如果有任务要处理，在锁外处理任务
                if (taskAvailable)
                {
                    try
                    {
                        logger_log_info("Processing task type %d for file: %s", task.type, task.filename.c_str());
                        
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
                    catch (const std::exception& e)
                    {
                        logger_log_error("Exception processing task for file %s: %s", task.filename.c_str(), e.what());
                        // 更新文件状态为错误
                        updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Task processing error: ") + e.what());
                    }
                    catch (...)
                    {
                        logger_log_error("Unknown exception processing task for file: %s", task.filename.c_str());
                        // 更新文件状态为错误
                        updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown task processing error");
                    }
                }
            }
        }
        catch (const std::system_error& e)
        {
            logger_log_error("System error in task processor thread: %s", e.what());
            logger_log_error("Error code: %d, Category: %s", e.code().value(), e.code().category().name());
        }
        catch (const std::exception& e)
        {
            logger_log_error("Exception in task processor thread: %s", e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown error in task processor thread");
        }
        
        logger_log_info("Task processor thread exited");
    }

    // 等待文件状态变化
    bool FileQueueManager::waitForFileStatus(const std::string &filename, const std::vector<FileStatus>& targetStatuses, int timeoutSeconds)
    {
        try
        {
            std::unique_lock<std::mutex> lock(statusMutex);
            auto now = std::chrono::system_clock::now();
            auto timeout = now + std::chrono::seconds(timeoutSeconds);

            // 检查目标状态列表是否为空
            if (targetStatuses.empty())
            {
                logger_log_error("waitForFileStatus called with empty target status list");
                return false;
            }

            // 检查当前状态是否已经符合要求
            auto it = fileStatusMap.find(filename);
            if (it != fileStatusMap.end())
            {
                FileStatus currentStatus = it->second.status;
                // 检查当前状态是否在目标状态列表中
                for (const auto& targetStatus : targetStatuses)
                {
                    if (currentStatus == targetStatus)
                    {
                        logger_log_info("File %s already in target status %d", filename.c_str(), currentStatus);
                        return true;
                    }
                }
            }
            else
            {
                // 文件不存在于状态映射表中，对于更新操作，我们将其视为可以处理的状态
                // 因为新文件或没有状态记录的文件可以被更新
                logger_log_info("File %s not found in status map, treating as ready for processing", filename.c_str());
                return true;
            }

            // 构建目标状态字符串用于日志
            std::string targetStatusStr;
            for (size_t i = 0; i < targetStatuses.size(); ++i)
            {
                if (i > 0) targetStatusStr += ", ";
                targetStatusStr += std::to_string(static_cast<int>(targetStatuses[i]));
            }

            // 等待状态变化或超时
            while (true)
            {
                // 等待条件变量或超时
                auto cvStatus = statusCondition.wait_until(lock, timeout);

                // 检查是否超时
                if (cvStatus == std::cv_status::timeout)
                {
                    logger_log_error("Timeout waiting for file %s to reach status(es): %s",
                                     filename.c_str(), targetStatusStr.c_str());
                    return false;
                }

                // 检查文件状态是否符合要求
                auto it = fileStatusMap.find(filename);
                if (it != fileStatusMap.end())
                {
                    FileStatus currentStatus = it->second.status;
                    // 检查当前状态是否在目标状态列表中
                    for (const auto& targetStatus : targetStatuses)
                    {
                        if (currentStatus == targetStatus)
                        {
                            logger_log_info("File %s reached target status %d", filename.c_str(), currentStatus);
                            return true;
                        }
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

    // 处理文件创建任务 统一生成后台默认规则的文件名，实际文件名通过重命名来实现，所以此任务处理完毕文件状态未 FILE_STATUS_CREATED
    void FileTaskProcessor::processCreateFileTask(const FileTask &task)
    {
        logger_log_info(" ------------ Processing create file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 等待文件状态变为不存在或关闭状态
            std::vector<FileStatus> targetStatuses = {FILE_STATUS_NOT_FOUND, FILE_STATUS_CLOSED};
            bool isReady = filemanager::FileQueueManager::getInstance().waitForFileStatus(task.filename, targetStatuses, 30);
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
                // 更新文件状态为创建
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_CREATED);
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

        logger_log_info(" ———————————— Finished processing create file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理文件更新任务
    void FileTaskProcessor::processUpdateFileTask(const FileTask &task)
    {
        logger_log_info(" ------------ Processing update file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            std::vector<FileStatus> targetStatuses = {FILE_STATUS_READY, FILE_STATUS_CLOSED};
            bool isReady = filemanager::FileQueueManager::getInstance().waitForFileStatus(task.filename, targetStatuses, 30);
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

        logger_log_info(" ———————————— Finished processing update file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理关闭文件任务
    void FileTaskProcessor::processCloseFileTask(const FileTask &task)
    {
        logger_log_info(" ------------ Processing close file task: %s", task.filename.c_str());

        // 更新文件状态为处理中
        filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

        // 删除文件信息记状态记录
        filemanager::fileclose(task.taskData);

        // 更新文件状态为已关闭
        filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_CLOSED);

        logger_log_info(" ———————————— Finished processing close file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理删除文件任务
    void FileTaskProcessor::processDeleteFileTask(const FileTask &task)
    {
        logger_log_info(" ------------ Processing delete file task: %s",
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

            // 首先移除文件信息状态记录，防止其他线程访问
            filemanager::FileQueueManager::getInstance().eraseFileStatus(task.filename);

            // 确保文件未被加载或已关闭
            std::string docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(absoluteFilePath), filemanager::DocumentType::SPREADSHEET);
            if (!docId.empty())
            {
                filemanager::DocumentManager::getInstance().closeDocument(docId);
            }

            // 使用LibreOffice UNO API安全删除文件，确保连接状态与文件系统一致
            bool deleted = false;
            try
            {
                // 尝试通过UNO API删除文件，使用返回值判断是否删除成功
                deleted = filemanager::SafeDeleteFile(absoluteFilePath);
            }
            catch (...)
            {
                logger_log_error("UNO API delete failed, attempting OS-level delete as fallback");

// 如果UNO API删除失败，作为备选方案使用操作系统API
#ifdef _WIN32
                // 尝试多次删除，处理可能的文件锁问题
                int retryCount = 5;
                while (retryCount > 0)
                {
                    deleted = DeleteFileA(filePath.c_str()) != 0;
                    if (deleted)
                        break;

                    // 如果删除失败，等待一段时间后重试
                    logger_log_warning("Failed to delete file: %s, retrying... (%d attempts left)",
                                       filePath.c_str(), retryCount - 1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    retryCount--;
                }
#else
                deleted = unlink(filePath.c_str()) == 0;
#endif
            }

            if (deleted)
            {
                logger_log_info("Successfully deleted file: %s", filePath.c_str());

                // 增加额外的等待时间，确保文件完全从操作系统缓存中释放
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                // 在删除文件后，尝试使用UNO API再次确认文件已被删除
                try
                {
                    uno::Reference<uno::XComponentContext> xContext = LibreOfficeService::getContext();
                    if (xContext.is())
                    {
                        uno::Reference<lang::XMultiComponentFactory> xServiceManager(xContext->getServiceManager(), uno::UNO_QUERY);
                        if (xServiceManager.is())
                        {
                            uno::Reference<ucb::XSimpleFileAccess> fileAccess(xServiceManager->createInstanceWithContext(
                                                                                  convertStringToOUString("com.sun.star.ucb.SimpleFileAccess"), xContext),
                                                                              uno::UNO_QUERY);
                            if (fileAccess.is())
                            {
                                // 尝试多次检查文件是否存在，直到确认文件已删除或达到最大重试次数
                                int confirmRetry = 3;
                                while (confirmRetry > 0)
                                {
                                    if (!fileAccess->exists(absoluteFilePath))
                                    {
                                        logger_log_info("UNO API confirmed file deleted: %s", filePath.c_str());
                                        break;
                                    }

                                    logger_log_info("File still exists in UNO cache, waiting...");
                                    std::this_thread::sleep_for(std::chrono::milliseconds(300));
                                    confirmRetry--;
                                }
                            }
                        }
                    }
                }
                catch (const uno::Exception &e)
                {
                    logger_log_error("UNO Exception during file deletion confirmation: %s",
                                     rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
                }
                catch (...)
                {
                    logger_log_error("Unknown exception during file deletion confirmation");
                }
            }
            else
            {
#ifdef _WIN32
                DWORD error = GetLastError();
                logger_log_error("Failed to delete file: %s, Windows error: %d", filePath.c_str(), error);
#else
                logger_log_error("Failed to delete file: %s, errno: %d", filePath.c_str(), errno);
#endif
            }

            // 从状态映射表中移除文件
            filemanager::FileQueueManager::getInstance().removeFileInfo(task.filename);
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

        logger_log_info(" ———————————— Finished processing delete file task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理更新模板任务
    void FileTaskProcessor::processUpdateTemplateTask(const FileTask &task)
    {
        logger_log_info(" ------------ Processing update template task",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
        // 更新文件状态为处理中
        filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_PROCESSING);
        try
        {
            // 直接重新初始化文件
            int res = fileTemplateChange(task.filename);
            if (res != 0)
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
        logger_log_info(" ———————————— Finished processing update template task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理新增工作表任务
    void FileTaskProcessor::processAddWorksheetTask(const FileTask &task)
    {
        logger_log_info(" ------------ Processing add worksheet task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            std::vector<FileStatus> targetStatuses = {FILE_STATUS_READY, FILE_STATUS_CLOSED};
            bool isReady = filemanager::FileQueueManager::getInstance().waitForFileStatus(task.filename, targetStatuses, 30);
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

        logger_log_info(" ———————————— Finished processing add worksheet task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理删除工作表任务
    void FileTaskProcessor::processRemoveWorksheetTask(const FileTask &task)
    {
        logger_log_info(" ------------ Processing remove worksheet task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            std::vector<FileStatus> targetStatuses = {FILE_STATUS_CREATED, FILE_STATUS_CLOSED};
            bool isReady = filemanager::FileQueueManager::getInstance().waitForFileStatus(task.filename, targetStatuses, 30);
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

        logger_log_info(" ———————————— Finished processing remove worksheet task: %s",
                        filemanager::convertOUStringToString(filemanager::convertStringToOUString(task.filename.c_str())).c_str());
    }

    // 处理重命名工作表任务
    void FileTaskProcessor::processRenameWorksheetTask(const FileTask &task)
    {
        logger_log_info(" ------------ Processing rename worksheet task: %s", task.filename.c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            std::vector<FileStatus> targetStatuses = {FILE_STATUS_CREATED, FILE_STATUS_CLOSED};
            bool isReady = filemanager::FileQueueManager::getInstance().waitForFileStatus(task.filename, targetStatuses, 30);
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

        logger_log_info(" ———————————— Finished processing rename worksheet task: %s", task.filename.c_str());
    }
    // 处理重命名文件任务
    void FileTaskProcessor::processRenameFileTask(const FileTask &task)
    {
        logger_log_info(" ------------ Processing rename file task: %s", ensureOdsExtension(task.filename).c_str());

        try
        {
            // 等待文件状态变为就绪或关闭状态
            std::vector<FileStatus> targetStatuses = {FILE_STATUS_CREATED, FILE_STATUS_READY,FILE_STATUS_CLOSED};
            bool isReady = filemanager::FileQueueManager::getInstance().waitForFileStatus(task.filename, targetStatuses, 30);
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

            // 首先关闭源文件，如果已加载
            std::string docId = filemanager::DocumentManager::getInstance().openDocument(convertOUStringToString(oldAbsoluteFilePath), filemanager::DocumentType::SPREADSHEET);
            if (!docId.empty())
            {
                filemanager::DocumentManager::getInstance().closeDocument(docId);
            }

            // 检查目标文件是否存在
            FileInfo fileInfo = filemanager::FileQueueManager::getInstance().getFileInfo(newFilename);
            if (fileInfo.filename == newFilename && fileInfo.status != FILE_STATUS_NOT_FOUND)
            {
                logger_log_error("file %s exists:", newFilename.c_str());
                filemanager::FileQueueManager::getInstance().updateFileStatus(task.filename, FILE_STATUS_ERROR, "Target file already exists");
                return; // 如果新文件名存在直接返回报错
            }

            // 重命名文件
            if (rename(oldFilePath.c_str(), newFilePath.c_str()) == 0)
            {
                // 通过公有接口实现文件状态映射表的重命名逻辑
                FileInfo fileInfo = filemanager::FileQueueManager::getInstance().getFileInfo(task.filename);
                if (fileInfo.filename == task.filename)
                {
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

        logger_log_info(" ———————————— Finished processing rename file task: %s", ensureOdsExtension(task.filename).c_str());
    }

    // 处理获取工作表数据任务
    void FileTaskProcessor::processSheetDataTask(const FileTask &task)
    {
        logger_log_info(" ------------ Processing sheet data task");

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

        logger_log_info(" ———————————— Finished processing sheet data task");
    }

    // 移除文件信息
    void FileQueueManager::removeFileInfo(const std::string &filename)
    {
        FileInfo info = getFileInfo(filename);
        if (info.filename == filename)
        {
            // 不再需要手动关闭xDoc，DocumentManager会处理资源释放
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
    QueueStats FileQueueManager::getQueueStats() const
    {
        QueueStats stats{};
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            for (const auto &kv : fileStatusMap)
            {
                switch (kv.second.status)
                {
                case FILE_STATUS_ERROR:
                    stats.errorCount++;
                    break;
                case FILE_STATUS_READY:
                    stats.readyCount++;
                    break;
                case FILE_STATUS_PROCESSING:
                    stats.processingCount++;
                    break;
                case FILE_STATUS_CLOSED:
                    stats.closedCount++;
                    break;
                default:
                    break;
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
