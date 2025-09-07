#include "DocumentManager.h"
#include "LibreOfficeService.h"
#include "../logger/logger.h"
#include "../config/json_config.h"
#include "utils.h"

#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/util/XCloseable.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>

#include <chrono>
#include <random>
#include <sstream>
#include <filesystem>

namespace filemanager
{

    using namespace com::sun::star;
    using namespace com::sun::star::uno;
    using namespace com::sun::star::frame;
    using namespace com::sun::star::beans;
    using namespace com::sun::star::sheet;
    using namespace com::sun::star::text;
    using namespace com::sun::star::container;
    using namespace com::sun::star::util;

    // 实现DocumentManager单例
    DocumentManager &DocumentManager::getInstance()
    {
        static DocumentManager instance;
        return instance;
    }

    DocumentManager::DocumentManager() : m_maxDocs(10)
    {
        // 从配置中读取最大文档数
        int maxDocsFromConfig = json_config_get_int("maxOpenDocuments");
        if (maxDocsFromConfig > 0)
        {
            m_maxDocs = static_cast<size_t>(maxDocsFromConfig);
        }
        logger_log_info("DocumentManager initialized with max %zu documents", m_maxDocs);
    }

    DocumentManager::~DocumentManager()
    {
        closeAllDocuments();
        logger_log_info("DocumentManager destroyed");
    }

    std::string DocumentManager::openDocument(const std::string &filePath, DocumentType type)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 检查文件是否已打开
        auto pathIt = m_pathToDocId.find(filePath);
        if (pathIt != m_pathToDocId.end())
        {
            logger_log_info("Document already open: %s", filePath.c_str());
            return pathIt->second;
        }

        // 检查是否达到最大文档数
        if (m_documents.size() >= m_maxDocs)
        {
            logger_log_error("Reached maximum number of open documents: %zu", m_maxDocs);
            return "";
        }

        // 打开文档
        std::shared_ptr<Document> doc = openDocumentImpl(filePath, type);
        if (!doc)
        {
            return "";
        }

        // 存储文档
        const std::string &docId = doc->getId();
        m_documents[docId] = doc;
        m_pathToDocId[filePath] = docId;

