#include "LibreOfficeService.h"
#include "../logger/logger.h"
#include "utils.h"
#include "ErrorHandler.h"
#include <cppuhelper/bootstrap.hxx>
#include <com/sun/star/bridge/UnoUrlResolver.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/util/XCloseable.hpp>
#include <com/sun/star/util/CloseVetoException.hpp>
#include <com/sun/star/sdbc/XCloseable.hpp>
#include <com/sun/star/io/IOException.hpp>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::frame;
using namespace com::sun::star::sheet;
using namespace com::sun::star::bridge;
using namespace com::sun::star::lang;
using namespace com::sun::star::container;
using namespace com::sun::star::beans;

namespace filemanager
{
    namespace
    {
        // 静态成员变量
        Reference<XComponentContext> s_context;
        Reference<XComponentLoader> s_loader;
        bool s_initialized = false;

        bool establishConnection() {
            try {
                logger_log_debug("Starting LibreOffice connection process...");
                
                // 初始化本地上下文
                Reference<XComponentContext> localContext = cppu::defaultBootstrap_InitialComponentContext();
                if (!localContext.is()) {
                    logger_log_error("Failed to create local component context");
                    return false;
                }
                logger_log_debug("Local component context created successfully");

                // 获取服务管理器
                Reference<XMultiComponentFactory> localMCF = localContext->getServiceManager();
                if (!localMCF.is()) {
                    logger_log_error("Failed to get local service manager");
                    return false;
                }
                logger_log_debug("Local service manager obtained");

                // 创建UnoUrlResolver
                Reference<XUnoUrlResolver> resolver = Reference<XUnoUrlResolver>(
                    localMCF->createInstanceWithContext("com.sun.star.bridge.UnoUrlResolver", localContext),
                    UNO_QUERY);
                if (!resolver.is()) {
                    logger_log_error("Failed to create UnoUrlResolver");
                    return false;
                }
                logger_log_debug("UnoUrlResolver created successfully");

                // 解析远程上下文
                rtl::OUString connectionString = "uno:socket,host=127.0.0.1,port=2002;urp;StarOffice.ComponentContext";
                logger_log_debug("Attempting to connect to LibreOffice with: %s", 
                               OUStringToOString(connectionString, RTL_TEXTENCODING_UTF8).getStr());
                
                Reference<XComponentContext> remoteContext = Reference<XComponentContext>(
                    resolver->resolve(connectionString),
                    UNO_QUERY);
                if (!remoteContext.is()) {
                    logger_log_error("Failed to resolve LibreOffice context");
                    return false;
                }
                logger_log_debug("Successfully connected to LibreOffice service");

                // 创建Desktop实例
                Reference<XDesktop2> desktop = Reference<XDesktop2>(
                    remoteContext->getServiceManager()->createInstanceWithContext(
                        "com.sun.star.frame.Desktop", remoteContext),
                    UNO_QUERY);
                if (!desktop.is()) {
                    logger_log_error("Failed to create Desktop instance");
                    return false;
                }

                s_context = remoteContext;
                s_loader = Reference<XComponentLoader>(desktop, UNO_QUERY);
                s_initialized = true;
                return true;

            } catch (const Exception& e) {
                logger_log_error("UNO Exception: %s", 
                    rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
                return false;
            }
        }
    }

    bool LibreOfficeService::initialize() {
        if (s_initialized) return true;
        
        const int maxRetries = 3;
        const int retryDelay = 1000; // 1秒延迟
        
        for (int attempt = 1; attempt <= maxRetries; ++attempt) {
            if (establishConnection()) {
                logger_log_info("Successfully connected to LibreOffice");
                return true;
            }
            
            if (attempt < maxRetries) {
                logger_log_info("Retrying connection in 1 second... (attempt %d/%d)", attempt, maxRetries);
                // 添加延迟以避免立即重试
                #ifdef _WIN32
                Sleep(retryDelay);
                #else
                usleep(retryDelay * 1000); // 转换为微秒
                #endif
            }
        }
        
        logger_log_error("Failed to connect to LibreOffice after %d attempts", maxRetries);
        logger_log_info("Please ensure LibreOffice is running with the following command:");
        logger_log_info("  libreoffice --headless --accept=\"socket,host=127.0.0.1,port=2002;urp;\"");
        return false;
    }

    uno::Reference<frame::XComponentLoader> LibreOfficeService::getComponentLoader()
    {
        if (!initialize())
            return nullptr;
        return s_loader;
    }

