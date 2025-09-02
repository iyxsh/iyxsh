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
#include <vector>
#include "../cJSON/cJSON.h" // 使用相对当前文件的路径
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
using namespace com::sun::star;
namespace filemanager
{

    // 文件状态枚举
    enum FileStatus
    {
        FILE_STATUS_CREATED,    // 文件已创建
        FILE_STATUS_PROCESSING, // 文件正在处理中
        FILE_STATUS_READY,      // 文件就绪
        FILE_STATUS_ERROR,      // 文件处理出错
        FILE_STATUS_CLOSED,     // 文件已关闭
        FILE_STATUS_NOT_FOUND   // 文件未找到
    };

    // 文件任务类型枚举
    enum TaskType
    {
        TASK_CREATE_FILE,      // 创建文件任务
        TASK_UPDATE_FILE,      // 更新文件任务
        TASK_CLOSE_FILE,       // 关闭文件任务
        TASK_DELETE_FILE,      // 删除文件任务
        TASK_UPDATE_TEMPLATE,  // 更新模板任务
        TASK_ADD_WORKSHEET,    // 新增工作表任务
        TASK_REMOVE_WORKSHEET, // 删除工作表任务
        TASK_RENAME_WORKSHEET, // 重命名工作表任务
        TASK_RENAME_FILE,      // 重命名文件任务
        TASK_SHEET_DATA        // 获取工作表数据任务
    };

    // 文件状态信息结构
    struct FileInfo
    {
        std::string filename;
        FileStatus status;
        time_t lastModified;
        std::string errorMessage;
        // 优化：xComponent用于缓存已加载的UNO文档组件，需配合引用计数管理生命周期
        uno::Reference<sheet::XSpreadsheetDocument> xDoc;
        int refCount = 0; // 引用计数，>0表示有业务持有，关闭时需判断
    };

    // 文件任务结构
    struct FileTask
    {
        TaskType type;
        std::string filename;
        std::string data; // 任务相关数据（如创建文件时的内容）
        cJSON *taskData;  // 任务的JSON数据
        time_t createTime;
    };

    // 队列和错误统计信息结构
    struct QueueStats
    {
        size_t queueLength = 0;
        size_t errorCount = 0;
        size_t readyCount = 0;
        size_t processingCount = 0;
        size_t closedCount = 0;
    };

    // 文件任务处理类
    class FileTaskProcessor
    {
    public:
        // 处理文件创建任务
        void processCreateFileTask(const FileTask &task);

        // 处理文件更新任务
        void processUpdateFileTask(const FileTask &task);

        // 处理关闭文件任务
        void processCloseFileTask(const FileTask &task);

        // 处理删除文件任务
        void processDeleteFileTask(const FileTask &task);

        // 处理更新模板任务
        void processUpdateTemplateTask(const FileTask &task);

        // 处理新增工作表任务
        void processAddWorksheetTask(const FileTask &task);

        // 处理删除工作表任务
        void processRemoveWorksheetTask(const FileTask &task);

        // 处理重命名工作表任务
        void processRenameWorksheetTask(const FileTask &task);

        // 处理重命名文件任务
        void processRenameFileTask(const FileTask &task);

        // 处理获取工作表数据任务
        void processSheetDataTask(const FileTask &task);
    };

    // 修改 FileQueueManager 类以使用 FileTaskProcessor
    class FileQueueManager
    {
    public:
        // 处理重命名文件任务
        void processRenameFileTask(const FileTask &task);
        // 处理获取工作表数据任务
        void processSheetDataTask(const FileTask &task);

    public:
        static FileQueueManager &getInstance();

        // 添加文件状态
        void addFileStatus(const std::string &filename, FileStatus status, const std::string &errorMsg = "");

        // 添加文件状态（完整信息）
        void addFileStatus(const FileInfo &fileInfo);

        // 更新文件状态
        void updateFileStatus(const std::string &filename, FileStatus status, const std::string &errorMsg = "");

        void updateFilexDoc(const std::string &filename, uno::Reference<sheet::XSpreadsheetDocument> &xDoc, const std::string &errorMsg);

        // 获取文件状态
        FileStatus getFileStatus(const std::string &filename);

        // 添加文件任务
        void addFileTask(const FileTask &task);

        // 获取所有文件状态副本
        std::unordered_map<std::string, FileInfo> getFileStatusMapCopy() const;
        // 获取所有文件状态副本（重载，直接返回副本）
        std::unordered_map<std::string, FileInfo> getFileStatusMap() const;

        // 获取文件信息
        FileInfo getFileInfo(const std::string &filename);

        // 获取下一个任务
        bool getNextTask(FileTask &task);

        // 等待文件状态变化
        bool waitForFileStatus(const std::string &filename, const std::vector<FileStatus>& targetStatuses, int timeoutSeconds);

        // 启动任务处理线程
        void startTaskProcessor();

        // 停止任务处理线程
        void stopTaskProcessor();
        void removeFileInfo(const std::string &filename);
        void eraseFileStatus(const std::string &filename);

        // 获取队列和错误统计信息
        QueueStats getQueueStats() const;

    private:
        FileQueueManager();
        ~FileQueueManager();

        // 禁止拷贝构造和赋值
        FileQueueManager(const FileQueueManager &) = delete;
        FileQueueManager &operator=(const FileQueueManager &) = delete;

        // 任务处理线程函数
        void processTasks();

        // 文件任务队列
        std::queue<FileTask> taskQueue;

        // 互斥锁
        mutable std::mutex taskMutex;

        // 条件变量
        std::condition_variable taskCondition;

        // 任务处理线程
        std::thread taskProcessorThread;
        bool shouldStop;

        // 模板加载状态
        bool templateLoading;
        mutable std::mutex templateMutex;

        std::unordered_map<std::string, FileInfo> fileStatusMap;
        mutable std::mutex statusMutex;
        std::condition_variable statusCondition;

        // 新增 FileTaskProcessor 实例
        FileTaskProcessor taskProcessor;
    };

} // namespace filemanager

#endif // FILEQUEUE_H