        logger_log_info("Document opened successfully: %s (ID: %s)", filePath.c_str(), docId.c_str());
        return docId;
    }

    std::string DocumentManager::createDocument(const std::string &filePath, DocumentType type)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 检查文件是否已存在
        if (std::filesystem::exists(filePath))
        {
            logger_log_error("File already exists: %s", filePath.c_str());
            return "";
        }

        // 检查是否已打开
        auto pathIt = m_pathToDocId.find(filePath);
        if (pathIt != m_pathToDocId.end())
        {
            logger_log_info("Document already open: %s", filePath.c_str());
            return pathIt->second;
        }

        // 检查是否达到最大文档数
        if (m_documents.size() >= m_maxDocs)
        {
            logger_log_error("Reached maximum number of open documents: %zu", m_maxDocs);
            return "";
        }

        try
        {
            // 使用LibreOfficeService提供的组件加载器，确保连接一致性
            Reference<XComponentLoader> xComponentLoader = LibreOfficeService::getComponentLoader();
            if (!xComponentLoader.is())
            {
                logger_log_error("Failed to get component loader");
                return ""; // 返回空字符串表示失败
            }

            // 设置URL
            rtl::OUString url = rtl::OUString::createFromAscii("private:factory/");
            switch (type)
            {
            case DocumentType::SPREADSHEET:
                url += rtl::OUString::createFromAscii("scalc");
                break;
            case DocumentType::TEXT:
                url += rtl::OUString::createFromAscii("swriter");
                break;
            case DocumentType::PRESENTATION:
                url += rtl::OUString::createFromAscii("simpress");
                break;
            case DocumentType::DRAWING:
                url += rtl::OUString::createFromAscii("sdraw");
                break;
            default:
                logger_log_error("Unsupported document type");
                return "";
            }

            // 创建空文档
            Sequence<PropertyValue> loadProps(0);
            Reference<lang::XComponent> xComponent = xComponentLoader->loadComponentFromURL(
                url, rtl::OUString::createFromAscii("_blank"), 0, loadProps);

            if (!xComponent.is())
            {
                logger_log_error("Failed to create document");
                return "";
            }

            // 保存文档到指定路径
            logger_log_info("Saving new document to: %s", filePath.c_str());
            uno::Reference<uno::XInterface> xDocInterface(xComponent, UNO_QUERY);
            rtl::OUString ouFilePath = convertStringToOUString(filePath.c_str());
            if (!LibreOfficeService::saveDocument(xDocInterface, ouFilePath))
            {
                logger_log_error("Failed to save document to: %s", filePath.c_str());
                xComponent->dispose();
                return "";
            }

            // 生成文档ID
            std::string docId = generateDocId();

            // 根据类型创建文档对象
            std::shared_ptr<Document> doc;
            switch (type)
            {
            case DocumentType::SPREADSHEET:
                doc = std::make_shared<SpreadsheetDocument>(docId, filePath);
                break;
            case DocumentType::TEXT:
                doc = std::make_shared<TextDocument>(docId, filePath);
                break;
            // 其他类型暂不实现
            default:
                logger_log_error("Unsupported document type");
                xComponent->dispose();
                return "";
            }

            // 存储文档
            m_documents[docId] = doc;
            m_pathToDocId[filePath] = docId;

            logger_log_info("Document created successfully: %s (ID: %s)", filePath.c_str(), docId.c_str());
            return docId;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return "";
        }
        catch (const std::exception &e)
        {
            logger_log_error("std exception: %s", e.what());
            return "";
        }
    }

    bool DocumentManager::closeDocument(const std::string &docId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_documents.find(docId);
        if (it == m_documents.end())
        {
            logger_log_error("Document not found: %s", docId.c_str());
            return false;
        }

        std::shared_ptr<Document> doc = it->second;
        bool result = doc->close();

        if (result)
        {
            // 从映射中移除
            m_pathToDocId.erase(doc->getFilePath());
            m_documents.erase(it);
            logger_log_info("Document closed successfully: %s", docId.c_str());
        }
        else
        {
            logger_log_error("Failed to close document: %s", docId.c_str());
        }

        return result;
    }

    void DocumentManager::closeAllDocuments()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        logger_log_info("Closing all %zu documents", m_documents.size());

        // 复制文档ID列表，避免在遍历时修改映射
        std::vector<std::string> docIds;
        docIds.reserve(m_documents.size());
        for (const auto &pair : m_documents)
        {
            docIds.push_back(pair.first);
        }

        // 关闭所有文档
        for (const std::string &docId : docIds)
        {
            closeDocument(docId);
        }
    }

    std::shared_ptr<Document> DocumentManager::getDocument(const std::string &docId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_documents.find(docId);
        if (it == m_documents.end())
        {
            return nullptr;
        }

        return it->second;
    }

    std::shared_ptr<Document> DocumentManager::getDocumentByPath(const std::string &filePath)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto pathIt = m_pathToDocId.find(filePath);
        if (pathIt == m_pathToDocId.end())
        {
            return nullptr;
        }

        return getDocument(pathIt->second);
    }

    std::string DocumentManager::getDocumentIdByPath(const std::string &filePath)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto pathIt = m_pathToDocId.find(filePath);
        if (pathIt == m_pathToDocId.end())
        {
            return "";
        }

        return pathIt->second;
    }

    bool DocumentManager::isDocumentOpen(const std::string &docId) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_documents.find(docId) != m_documents.end();
    }

    std::vector<std::string> DocumentManager::getAllDocumentIds() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::vector<std::string> docIds;
        docIds.reserve(m_documents.size());
        for (const auto &pair : m_documents)
        {
            docIds.push_back(pair.first);
        }

        return docIds;
    }

    void DocumentManager::setMaxDocuments(size_t maxDocs)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_maxDocs = maxDocs;
        logger_log_info("Maximum documents set to: %zu", m_maxDocs);
    }

    size_t DocumentManager::getDocumentCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_documents.size();
}

