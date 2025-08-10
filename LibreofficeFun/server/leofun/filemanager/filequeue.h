#ifndef FILEQUEUE_H
#define FILEQUEUE_H

#include <queue>
#include <unordered_map>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <memory>
#include <chrono>
#include "../cJSON/cJSON.h"

namespace filemanager {

    // 文件状态枚举
    enum FileStatus {
        FILE_STATUS_CREATED,      // 文件已创建
        FILE_STATUS_PROCESSING,   // 文件正在处理中
        FILE_STATUS_READY,        // 文件就绪
        FILE_STATUS_ERROR,        // 文件处理出错
        FILE_STATUS_CLOSED        // 文件已关闭
    };

    // 文件任务类型枚举
    enum TaskType {
        TASK_CREATE_FILE,         // 创建文件任务
        TASK_UPDATE_FILE,         // 更新文件任务
        TASK_CLOSE_FILE,          // 关闭文件任务
        TASK_DELETE_FILE,         // 删除文件任务
        TASK_UPDATE_TEMPLATE,     // 更新模板任务
        TASK_ADD_WORKSHEET,       // 新增工作表任务
        TASK_REMOVE_WORKSHEET,    // 删除工作表任务
        TASK_RENAME_FILE          // 重命名文件任务
    };

    // 文件状态信息结构
    struct FileInfo {
        std::string filename;
        FileStatus status;
        time_t lastModified;
        std::string errorMessage;
    };

    // 文件任务结构
    struct FileTask {
        TaskType type;
        std::string filename;
        std::string data;  // 任务相关数据（如创建文件时的内容）
        cJSON* taskData;   // 任务的JSON数据
        time_t createTime;
    };

    // 文件队列管理类
    class FileQueueManager {
    public:
        static FileQueueManager& getInstance();
        
        // 添加文件状态
        void addFileStatus(const std::string& filename, FileStatus status, const std::string& errorMsg = "");
        
        // 添加文件状态（完整信息）
        void addFileStatus(const FileInfo& fileInfo);
        
        // 更新文件状态
        void updateFileStatus(const std::string& filename, FileStatus status, const std::string& errorMsg = "");
        
        // 获取文件状态
        FileStatus getFileStatus(const std::string& filename);
        
        // 添加文件任务
        void addFileTask(const FileTask& task);
        
        // 获取下一个任务
        bool getNextTask(FileTask& task);
        
        // 获取文件信息
        FileInfo getFileInfo(const std::string& filename);
        
        // 获取所有文件状态
        const std::unordered_map<std::string, FileInfo>& getFileStatusMap() const;
        
        // 获取所有文件状态的副本
        std::unordered_map<std::string, FileInfo> getFileStatusMapCopy() const;
        
        // 启动任务处理线程
        void startTaskProcessor();
        
        // 停止任务处理线程
        void stopTaskProcessor();
        
        // 检查模板是否正在加载
        bool isTemplateLoading() const;
        
        // 设置模板加载状态
        void setTemplateLoading(bool loading);

    private:
        FileQueueManager();
        ~FileQueueManager();
        
        // 禁止拷贝构造和赋值
        FileQueueManager(const FileQueueManager&) = delete;
        FileQueueManager& operator=(const FileQueueManager&) = delete;
        
        // 任务处理线程函数
        void processTasks();
        
        // 处理创建文件任务
        void processCreateFileTask(const FileTask& task);
        
        // 处理更新文件任务
        void processUpdateFileTask(const FileTask& task);
        
        // 处理关闭文件任务
        void processCloseFileTask(const FileTask& task);
        
        // 处理删除文件任务
        void processDeleteFileTask(const FileTask& task);
        
        // 处理更新模板任务
        void processUpdateTemplateTask(const FileTask& task);
        
        // 处理新增工作表任务
        void processAddWorksheetTask(const FileTask& task);
        
        // 处理删除工作表任务
        void processRemoveWorksheetTask(const FileTask& task);
        
        // 处理重命名文件任务
        void processRenameFileTask(const FileTask& task);

        // 等待文件状态变化
        bool waitForFileStatus(const std::string& filename, FileStatus targetStatus1, FileStatus targetStatus2, int timeoutSeconds = 30);

        // 文件状态映射表
        std::unordered_map<std::string, FileInfo> fileStatusMap;
        
        // 文件任务队列
        std::queue<FileTask> taskQueue;
        
        // 互斥锁
        mutable std::mutex statusMutex;
        std::mutex taskMutex;
        
        // 条件变量
        mutable std::condition_variable statusCondition;
        std::condition_variable taskCondition;
        
        // 任务处理线程
        std::thread taskProcessorThread;
        bool shouldStop;
        
        // 模板加载状态
        bool templateLoading;
        mutable std::mutex templateMutex;
    };

} // namespace filemanager

#endif // FILEQUEUE_H