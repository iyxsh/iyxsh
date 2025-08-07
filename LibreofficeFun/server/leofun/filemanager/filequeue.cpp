#include "filequeue.h"
#include "filehandlers.h"
#include "cache.h"
#include "../logger/logger.h"
#include <iostream>
#include <chrono>

namespace filemanager {

    // 获取单例实例
    FileQueueManager& FileQueueManager::getInstance() {
        static FileQueueManager instance;
        return instance;
    }

    // 构造函数
    FileQueueManager::FileQueueManager() : shouldStop(false), templateLoading(false) {
        logger_log("FileQueueManager initialized");
    }

    // 析构函数
    FileQueueManager::~FileQueueManager() {
        stopTaskProcessor();
        logger_log("FileQueueManager destroyed");
    }

    // 添加文件状态
    void FileQueueManager::addFileStatus(const std::string& filename, FileStatus status, const std::string& errorMsg) {
        std::lock_guard<std::mutex> lock(statusMutex);
        FileInfo info;
        info.filename = filename;
        info.status = status;
        info.lastModified = std::time(nullptr);
        info.errorMessage = errorMsg;
        fileStatusMap[filename] = info;
        
        logger_log("Added file status: %s - %d", filename.c_str(), status);
    }

    // 更新文件状态
    void FileQueueManager::updateFileStatus(const std::string& filename, FileStatus status, const std::string& errorMsg) {
        std::lock_guard<std::mutex> lock(statusMutex);
        auto it = fileStatusMap.find(filename);
        if (it != fileStatusMap.end()) {
            it->second.status = status;
            it->second.lastModified = std::time(nullptr);
            if (!errorMsg.empty()) {
                it->second.errorMessage = errorMsg;
            }
            logger_log("Updated file status: %s - %d", filename.c_str(), status);
        } else {
            // 如果文件不存在，添加新条目
            addFileStatus(filename, status, errorMsg);
        }
    }

    // 获取文件状态
    FileStatus FileQueueManager::getFileStatus(const std::string& filename) {
        std::lock_guard<std::mutex> lock(statusMutex);
        auto it = fileStatusMap.find(filename);
        if (it != fileStatusMap.end()) {
            return it->second.status;
        }
        return FILE_STATUS_CLOSED; // 默认返回已关闭状态
    }

    // 获取文件信息
    FileInfo FileQueueManager::getFileInfo(const std::string& filename) {
        std::lock_guard<std::mutex> lock(statusMutex);
        auto it = fileStatusMap.find(filename);
        if (it != fileStatusMap.end()) {
            return it->second;
        }
        
        FileInfo info;
        info.filename = filename;
        info.status = FILE_STATUS_CLOSED;
        info.lastModified = 0;
        return info;
    }
    
    // 获取所有文件状态的副本
    std::unordered_map<std::string, FileInfo> FileQueueManager::getFileStatusMapCopy() const {
        std::lock_guard<std::mutex> lock(statusMutex);
        return fileStatusMap; // 返回副本，确保线程安全
    }

    // 添加文件任务
    void FileQueueManager::addFileTask(const FileTask& task) {
        std::lock_guard<std::mutex> lock(taskMutex);
        taskQueue.push(task);
        taskCondition.notify_one();
        
        logger_log("Added file task: type=%d, filename=%s", task.type, task.filename.c_str());
    }

    // 获取下一个任务
    bool FileQueueManager::getNextTask(FileTask& task) {
        std::lock_guard<std::mutex> lock(taskMutex);
        if (!taskQueue.empty()) {
            task = taskQueue.front();
            taskQueue.pop();
            return true;
        }
        return false;
    }

    // 检查模板是否正在加载
    bool FileQueueManager::isTemplateLoading() const {
        try {
            // 使用新的缓存管理器检查模板加载状态
            return filemanager::TemplateCacheManager::getInstance().isTemplateLoading();
        } catch (...) {
            logger_log("Exception in isTemplateLoading, returning false by default");
            return false;
        }
    }

    void FileQueueManager::setTemplateLoading(bool loading) {
        try {
            // 使用新的缓存管理器设置模板加载状态
            filemanager::TemplateCacheManager::getInstance().setTemplateLoading(loading);
            logger_log("Template loading status set to: %s", loading ? "true" : "false");
        } catch (...) {
            logger_log("Exception in setTemplateLoading, failed to set status to: %s", loading ? "true" : "false");
        }
    }

    // 启动任务处理线程
    void FileQueueManager::startTaskProcessor() {
        shouldStop = false;
        taskProcessorThread = std::thread(&FileQueueManager::processTasks, this);
        logger_log("File task processor started");
    }

    // 停止任务处理线程
    void FileQueueManager::stopTaskProcessor() {
        if (taskProcessorThread.joinable()) {
            {
                std::lock_guard<std::mutex> lock(taskMutex);
                shouldStop = true;
            }
            taskCondition.notify_all();
            taskProcessorThread.join();
            logger_log("File task processor stopped");
        }
    }