void DocumentManager::cleanupForShutdown()
{
    try {
        logger_log_info("DocumentManager: Starting cleanup for shutdown");
        
        // 关闭所有文档
        closeAllDocuments();
        
        logger_log_info("DocumentManager: All documents closed during cleanup");
        
        // 清空所有映射表，确保没有资源泄漏
        std::lock_guard<std::mutex> lock(m_mutex);
        m_documents.clear();
        m_pathToDocId.clear();
        
        logger_log_info("DocumentManager: Cleanup for shutdown completed");
    } catch (const std::exception &e) {
        logger_log_error("DocumentManager: Exception during cleanup for shutdown: %s", e.what());
    }
}

    std::string DocumentManager::generateDocId() const
    {
        // 使用时间戳和随机数生成唯一ID
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1000, 9999);
        int randomNum = distrib(gen);

        std::stringstream ss;
        ss << "doc_" << millis << "_" << randomNum;
        return ss.str();
    }

    std::shared_ptr<Document> DocumentManager::openDocumentImpl(const std::string &filePath, DocumentType type)
    {
        try
        {
            // 获取文档类型
            if (type == DocumentType::UNKNOWN)
            {
                // 根据文件扩展名判断类型
                std::string ext = filePath.substr(filePath.find_last_of(".") + 1);
                if (ext == "ods" || ext == "xls" || ext == "xlsx")
                {
                    type = DocumentType::SPREADSHEET;
                }
                else if (ext == "odt" || ext == "doc" || ext == "docx")
                {
                    type = DocumentType::TEXT;
                }
                else
                {
                    logger_log_error("Unknown document type for file: %s", filePath.c_str());
                    return nullptr;
                }
            }

            // 生成文档ID
            std::string docId = generateDocId();

            // 根据类型创建文档对象
            std::shared_ptr<Document> doc;
            switch (type)
            {
            case DocumentType::SPREADSHEET:
                try
                {
                    doc = std::make_shared<SpreadsheetDocument>(docId, filePath);
                }
                catch (const std::runtime_error &e)
                {
                    logger_log_error("Runtime error creating spreadsheet document: %s", e.what());
                    return nullptr;
                }
                break;
            case DocumentType::TEXT:
                try
                {
                    doc = std::make_shared<TextDocument>(docId, filePath);
                }
                catch (const std::runtime_error &e)
                {
                    logger_log_error("Runtime error creating text document: %s", e.what());
                    return nullptr;
                }
                break;
            default:
                logger_log_error("Unsupported document type");
                return nullptr;
            }

            return doc;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return nullptr;
        }
        catch (const std::exception &e)
        {
            logger_log_error("std exception: %s", e.what());
            return nullptr;
        }
        catch (...)
        {
            logger_log_error("Unknown exception in openDocumentImpl");
            return nullptr;
        }
    }

    // SpreadsheetDocument 实现
    SpreadsheetDocument::SpreadsheetDocument(const std::string &docId, const std::string &filePath)
        : Document(docId, filePath, DocumentType::SPREADSHEET)
    {
        // 加载文档
        m_xDoc = LibreOfficeService::loadSpreadsheetDocument(convertStringToOUString(filePath.c_str()));
        if (!m_xDoc.is())
        {
            throw std::runtime_error("Failed to load spreadsheet document: " + filePath);
        }
    }

    SpreadsheetDocument::~SpreadsheetDocument()
    {
        close();
    }

    bool SpreadsheetDocument::save()
    {
        if (!m_xDoc.is())
        {
            return false;
        }

        try
        {
            LibreOfficeService::saveDocument(m_xDoc);
            return true;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
    }

    bool SpreadsheetDocument::saveAs(const std::string &filePath)
    {
        if (!m_xDoc.is())
        {
            return false;
        }

        try
        {
            Reference<XModel> xModel(m_xDoc, UNO_QUERY);
            LibreOfficeService::saveDocumentAs(xModel, convertStringToOUString(filePath.c_str()));
            m_filePath = filePath;
            return true;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
    }

    bool SpreadsheetDocument::close()
    {
        if (!m_xDoc.is())
        {
            return true; // 已经关闭
        }

        try
        {
            LibreOfficeService::closeDocument(m_xDoc);
            m_xDoc.clear();
            return true;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
    }

    bool SpreadsheetDocument::isModified() const
    {
        if (!m_xDoc.is())
        {
            return false;
        }

        try
        {
            Reference<XModel> xModel(m_xDoc, UNO_QUERY);
            if (xModel.is())
            {
                Reference<beans::XPropertySet> xPropertySet(m_xDoc, UNO_QUERY);
                if (xPropertySet.is()) {
                    PropertyValue aIsModified; aIsModified.Name = "IsModified";
                    xPropertySet->getPropertyValue("IsModified") >>= aIsModified.Value;
                    return aIsModified.Value.get<bool>();
                }
                return false;
            }
            return false;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
    }

    Reference<XModel> SpreadsheetDocument::getModel() const
    {
        if (!m_xDoc.is())
        {
            return Reference<XModel>();
        }

        return Reference<XModel>(m_xDoc, UNO_QUERY);
    }

    Reference<XSpreadsheetDocument> SpreadsheetDocument::getSpreadsheetDocument() const
    {
        return m_xDoc;
    }

    Reference<XSpreadsheet> SpreadsheetDocument::getSheet(const std::string &sheetName) const
    {
        if (!m_xDoc.is())
        {
            return Reference<XSpreadsheet>();
        }

        try
        {
            Reference<XSpreadsheets> xSheets = m_xDoc->getSheets();
            Reference<XNameAccess> xNameAccess(xSheets, UNO_QUERY);

            if (xNameAccess->hasByName(rtl::OUString::createFromAscii(sheetName.c_str())))
            {
                return Reference<XSpreadsheet>(xNameAccess->getByName(rtl::OUString::createFromAscii(sheetName.c_str())), UNO_QUERY);
            }
            return Reference<XSpreadsheet>();
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return Reference<XSpreadsheet>();
        }
    }

    std::vector<std::string> SpreadsheetDocument::getSheetNames() const
    {
        std::vector<std::string> sheetNames;

        if (!m_xDoc.is())
        {
            return sheetNames;
        }

        try
        {
            Reference<XSpreadsheets> xSheets = m_xDoc->getSheets();
            Reference<XNameAccess> xNameAccess(xSheets, UNO_QUERY);

            Sequence<rtl::OUString> names = xNameAccess->getElementNames();
            for (const rtl::OUString &name : names)
            {
                sheetNames.push_back(OUStringToOString(name, RTL_TEXTENCODING_UTF8).getStr());
            }

            return sheetNames;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return sheetNames;
        }
    }

    bool SpreadsheetDocument::createSheet(const std::string &sheetName) const
    {
        if (!m_xDoc.is())
        {
            return false;
        }

        try
        {
            Reference<XSpreadsheets> xSheets = m_xDoc->getSheets();

            // 检查工作表是否已存在
            Reference<XNameAccess> xNameAccess(xSheets, UNO_QUERY);
            if (xNameAccess->hasByName(rtl::OUString::createFromAscii(sheetName.c_str())))
            {
                logger_log_warn("Sheet already exists: %s", sheetName.c_str());
                return false;
            }

            // 创建新工作表
            Reference<XIndexAccess> xIndexAccess(xSheets, UNO_QUERY);
            xSheets->insertNewByName(rtl::OUString::createFromAscii(sheetName.c_str()), xIndexAccess->getCount());
            return true;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
    }

    bool SpreadsheetDocument::deleteSheet(const std::string &sheetName) const
    {
        if (!m_xDoc.is())
        {
            return false;
        }

        try
        {
            Reference<XSpreadsheets> xSheets = m_xDoc->getSheets();

            // 检查工作表是否存在
            Reference<XNameAccess> xNameAccess(xSheets, UNO_QUERY);
            if (!xNameAccess->hasByName(rtl::OUString::createFromAscii(sheetName.c_str())))
            {
                logger_log_error("Sheet not found: %s", sheetName.c_str());
                return false;
            }

            // 删除工作表
            xSheets->removeByName(rtl::OUString::createFromAscii(sheetName.c_str()));
            return true;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
    }

    // TextDocument 实现
    TextDocument::TextDocument(const std::string &docId, const std::string &filePath)
        : Document(docId, filePath, DocumentType::TEXT)
    {
        // 加载文档
        m_xDoc = LibreOfficeService::loadTextDocument(rtl::OUString::createFromAscii(filePath.c_str()));
        if (!m_xDoc.is())
        {
            throw std::runtime_error("Failed to load text document: " + filePath);
        }
    }

    TextDocument::~TextDocument()
    {
        close();
    }

    bool TextDocument::save()
    {
        if (!m_xDoc.is())
        {
            return false;
        }

        try
        {
            LibreOfficeService::saveDocument(m_xDoc);
            return true;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
    }

    bool TextDocument::saveAs(const std::string &filePath)
    {
        if (!m_xDoc.is())
        {
            return false;
        }

        try
        {
            Reference<XModel> xModel(m_xDoc, UNO_QUERY);
            LibreOfficeService::saveDocumentAs(xModel, rtl::OUString::createFromAscii(filePath.c_str()));
            m_filePath = filePath;
            return true;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
    }

    bool TextDocument::close()
    {
        if (!m_xDoc.is())
        {
            return true; // 已经关闭
        }

        try
        {
            LibreOfficeService::closeDocument(m_xDoc);
            m_xDoc.clear();
            return true;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
    }

    bool TextDocument::isModified() const
    {
        if (!m_xDoc.is())
        {
            return false;
        }

        try
        {
            Reference<XModel> xModel(m_xDoc, UNO_QUERY);
            if (xModel.is())
            {
                Reference<beans::XPropertySet> xPropertySet(m_xDoc, UNO_QUERY);
                if (xPropertySet.is()) {
                    PropertyValue aIsModified; aIsModified.Name = "IsModified";
                    xPropertySet->getPropertyValue("IsModified") >>= aIsModified.Value;
                    return aIsModified.Value.get<bool>();
                }
                return false;
            }
            return false;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO exception: %s", OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
    }

    Reference<XModel> TextDocument::getModel() const
    {
        if (!m_xDoc.is())
        {
            return Reference<XModel>();
        }

        return Reference<XModel>(m_xDoc, UNO_QUERY);
    }

    Reference<XTextDocument> TextDocument::getTextDocument() const
    {
        return m_xDoc;
    }
} // namespace filemanager