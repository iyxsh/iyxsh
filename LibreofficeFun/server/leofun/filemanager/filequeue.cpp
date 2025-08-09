#include "filequeue.h"
#include "filehandlers.h"
#include "cache.h"
#include "../logger/logger.h"
#include <iostream>
#include <chrono>

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

        logger_log_info("Added file status: %s - %d", filename.c_str(), status);
    }

    // 更新文件状态
    void FileQueueManager::updateFileStatus(const std::string &filename, FileStatus status, const std::string &errorMsg)
    {
        try {
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
                logger_log_info("Updated file status: %s - %d", filename.c_str(), status);
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
                logger_log_info("Added file status: %s - %d", filename.c_str(), status);
            }
            
            // 通知等待状态变化的线程
            statusCondition.notify_all();
        }
        catch (const std::system_error& e) {
            logger_log_error("System error in updateFileStatus for %s: %s", filename.c_str(), e.what());
            logger_log_error("Error code: %d, Error category: %s", e.code().value(), e.code().category().name());
        }
        catch (const std::exception& e) {
            logger_log_error("Exception in updateFileStatus for %s: %s", filename.c_str(), e.what());
        }
        catch (...) {
            logger_log_error("Unknown exception in updateFileStatus for %s", filename.c_str());
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

        while (true)
        {
            std::unique_lock<std::mutex> lock(taskMutex);
            try
            {
                taskCondition.wait(lock, [this]
                                   { return !taskQueue.empty() || shouldStop; });
            }
            catch (...)
            {
                logger_log_info("Exception in task condition wait");
                // 出现异常时继续处理任务
                if (taskQueue.empty() && !shouldStop)
                {
                    // 如果队列为空且不应该停止，短暂等待后继续
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
            }

            if (shouldStop && taskQueue.empty())
            {
                break;
            }

            // 检查模板是否正在加载
            // 修复：添加异常处理以防止system_error异常
            bool templateLoading = false;
            try
            {
                templateLoading = isTemplateLoading();
            }
            catch (const std::system_error &e)
            {
                logger_log_error("System error checking template loading status: %s", e.what());
                logger_log_error("Error code: %d, Error category: %s", e.code().value(), e.code().category().name());
                // 出现异常时继续处理任务
            }
            catch (const std::exception &e)
            {
                logger_log_error("Exception checking template loading status: %s", e.what());
                // 出现异常时继续处理任务
            }
            catch (...)
            {
                logger_log_error("Unknown exception checking template loading status");
                // 出现异常时继续处理任务
            }

            if (templateLoading)
            {
                logger_log_info("Template is loading, pausing task processing");
                // 等待模板加载完成
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            if (!taskQueue.empty())
            {
                FileTask task = taskQueue.front();
                taskQueue.pop();
                lock.unlock();

                // 处理任务
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
                default:
                    logger_log_warn("Unknown task type: %d", task.type);
                    break;
                }
            }
        }

        logger_log_info("File task processor thread stopped");
    }

    // 处理创建文件任务
    void FileQueueManager::processCreateFileTask(const FileTask &task)
    {
        logger_log_info("Processing create file task: %s", task.filename.c_str());

        // 更新文件状态为处理中
        updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

        try
        {
            // 创建一个可修改的文件名副本
            std::string filenameCopy = task.filename;
            int res = createfile(filenameCopy);

            if (res == 0)
            {
                // 更新文件状态为就绪
                updateFileStatus(task.filename, FILE_STATUS_READY);
            }
            else
            {
                // 更新文件状态为错误
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to create file");
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in createfile for %s: %s", task.filename.c_str(), e.what());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in createfile for %s", task.filename.c_str());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing create file task: %s", task.filename.c_str());
    }

    // 处理更新文件任务
    void FileQueueManager::processUpdateFileTask(const FileTask &task)
    {
        logger_log_info("Processing update file task: %s", task.filename.c_str());

        try 
        {
            // 确保文件在状态映射表中存在，如果不存在则添加
            // 这样可以避免在updateFileStatus中出现文件不存在的情况
            {
                std::lock_guard<std::mutex> lock(statusMutex);
                auto it = fileStatusMap.find(task.filename);
                if (it == fileStatusMap.end()) {
                    FileInfo info;
                    info.filename = task.filename;
                    info.status = FILE_STATUS_CLOSED;  // 初始状态设为CLOSED
                    info.lastModified = std::time(nullptr);
                    info.errorMessage = "";
                    fileStatusMap[task.filename] = info;
                    logger_log_info("Added initial status for file: %s as CLOSED", task.filename.c_str());
                }
            }
            
            // 检查文件状态，只有在READY或CLOSED状态下才能执行更新
            // 使用条件变量等待而不是轮询，避免死锁
            logger_log_info("Waiting for file %s to be ready or closed for update", task.filename.c_str());
            bool statusReady = waitForFileStatus(task.filename, FILE_STATUS_READY, FILE_STATUS_CLOSED, 30);
            
            if (!statusReady) {
                logger_log_error("File %s is not ready for update, timeout or error occurred", task.filename.c_str());
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Timeout waiting for file to be ready");
                return;
            }

            // 更新文件状态为处理中
            updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

            // 注意：不要使用std::unique_ptr包装task.taskData，因为调用方负责内存管理
            int res = fileupdate(task.taskData);
            if (res == 0)
            {
                // 更新文件状态为就绪
                updateFileStatus(task.filename, FILE_STATUS_READY);
                logger_log_info("Successfully updated file: %s", task.filename.c_str());
            }
            else
            {
                // 更新文件状态为错误
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to update file");
                logger_log_error("Failed to update file: %s", task.filename.c_str());
            }
        }
        catch (const std::system_error& e)
        {
            logger_log_error("System error in processUpdateFileTask for %s: %s", task.filename.c_str(), e.what());
            logger_log_error("Error code: %d, Error category: %s", e.code().value(), e.code().category().name());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("System error: ") + e.what());
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processUpdateFileTask for %s: %s", task.filename.c_str(), e.what());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processUpdateFileTask for %s", task.filename.c_str());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing update file task: %s", task.filename.c_str());
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

        logger_log_info("Finished processing close file task: %s", task.filename.c_str());
    }

    // 处理删除文件任务
    void FileQueueManager::processDeleteFileTask(const FileTask &task)
    {
        logger_log_info("Processing delete file task: %s", task.filename.c_str());

        // 更新文件状态为处理中
        updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

        // 模拟处理时间
        std::this_thread::sleep_for(std::chrono::milliseconds(30));

        // 从状态映射表中移除文件
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            fileStatusMap.erase(task.filename);
        }

        logger_log_info("Finished processing delete file task: %s", task.filename.c_str());
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

        logger_log_info("Finished processing update template task: %s", task.filename.c_str());
    }

    // 等待文件状态变化
    bool FileQueueManager::waitForFileStatus(const std::string& filename, FileStatus targetStatus1, FileStatus targetStatus2, int timeoutSeconds)
    {
        try {
            std::unique_lock<std::mutex> lock(statusMutex);
            auto now = std::chrono::system_clock::now();
            auto timeout = now + std::chrono::seconds(timeoutSeconds);
            
            // 检查当前状态是否已经符合要求
            auto it = fileStatusMap.find(filename);
            if (it != fileStatusMap.end()) {
                FileStatus currentStatus = it->second.status;
                if (currentStatus == targetStatus1 || currentStatus == targetStatus2) {
                    logger_log_info("File %s already in target status %d", filename.c_str(), currentStatus);
                    return true;
                }
            } else {
                // 文件不存在于状态映射表中，对于更新操作，我们将其视为可以处理的状态
                // 因为新文件或没有状态记录的文件可以被更新
                logger_log_info("File %s not found in status map, treating as ready for processing", filename.c_str());
                return true;
            }
            
            // 等待状态变化或超时
            while (true) {
                // 等待条件变量或超时
                auto cvStatus = statusCondition.wait_until(lock, timeout);
                
                // 检查是否超时
                if (cvStatus == std::cv_status::timeout) {
                    logger_log_error("Timeout waiting for file %s to reach status %d or %d", 
                                    filename.c_str(), targetStatus1, targetStatus2);
                    return false;
                }
                
                // 检查文件状态是否符合要求
                auto it = fileStatusMap.find(filename);
                if (it != fileStatusMap.end()) {
                    FileStatus currentStatus = it->second.status;
                    if (currentStatus == targetStatus1 || currentStatus == targetStatus2) {
                        logger_log_info("File %s reached target status %d", filename.c_str(), currentStatus);
                        return true;
                    }
                    
                    // 如果文件出错，也返回失败
                    if (currentStatus == FILE_STATUS_ERROR) {
                        logger_log_info("File %s is in error state, stop waiting for status change", filename.c_str());
                        return false;
                    }
                } else {
                    // 文件在等待过程中被删除，这种情况很少见，但我们需要处理
                    logger_log_info("File %s no longer in status map during waiting", filename.c_str());
                    return true;
                }
                
                // 检查是否应该停止
                if (shouldStop) {
                    logger_log_info("Task processor stopping, stop waiting for file status");
                    return false;
                }
            }
        }
        catch (const std::system_error& e) {
            logger_log_error("System error in waitForFileStatus for %s: %s", filename.c_str(), e.what());
            logger_log_error("Error code: %d, Error category: %s", e.code().value(), e.code().category().name());
            return false;
        }
        catch (const std::exception& e) {
            logger_log_error("Exception in waitForFileStatus for %s: %s", filename.c_str(), e.what());
            return false;
        }
        catch (...) {
            logger_log_error("Unknown exception in waitForFileStatus for %s", filename.c_str());
            return false;
        }
        
        return false;
    }
    
    // 处理新增工作表任务
    void FileQueueManager::processAddWorksheetTask(const FileTask &task)
    {
        logger_log_info("Processing add worksheet task: %s", task.filename.c_str());

        // 更新文件状态为处理中
        updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

        try
        {
            // 确保taskData不为空
            if (!task.taskData)
            {
                logger_log_error("Add worksheet task has no data");
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Add worksheet task has no data");
                return;
            }

            // 调用filehandlers中的处理函数
            int res = filemanager::worksheetAdd(task.taskData);
            if (res == 0)
            {
                // 更新文件状态为就绪
                updateFileStatus(task.filename, FILE_STATUS_READY);
                logger_log_info("Successfully added worksheet for file: %s", task.filename.c_str());
            }
            else
            {
                // 更新文件状态为错误
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to add worksheet");
                logger_log_error("Failed to add worksheet for file: %s", task.filename.c_str());
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processAddWorksheetTask for %s: %s", task.filename.c_str(), e.what());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processAddWorksheetTask for %s", task.filename.c_str());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing add worksheet task: %s", task.filename.c_str());
    }
    
    // 处理删除工作表任务
    void FileQueueManager::processRemoveWorksheetTask(const FileTask &task)
    {
        logger_log_info("Processing remove worksheet task: %s", task.filename.c_str());

        // 更新文件状态为处理中
        updateFileStatus(task.filename, FILE_STATUS_PROCESSING);

        try
        {
            // 确保taskData不为空
            if (!task.taskData)
            {
                logger_log_error("Remove worksheet task has no data");
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Remove worksheet task has no data");
                return;
            }

            // 调用filehandlers中的处理函数
            int res = filemanager::worksheetRemove(task.taskData);
            if (res == 0)
            {
                // 更新文件状态为就绪
                updateFileStatus(task.filename, FILE_STATUS_READY);
                logger_log_info("Successfully removed worksheet for file: %s", task.filename.c_str());
            }
            else
            {
                // 更新文件状态为错误
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to remove worksheet");
                logger_log_error("Failed to remove worksheet for file: %s", task.filename.c_str());
            }
        }
        catch (const std::exception &e)
        {
            logger_log_error("Exception in processRemoveWorksheetTask for %s: %s", task.filename.c_str(), e.what());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            logger_log_error("Unknown exception in processRemoveWorksheetTask for %s", task.filename.c_str());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }

        logger_log_info("Finished processing remove worksheet task: %s", task.filename.c_str());
    }
} // namespace filemanager