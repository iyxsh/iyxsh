#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/frame/XModel.hpp>

// 前向声明
namespace com
{
    namespace sun
    {
        namespace star
        {
            namespace sheet
            {
                class XSpreadsheetDocument;
                class XSpreadsheet;
            }
        }
    }
}

namespace com
{
    namespace sun
    {
        namespace star
        {
            namespace text
            {
                class XTextDocument;
            }
        }
    }
}

namespace com
{
    namespace sun
    {
        namespace star
        {
            namespace frame
            {
                class XModel;
            }
        }
    }
}

// 别名定义
namespace css = com::sun::star;

namespace filemanager
{

    // 前向声明
    class Document;
    class SpreadsheetDocument;
    class TextDocument;

    /**
     * 文档类型枚举
     */
    enum class DocumentType
    {
        SPREADSHEET,
        TEXT,
        PRESENTATION,
        DRAWING,
        UNKNOWN
    };

    /**
     * 文档管理器类，负责管理多个文档实例
     * 提供文档的创建、打开、关闭、查询等功能
     */
    class DocumentManager
    {
    public:
        /**
         * 获取DocumentManager实例
         * @return DocumentManager& 引用
         */
        static DocumentManager &getInstance();

        /**
         * 打开一个文档
         * @param filePath 文件路径
         * @param type 文档类型，如果为UNKNOWN则自动检测
         * @return 文档ID，如果失败则返回空字符串
         */
        std::string openDocument(const std::string &filePath, DocumentType type = DocumentType::UNKNOWN);

        /**
         * 创建一个新文档
         * @param filePath 文件路径
         * @param type 文档类型
         * @return 文档ID，如果失败则返回空字符串
         */
        std::string createDocument(const std::string &filePath, DocumentType type);

        /**
         * 关闭指定文档
         * @param docId 文档ID
         * @return 是否成功关闭
         */
        bool closeDocument(const std::string &docId);

        /**
         * 关闭所有文档
         */
        void closeAllDocuments();

        /**
         * 获取指定ID的文档
         * @param docId 文档ID
         * @return 文档指针，如果不存在则返回nullptr
         */
        std::shared_ptr<Document> getDocument(const std::string &docId);

        /**
         * 获取指定文件路径的文档
         * @param filePath 文件路径
         * @return 文档指针，如果不存在则返回nullptr
         */
        std::shared_ptr<Document> getDocumentByPath(const std::string &filePath);

        /**
         * 获取指定文件路径的文档ID
         * @param filePath 文件路径
         * @return 文档ID，如果不存在则返回空字符串
         */
        std::string getDocumentIdByPath(const std::string &filePath);

        /**
         * 检查文档是否已打开
         * @param docId 文档ID
         * @return 是否已打开
         */
        bool isDocumentOpen(const std::string &docId) const;

        /**
         * 获取所有打开的文档ID
         * @return 文档ID列表
         */
        std::vector<std::string> getAllDocumentIds() const;

        /**
         * 设置最大打开文档数
         * @param maxDocs 最大文档数
         */
        void setMaxDocuments(size_t maxDocs);

        /**
     * 获取当前打开的文档数
     * @return 文档数
     */
    size_t getDocumentCount() const;
    
    /**
     * 主动清理资源（用于程序退出前调用）
     */
    void cleanupForShutdown();

private:
        // 私有构造函数，防止外部实例化
        DocumentManager();
        // 析构函数
        ~DocumentManager();

        // 禁止拷贝和移动
        DocumentManager(const DocumentManager &) = delete;
        DocumentManager &operator=(const DocumentManager &) = delete;
        DocumentManager(DocumentManager &&) = delete;
        DocumentManager &operator=(DocumentManager &&) = delete;

        // 生成唯一文档ID
        std::string generateDocId() const;

        // 内部打开文档实现
        std::shared_ptr<Document> openDocumentImpl(const std::string &filePath, DocumentType type);

        // 文档映射表：ID -> 文档指针
        std::unordered_map<std::string, std::shared_ptr<Document>> m_documents;
        // 文件路径到文档ID的映射
        std::unordered_map<std::string, std::string> m_pathToDocId;
        // 互斥锁，保护并发访问
        mutable std::mutex m_mutex;
        // 最大打开文档数
        size_t m_maxDocs;
    };

    /**
     * 文档抽象基类
     */
    class Document
    {
    public:
        virtual ~Document() = default;

        /**
         * 获取文档ID
         * @return 文档ID
         */
        const std::string &getId() const { return m_docId; }

        /**
         * 获取文件路径
         * @return 文件路径
         */
        const std::string &getFilePath() const { return m_filePath; }

        /**
         * 获取文档类型
         * @return 文档类型
         */
        DocumentType getType() const { return m_type; }

        /**
         * 保存文档
         * @return 是否成功
         */
        virtual bool save() = 0;

        /**
         * 另存为
         * @param filePath 新文件路径
         * @return 是否成功
         */
        virtual bool saveAs(const std::string &filePath) = 0;

        /**
         * 关闭文档
         * @return 是否成功
         */
        virtual bool close() = 0;

        /**
         * 检查文档是否已修改
         * @return 是否已修改
         */
        virtual bool isModified() const = 0;

        /**
         * 获取UNO模型接口
         * @return XModel引用
         */
        virtual css::uno::Reference<css::frame::XModel> getModel() const = 0;

    protected:
        Document(const std::string &docId, const std::string &filePath, DocumentType type)
            : m_docId(docId), m_filePath(filePath), m_type(type) {}

        std::string m_docId;
        std::string m_filePath;
        DocumentType m_type;
    };

    /**
     * 电子表格文档类
     */
    class SpreadsheetDocument : public Document
    {
    public:
        SpreadsheetDocument(const std::string &docId, const std::string &filePath);
        ~SpreadsheetDocument() override;

        bool save() override;
        bool saveAs(const std::string &filePath) override;
        bool close() override;
        bool isModified() const override;
        css::uno::Reference<css::frame::XModel> getModel() const override;

        /**
         * 获取电子表格文档接口
         * @return XSpreadsheetDocument引用
         */
        css::uno::Reference<css::sheet::XSpreadsheetDocument> getSpreadsheetDocument() const;

        /**
         * 获取指定工作表
         * @param sheetName 工作表名称
         * @return 工作表引用，如果不存在则返回空
         */
        css::uno::Reference<css::sheet::XSpreadsheet> getSheet(const std::string &sheetName) const;

        /**
         * 获取所有工作表名称
         * @return 工作表名称列表
         */
        std::vector<std::string> getSheetNames() const;

        /**
         * 创建新工作表
         * @param sheetName 工作表名称
         * @return 是否成功
         */
        bool createSheet(const std::string &sheetName) const;

        /**
         * 删除工作表
         * @param sheetName 工作表名称
         * @return 是否成功
         */
        bool deleteSheet(const std::string &sheetName) const;

    private:
        css::uno::Reference<css::sheet::XSpreadsheetDocument> m_xDoc;
    };

    /**
     * 文本文档类
     */
    class TextDocument : public Document
    {
    public:
        TextDocument(const std::string &docId, const std::string &filePath);
        ~TextDocument() override;

        bool save() override;
        bool saveAs(const std::string &filePath) override;
        bool close() override;
        bool isModified() const override;
        css::uno::Reference<css::frame::XModel> getModel() const override;

        /**
         * 获取文本文档接口
         * @return XTextDocument引用
         */
        css::uno::Reference<css::text::XTextDocument> getTextDocument() const;

    private:
        css::uno::Reference<css::text::XTextDocument> m_xDoc;
    };

} // namespace filemanager