    uno::Reference<uno::XComponentContext> LibreOfficeService::getContext()
    {
        if (!initialize())
            return nullptr;
        return s_context;
    }
    uno::Reference<sheet::XSpreadsheetDocument>
    LibreOfficeService::CreateNewSpreadsheetDocument()
    {
        if (!initialize())
            return nullptr;
        uno::Sequence<beans::PropertyValue> args(0); // 创建一个空的参数序列
        uno::Reference<uno::XInterface> loadedIface = s_loader->loadComponentFromURL(convertStringToOUString("private:factory/scalc"), convertStringToOUString("_blank"), 0, args); // 创建新的电子表格文档
        if (!loadedIface.is())
            {
                std::cerr << "createNewSpreadsheetFile: loadComponentFromURL returned null!" << std::endl;
                return nullptr;
            }

            uno::Reference<lang::XComponent> xComp = uno::Reference<lang::XComponent>(loadedIface, uno::UNO_QUERY);
            if (!xComp.is())
            {
                std::cerr << "createNewSpreadsheetFile: loaded interface is not XComponent!" << std::endl;
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is())
            {
                std::cerr << "createNewSpreadsheetFile: loaded component is not a spreadsheet document!" << std::endl;
                closeDocument(xDoc);
                return nullptr;
            }
            logger_log_info("createNewSpreadsheetFile: new spreadsheet document created");
            return xDoc;
    }
    uno::Reference<sheet::XSpreadsheetDocument>
    LibreOfficeService::loadSpreadsheetDocument(const rtl::OUString &filePath)
    {
        if (filePath.isEmpty())
        {
            logger_log_error("Empty file path provided");
            return nullptr;
        }

        try
        {
            logger_log_debug("Attempting to load document: %s", 
                           rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
            
            auto loader = getComponentLoader();
            if (!loader.is())
            {
                logger_log_error("Invalid component loader");
                return nullptr;
            }

            // 准备加载参数
            uno::Sequence<beans::PropertyValue> loadProps(3);
            loadProps[0].Name = "Hidden";
            loadProps[0].Value <<= true;
            loadProps[1].Name = "ReadOnly";
            loadProps[1].Value <<= false;
            loadProps[2].Name = "Silent";
            loadProps[2].Value <<= true;

            rtl::OUString url = convertStringToOUString("file://") + filePath;
            auto xComponent = loader->loadComponentFromURL(
                url, convertStringToOUString("_blank"), 0, loadProps);

            if (!xComponent.is())
            {
                logger_log_error("Failed to load document: %s",
                               rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
                return nullptr;
            }

            auto xDoc = uno::Reference<sheet::XSpreadsheetDocument>(xComponent, uno::UNO_QUERY);
            if (!xDoc.is())
            {
                closeDocument(xDoc);
                logger_log_error("Loaded file is not a spreadsheet: %s",
                               rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
                return nullptr;
            }

            logger_log_info("Document loaded successfully: %s",
                          rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
            return xDoc;
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO Exception while loading document: %s",
                           rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return nullptr;
        }
        catch (const std::exception &e)
        {
            logger_log_error("STD Exception while loading document: %s", e.what());
            return nullptr;
        }
        catch (...)
        {
            logger_log_error("Unknown exception while loading document");
            return nullptr;
        }
    }

    bool LibreOfficeService::saveDocument(const uno::Reference<uno::XInterface> &doc,
                                          const rtl::OUString &filePath)
    {
        if (!doc.is())
        {
            logger_log_error("Invalid document reference");
            return false;
        }

        try
        {
            logger_log_info("Preparing to save document...");
            auto xStorable = uno::Reference<XStorable>(doc, uno::UNO_QUERY);
            if (!xStorable.is())
            {
                logger_log_error("Document is not storable");
                return false;
            }

            if (filePath.isEmpty())
            {
                logger_log_info("Saving document to original location...");
                xStorable->store();
                logger_log_info("Document saved to original location");
                return true;
            }

            logger_log_info("Saving document to new location: %s",
                           rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
            
            // 准备保存参数
            rtl::OUString url = convertStringToOUString("file://") + filePath;
            uno::Sequence<beans::PropertyValue> props(5);
            props[0].Name = "Overwrite";
            props[0].Value <<= true;
            props[1].Name = "FilterName";
            props[1].Value <<= convertStringToOUString("Calc Office Open XML");
            props[2].Name = "Compress";
            props[2].Value <<= true;
            props[3].Name = "Encrypt";
            props[3].Value <<= false;
            props[4].Name = "NoFileSync";
            props[4].Value <<= true; // 避免文件系统同步问题

            try {
                xStorable->storeAsURL(url, props);
                logger_log_info("Document saved successfully to: %s",
                              rtl::OUStringToOString(url, RTL_TEXTENCODING_UTF8).getStr());
                return true;
            } catch (const io::IOException& e) {
                // 处理文件已存在或权限问题
                logger_log_error("IO Exception while saving document: %s",
                               rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
                return false;
            } catch (const uno::RuntimeException& e) {
                // 处理运行时错误
                logger_log_error("Runtime Exception while saving document: %s",
                               rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
                return false;
            }
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO Exception while saving document: %s",
                           rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
        catch (const std::exception &e)
        {
            logger_log_error("STD Exception while saving document: %s", e.what());
            return false;
        }
        catch (...)
        {
            logger_log_error("Unknown exception while saving document");
            return false;
        }
    }

    void LibreOfficeService::closeDocument(const uno::Reference<sheet::XSpreadsheetDocument> &doc)
    {
        if (!doc.is())
        {
            logger_log_error("Attempt to close null document reference");
            return;
        }

        const int maxRetries = 3;
        const int retryDelay = 500; // 0.5秒延迟
        
        for (int attempt = 1; attempt <= maxRetries; ++attempt) {
            try
            {
                logger_log_debug("Starting to close document (attempt %d/%d)...", attempt, maxRetries);
                auto xComponent = uno::Reference<lang::XComponent>(doc, uno::UNO_QUERY);
                if (xComponent.is())
                {
                    // 尝试优雅关闭
                    try
                    {
                        Reference<util::XCloseable> xCloseable(xComponent, UNO_QUERY);
                        if (xCloseable.is())
                        {
                            try {
                                xCloseable->close(true); // 先尝试正常关闭
                                logger_log_info("Document closed gracefully");
                                return; // 成功则直接返回
                            } catch (const util::CloseVetoException& e) {
                                logger_log_error("Close vetoed: %s, attempting forced close",
                                                 convertOUStringToString(e.Message).c_str());
                                try {
                                    xCloseable->close(true);
                                    logger_log_info("Document forced closed successfully");
                                    return; // 成功则直接返回
                                } catch (const uno::Exception& e) {
                                    logger_log_error("Failed to force close document: %s",
                                                   convertOUStringToString(e.Message).c_str());
                                    // 继续执行外层异常处理
                                }
                            }
                            catch (const com::sun::star::uno::Exception& e)
                            {
                                logger_log_error("Failed to close document: %s",
                                                convertOUStringToString(e.Message).c_str());
                            }
                        }
                        
                        // 如果没有XCloseable接口，直接释放资源
                        try {
                            xComponent->dispose();
                            logger_log_info("Document resources released - attempt %d/%d", attempt, maxRetries);
                            return; // 成功则直接返回
                        } catch (const uno::Exception& e) {
                            logger_log_error("Failed to dispose document: %s",
                                           convertOUStringToString(e.Message).c_str());
                            // 继续重试
                        }
                    }
                    catch (const uno::Exception& e)
                    {
                        logger_log_error("UNO Exception while closing document: %s",
                                       convertOUStringToString(e.Message).c_str());
                        if (attempt >= maxRetries) break;
                        #ifdef _WIN32
                        Sleep(retryDelay);
                        #else
                        usleep(retryDelay * 1000);
                        #endif
                    }
                }
            }
            catch (const uno::Exception &e)
            {
                logger_log_error("UNO Exception while closing document: %s",
                               convertOUStringToString(e.Message).c_str());
                if (attempt >= maxRetries) break;
                #ifdef _WIN32
                Sleep(retryDelay);
                #else
                usleep(retryDelay * 1000);
                #endif
            }
            catch (const std::exception &e)
            {
                logger_log_error("STD Exception while closing document: %s", e.what());
                if (attempt >= maxRetries) break;
                #ifdef _WIN32
                Sleep(retryDelay);
                #else
                usleep(retryDelay * 1000);
                #endif
            }
            catch (...)
            {
                logger_log_error("Unknown exception while closing document");
                if (attempt >= maxRetries) break;
                #ifdef _WIN32
                Sleep(retryDelay);
                #else
                usleep(retryDelay * 1000);
                #endif
            }
        }
    }

    bool LibreOfficeService::isAvailable()
    {
        if (!s_initialized)
        {
            logger_log_debug("Service not initialized");
            return false;
        }

        try
        {
            logger_log_debug("Checking service availability...");
            auto mcf = s_context->getServiceManager();
            bool available = mcf.is();
            
            if (!available)
            {
                logger_log_error("Service manager is not available");
                s_initialized = false;
            }
            
            return available;
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO Exception while checking availability: %s",
                           rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            s_initialized = false;
            return false;
        }
        catch (...)
        {
            logger_log_error("Unknown exception while checking availability");
            s_initialized = false;
            return false;
        }
    }
    void LibreOfficeService::shutdown() {
        try {
            logger_log_info("Starting LibreOffice service shutdown...");
            
            // 正确的资源释放方式
            if (s_loader.is()) {
                Reference<XComponent> xComp(s_loader, UNO_QUERY);
                if (xComp.is()) {
                    logger_log_debug("Disposing component loader...");
                    xComp->dispose();
                }
            }
            
            s_loader.clear();
            s_context.clear();
            s_initialized = false;
            
            logger_log_info("LibreOffice service shutdown completed");
        } catch (const Exception& e) {
            logger_log_error("Shutdown error: %s",
                rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
    }
}