    // 任务处理线程函数
    void FileQueueManager::processTasks() {
        logger_log("File task processor thread started");
        
        while (true) {
            std::unique_lock<std::mutex> lock(taskMutex);
            try {
                taskCondition.wait(lock, [this] { 
                    return !taskQueue.empty() || shouldStop; 
                });
            } catch (...) {
                logger_log("Exception in task condition wait");
                // 出现异常时继续处理任务
                if (taskQueue.empty() && !shouldStop) {
                    // 如果队列为空且不应该停止，短暂等待后继续
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
            }
            
            if (shouldStop && taskQueue.empty()) {
                break;
            }
            
            // 检查模板是否正在加载
            bool templateLoading = false;
            try {
                templateLoading = isTemplateLoading();
            } catch (...) {
                logger_log("Exception checking template loading status");
                // 出现异常时继续处理任务
            }
            
            if (templateLoading) {
                logger_log("Template is loading, pausing task processing");
                // 等待模板加载完成
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            if (!taskQueue.empty()) {
                FileTask task = taskQueue.front();
                taskQueue.pop();
                lock.unlock();
                
                // 处理任务
                switch (task.type) {
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
                    default:
                        logger_log("Unknown task type: %d", task.type);
                        break;
                }
            }
        }
        
        logger_log("File task processor thread stopped");
    }

    // 处理创建文件任务
    void FileQueueManager::processCreateFileTask(const FileTask& task) {
        logger_log("Processing create file task: %s", task.filename.c_str());
        
        // 更新文件状态为处理中
        updateFileStatus(task.filename, FILE_STATUS_PROCESSING);
        
        try {
            // 创建一个可修改的文件名副本
            std::string filenameCopy = task.filename;
            int res = createfile(filenameCopy);
            
            if(res == 0)
            {
                // 更新文件状态为就绪
                updateFileStatus(task.filename, FILE_STATUS_READY);
            }
            else
            {
                // 更新文件状态为错误
                updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to create file");
            }
        } catch (const std::exception& e) {
            logger_log("Exception in createfile for %s: %s", task.filename.c_str(), e.what());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, std::string("Exception: ") + e.what());
        } catch (...) {
            logger_log("Unknown exception in createfile for %s", task.filename.c_str());
            updateFileStatus(task.filename, FILE_STATUS_ERROR, "Unknown exception occurred");
        }
        
        logger_log("Finished processing create file task: %s", task.filename.c_str());
    }

    // 处理更新文件任务
    void FileQueueManager::processUpdateFileTask(const FileTask& task) {
        logger_log("Processing update file task: %s", task.filename.c_str());
        
        // 更新文件状态为处理中
        updateFileStatus(task.filename, FILE_STATUS_PROCESSING);
        
        // 使用智能指针管理内存，确保自动释放
        std::unique_ptr<cJSON> taskDataPtr(task.taskData);
        
        int res = updatefile(taskDataPtr.get());
        if(res == 0)
        {
            // 更新文件状态为就绪
            updateFileStatus(task.filename, FILE_STATUS_READY);
        }
        else
        {
            // 更新文件状态为错误
            updateFileStatus(task.filename, FILE_STATUS_ERROR, "Failed to update file");
        }
        
        logger_log("Finished processing update file task: %s", task.filename.c_str());
    }

    // 处理关闭文件任务
    void FileQueueManager::processCloseFileTask(const FileTask& task) {
        logger_log("Processing close file task: %s", task.filename.c_str());
        
        // 更新文件状态为处理中
        updateFileStatus(task.filename, FILE_STATUS_PROCESSING);
        
        // 模拟处理时间
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        // 更新文件状态为已关闭
        updateFileStatus(task.filename, FILE_STATUS_CLOSED);
        
        logger_log("Finished processing close file task: %s", task.filename.c_str());
    }

    // 处理删除文件任务
    void FileQueueManager::processDeleteFileTask(const FileTask& task) {
        logger_log("Processing delete file task: %s", task.filename.c_str());
        
        // 更新文件状态为处理中
        updateFileStatus(task.filename, FILE_STATUS_PROCESSING);
        
        // 模拟处理时间
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        
        // 从状态映射表中移除文件
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            fileStatusMap.erase(task.filename);
        }
        
        logger_log("Finished processing delete file task: %s", task.filename.c_str());
    }
    
    // 处理更新模板任务
    void FileQueueManager::processUpdateTemplateTask(const FileTask& task) {
        logger_log("Processing update template task");
        
        try {
            // 确保taskData不为空
            if (!task.taskData) {
                logger_log("Update template task has no data");
                return;
            }
            
            updateAllFilesWithTemplateData(task.taskData);
        } catch (const std::exception& e) {
            logger_log("Exception in processUpdateTemplateTask: %s", e.what());
        } catch (...) {
            logger_log("Unknown exception in processUpdateTemplateTask");
        }
        
        logger_log("Finished processing update template task");
    }

} // namespace filemanager