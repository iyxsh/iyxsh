#include "LibreOfficeService.h"
#include "../logger/logger.h"
#include "utils.h"
#include "ErrorHandler.h"
#include <cppuhelper/bootstrap.hxx>
#include <thread>
#include <com/sun/star/bridge/UnoUrlResolver.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/util/XCloseable.hpp>
#include <com/sun/star/util/CloseVetoException.hpp>
#include <com/sun/star/sdbc/XCloseable.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/frame/XDesktop.hpp>
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
using namespace com::sun::star::text;
using namespace com::sun::star::util;

namespace filemanager
{
    // 初始化静态成员变量
    Reference<XComponentContext> LibreOfficeService::m_xContext;
    Reference<XMultiComponentFactory> LibreOfficeService::m_xServiceManager;
    Reference<XDesktop> LibreOfficeService::m_xDesktop;
    Reference<XComponentLoader> LibreOfficeService::m_xComponentLoader;
    bool LibreOfficeService::m_initialized = false;

    bool LibreOfficeService::establishConnection()
    {
        try
        {
            logger_log_info("Starting LibreOffice connection process...");

            // 初始化本地上下文
            Reference<XComponentContext> localContext = cppu::defaultBootstrap_InitialComponentContext();
            if (!localContext.is())
            {
                logger_log_error("Failed to create local component context");
                return false;
            }
            logger_log_info("Local component context created successfully");

            // 获取服务管理器
            Reference<XMultiComponentFactory> localMCF = localContext->getServiceManager();
            if (!localMCF.is())
            {
                logger_log_error("Failed to get local service manager");
                return false;
            }
            logger_log_debug("Local service manager obtained");

            // 创建UnoUrlResolver
            Reference<XUnoUrlResolver> resolver = Reference<XUnoUrlResolver>(
                localMCF->createInstanceWithContext("com.sun.star.bridge.UnoUrlResolver", localContext),
                UNO_QUERY);
            if (!resolver.is())
            {
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
            if (!remoteContext.is())
            {
                logger_log_error("Failed to resolve LibreOffice context");
                return false;
            }
            logger_log_debug("Successfully connected to LibreOffice service");

            // 创建Desktop实例
            Reference<XDesktop> desktop = Reference<XDesktop>(
                remoteContext->getServiceManager()->createInstanceWithContext(
                    "com.sun.star.frame.Desktop", remoteContext),
                UNO_QUERY);
            if (!desktop.is())
            {
                logger_log_error("Failed to create Desktop instance");
                return false;
            }

            LibreOfficeService::m_xContext = remoteContext;

            // 直接赋值而不是再次进行UNO_QUERY转换，避免潜在的引用问题
            LibreOfficeService::m_xDesktop = desktop;

            // 确保desktop有效后再进行XComponentLoader转换
            if (desktop.is())
            {
                LibreOfficeService::m_xComponentLoader = Reference<XComponentLoader>(desktop, UNO_QUERY);

                // 验证ComponentLoader是否成功获取
                if (LibreOfficeService::m_xComponentLoader.is())
                {
                    LibreOfficeService::m_initialized = true;
                    logger_log_debug("LibreOffice service initialized successfully");
                }
                else
                {
                    logger_log_error("Failed to obtain XComponentLoader interface");
                    return false;
                }
            }
            else
            {
                logger_log_error("Desktop reference is invalid");
                return false;
            }
            return true;
        }
        catch (const Exception &e)
        {
            logger_log_error("UNO Exception: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
        catch (const std::exception &e)
        {
            logger_log_error("Standard Exception: %s", e.what());
            return false;
        }
        catch (...)
        {
            logger_log_error("Unknown Exception during LibreOffice connection");
            return false;
        }
    }

    bool LibreOfficeService::initialize()
    {
        // 如果已初始化，先检查连接是否有效
        if (LibreOfficeService::m_initialized && LibreOfficeService::isAvailable())
            return true;

        // 如果已初始化但连接无效，先清理资源
        if (LibreOfficeService::m_initialized)
        {
            logger_log_warn("Reinitializing LibreOffice service due to invalid connection...");
            LibreOfficeService::shutdown();
        }

        logger_log_info("Initializing LibreOffice service...");
        // 创建初始上下文
        try
        {
            // 首先需要创建服务管理器
            Reference<XComponentContext> localContext = cppu::defaultBootstrap_InitialComponentContext();
            Reference<XMultiComponentFactory> localServiceManager = localContext->getServiceManager();
            LibreOfficeService::m_xContext = localContext;
            LibreOfficeService::m_xServiceManager = localServiceManager;
        }
        catch (const cppu::BootstrapException &e)
        {
            logger_log_error("Bootstrap Exception: %s", rtl::OUStringToOString(e.getMessage(), RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
        catch (const Exception &e)
        {
            logger_log_error("Failed to create initial context: %s", rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return false;
        }
        catch (...)
        {
            logger_log_error("Unknown exception occurred during LibreOffice initialization");
            return false;
        }

        const int maxRetries = 3;
        const int retryDelay = 2000; // 2秒延迟

        for (int attempt = 1; attempt <= maxRetries; ++attempt)
        {
            if (LibreOfficeService::establishConnection())
            {
                logger_log_info("Successfully connected to LibreOffice");
                LibreOfficeService::m_initialized = true;
                
                // 初始化成功后，强制进行一次连接验证
                if (!LibreOfficeService::isAvailable())
                {
                    logger_log_error("Connection established but service is not available");
                    LibreOfficeService::shutdown();
                    return false;
                }
                
                logger_log_info("LibreOffice service initialized and connection verified successfully");
                return true;
            }

            if (attempt < maxRetries)
            {
                logger_log_info("Retrying connection in 2 seconds... (attempt %d/%d)", attempt, maxRetries);
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
        // 确保服务已初始化且连接有效
        if (!LibreOfficeService::initialize() || !LibreOfficeService::isAvailable())
        {
            logger_log_error("Cannot get component loader: service not initialized or connection invalid");
            return nullptr;
        }
        
        // 直接验证m_xComponentLoader是否有效，避免返回无效的组件加载器
        if (!LibreOfficeService::m_xComponentLoader.is())
        {
            logger_log_error("Component loader is not available");
            return nullptr;
        }
        
        return LibreOfficeService::m_xComponentLoader;
    }

    uno::Reference<uno::XComponentContext> LibreOfficeService::getContext()
    {
        if (!LibreOfficeService::initialize())
            return nullptr;
        return LibreOfficeService::m_xContext;
    }
    uno::Reference<sheet::XSpreadsheetDocument>
    LibreOfficeService::CreateNewSpreadsheetDocument()
    {
        // 多次尝试创建文档
        const int maxRetries = 3;
        const int retryDelay = 500;

        for (int attempt = 1; attempt <= maxRetries; ++attempt)
        {
            try
            {
                logger_log_debug("CreateNewSpreadsheetDocument attempt %d/%d...", attempt, maxRetries);

                // 1. 检查并确保服务已初始化且可用
                if (!LibreOfficeService::initialize())
                {
                    logger_log_error("Failed to initialize LibreOffice service");
                    // 如果是最后一次尝试，返回nullptr；否则继续重试
                    if (attempt >= maxRetries)
                        return nullptr;
                    continue;
                }

                // 2. 使用优化后的isAvailable方法检查连接有效性
                if (!LibreOfficeService::isAvailable())
                {
                    logger_log_error("LibreOffice service is not available");
                    // 重新初始化并继续尝试
                    LibreOfficeService::shutdown();
                    std::this_thread::sleep_for(std::chrono::milliseconds(retryDelay));
                    continue;
                }

                // 3. 准备创建文档的参数
                uno::Sequence<beans::PropertyValue> args(0);
                logger_log_debug("Creating new spreadsheet document...");

                // 4. 获取组件加载器
                auto componentLoader = LibreOfficeService::getComponentLoader();
                if (!componentLoader.is())
                {
                    logger_log_error("Invalid component loader");
                    // 重新初始化并继续尝试
                    LibreOfficeService::shutdown();
                    std::this_thread::sleep_for(std::chrono::milliseconds(retryDelay));
                    continue;
                }

                // 5. 尝试创建新文档
                uno::Reference<uno::XInterface> loadedIface;
                try
                {
                    loadedIface = componentLoader->loadComponentFromURL(
                        convertStringToOUString("private:factory/scalc"),
                        convertStringToOUString("_blank"),
                        0,
                        args);
                }
                catch (const uno::Exception &e)
                {
                    logger_log_error("Error loading spreadsheet component: %s",
                                     rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
                    // 检查是否是TransactionManager错误
                    std::string errorMsg = rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr();
                    if (errorMsg.find("TransactionManager") != std::string::npos ||
                        errorMsg.find("Owner instance already closed") != std::string::npos)
                    {
                        logger_log_warn("Detected TransactionManager error, forcing connection reset...");
                        // 强制重置连接
                        LibreOfficeService::shutdown();
                        std::this_thread::sleep_for(std::chrono::milliseconds(retryDelay * 2));
                        continue;
                    }
                    // 其他错误，直接返回
                    if (attempt >= maxRetries)
                        return nullptr;
                    continue;
                }

                if (!loadedIface.is())
                {
                    logger_log_error("CreateNewSpreadsheetDocument: loadComponentFromURL returned null!");
                    if (attempt >= maxRetries)
                        return nullptr;
                    continue;
                }

                // 6. 验证文档接口
                uno::Reference<lang::XComponent> xComp = uno::Reference<lang::XComponent>(loadedIface, uno::UNO_QUERY);
                if (!xComp.is())
                {
                    logger_log_error("CreateNewSpreadsheetDocument: loaded interface is not XComponent!");
                    if (attempt >= maxRetries)
                        return nullptr;
                    continue;
                }

                // 7. 转换为电子表格文档
                uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
                if (!xDoc.is())
                {
                    logger_log_error("CreateNewSpreadsheetDocument: loaded component is not a spreadsheet document!");
                    if (attempt >= maxRetries)
                        return nullptr;
                    continue;
                }

                logger_log_info("CreateNewSpreadsheetDocument: new spreadsheet document created successfully");
                return xDoc;
            }
            catch (const uno::Exception &e)
            {
                std::string errorMsg = rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr();
                logger_log_error("UNO Exception in CreateNewSpreadsheetDocument: %s", errorMsg.c_str());

                // 特别处理TransactionManager错误
                if (errorMsg.find("TransactionManager") != std::string::npos ||
                    errorMsg.find("Owner instance already closed") != std::string::npos)
                {
                    logger_log_warn("Detected TransactionManager error, forcing connection reset...");
                    // 强制重置连接
                    LibreOfficeService::shutdown();
                    std::this_thread::sleep_for(std::chrono::milliseconds(retryDelay * 2));
                }
                else
                {
                    // 其他UNO异常，如果是最后一次尝试则返回nullptr
                    if (attempt >= maxRetries)
                        return nullptr;
                    std::this_thread::sleep_for(std::chrono::milliseconds(retryDelay));
                }
            }
            catch (const std::exception &e)
            {
                logger_log_error("STD Exception in CreateNewSpreadsheetDocument: %s", e.what());
                return nullptr;
            }
            catch (...)
            {
                logger_log_error("Unknown exception in CreateNewSpreadsheetDocument");
                return nullptr;
            }
        }
        return nullptr;
    }
    uno::Reference<sheet::XSpreadsheetDocument> LibreOfficeService::loadSpreadsheetDocument(const rtl::OUString &filePath)
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

            auto loader = LibreOfficeService::getComponentLoader();
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

            // 确保文件有.ods扩展名并添加file://前缀
            std::string filePathStr = convertOUStringToString(filePath);
            std::string filePathWithExt = ensureOdsExtension(filePathStr);
            rtl::OUString url = convertStringToOUString(("file://" + filePathWithExt).c_str());
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
        // 参数验证
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

            // 保存到原始位置
            if (filePath.isEmpty())
            {
                logger_log_info("Saving document to original location...");
                xStorable->store();
                logger_log_info("Document saved to original location");
                return true;
            }

            // 保存到新位置
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

            // 尝试保存文档
            xStorable->storeAsURL(url, props);
            logger_log_info("Document saved successfully to: %s",
                            rtl::OUStringToOString(url, RTL_TEXTENCODING_UTF8).getStr());
            return true;
        }
        catch (const uno::Exception &e)
        {
            // 处理异常
            std::string errorMsg = rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr();
            logger_log_error("UNO Exception while saving document: %s", errorMsg.c_str());

            // 特别处理文件已存在错误（针对UNO单例模式的解决方案）
            if (errorMsg.find("AlreadyExists") != std::string::npos)
            {
                logger_log_warn("File already exists according to UNO. Attempting to reset UNO connection and retry...");

                // 转换URL为本地文件路径
                std::string localFilePath = rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr();

                // 1. 关闭当前文档以释放资源
                try
                {
                    logger_log_info("Closing current document to release resources...");
                    uno::Reference<lang::XComponent> xComponent(doc, uno::UNO_QUERY);
                    if (xComponent.is())
                    {
                        // 创建一个不询问的关闭参数
                        uno::Sequence<beans::PropertyValue> closeProps(1);
                        closeProps[0].Name = "SynchronMode";
                        closeProps[0].Value <<= true;

                        // 尝试使用XCloseable接口关闭
                        uno::Reference<util::XCloseable> xCloseable(xComponent, uno::UNO_QUERY);
                        if (xCloseable.is())
                        {
                            try
                            {
                                xCloseable->close(false); // false表示不询问保存
                            }
                            catch (const util::CloseVetoException &)
                            {
                                logger_log_warn("Document close vetoed, forcing termination...");
                                xComponent->dispose();
                            }
                        }
                        else
                        {
                            xComponent->dispose();
                        }
                        logger_log_info("Document closed successfully");
                    }
                }
                catch (const uno::Exception &closeEx)
                {
                    logger_log_error("Exception while closing document: %s",
                                     rtl::OUStringToOString(closeEx.Message, RTL_TEXTENCODING_UTF8).getStr());
                }

                // 2. 重新初始化UNO连接以刷新文件系统状态
                logger_log_info("Reinitializing UNO connection to refresh file system state...");
                LibreOfficeService::shutdown();

                // 增加更长的延迟时间，确保资源完全释放
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                // 3. 重新初始化连接并创建新文档
                try
                {
                    // 增加更彻底的连接重置逻辑
                    logger_log_info("Performing complete UNO connection reset...");

                    // 确保所有资源都被释放
                    LibreOfficeService::shutdown();

                    // 增加更长的延迟时间，确保LibreOffice服务完全重置
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

                    // 重新初始化连接
                    if (!LibreOfficeService::initialize())
                    {
                        logger_log_error("Failed to reinitialize UNO connection");
                        return false;
                    }
                    logger_log_info("UNO connection reinitialized successfully");

                    // 验证重新初始化后的连接状态
                    if (!LibreOfficeService::isAvailable())
                    {
                        logger_log_error("Reinitialized connection is not available");
                        return false;
                    }
                    logger_log_info("Reinitialized connection is available");

                    // 增加更充分的延迟确保连接稳定
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                    // 重新创建空文档
                    // 使用多次尝试策略
                    const int maxCreateAttempts = 3;
                    const int createRetryDelay = 500;
                    uno::Reference<sheet::XSpreadsheetDocument> newDoc;

                    for (int attempt = 1; attempt <= maxCreateAttempts; ++attempt)
                    {
                        logger_log_info("Attempt %d/%d to create new spreadsheet document...", attempt, maxCreateAttempts);
                        newDoc = LibreOfficeService::CreateNewSpreadsheetDocument();

                        if (newDoc.is())
                        {
                            logger_log_info("New empty spreadsheet created successfully");
                            break;
                        }

                        if (attempt < maxCreateAttempts)
                        {
                            logger_log_warn("Failed to create document, retrying in %dms...", createRetryDelay);
                            std::this_thread::sleep_for(std::chrono::milliseconds(createRetryDelay));
                            // 尝试再次初始化连接
                            LibreOfficeService::shutdown();
                            std::this_thread::sleep_for(std::chrono::milliseconds(500));
                            LibreOfficeService::initialize();
                        }
                    }

                    if (!newDoc.is())
                    {
                        logger_log_error("Failed to create new empty spreadsheet after %d attempts", maxCreateAttempts);
                        return false;
                    }
                    // 转换为通用XInterface类型以匹配saveDocument参数要求
                    uno::Reference<uno::XInterface> newDocInterface(newDoc, uno::UNO_QUERY);

                    // 将新文档保存到指定位置
                    // 使用单独的保存逻辑，避免递归调用可能导致的问题
                    try
                    {
                        logger_log_info("Attempting to save new document after reset...");
                        auto xStorable = uno::Reference<XStorable>(newDocInterface, uno::UNO_QUERY);
                        if (!xStorable.is())
                        {
                            logger_log_error("New document is not storable");
                            return false;
                        }

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
                        props[4].Value <<= true;

                        xStorable->storeAsURL(url, props);
                        logger_log_info("New document saved successfully after reset");
                        return true;
                    }
                    catch (const uno::Exception &saveEx)
                    {
                        logger_log_error("Failed to save new document after reset: %s",
                                         rtl::OUStringToOString(saveEx.Message, RTL_TEXTENCODING_UTF8).getStr());
                        // 如果重试也失败，关闭新创建的文档
                        try
                        {
                            LibreOfficeService::closeDocument(newDoc);
                        }
                        catch (...)
                        {
                            // 忽略关闭时的异常
                        }
                    }
                }
                catch (const uno::Exception &retryEx)
                {
                    logger_log_error("Retry UNO Exception while saving document after reset: %s",
                                     rtl::OUStringToOString(retryEx.Message, RTL_TEXTENCODING_UTF8).getStr());
                }
                catch (const std::exception &stdEx)
                {
                    logger_log_error("Retry STD Exception while saving document after reset: %s", stdEx.what());
                }
            }

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
    bool LibreOfficeService::isAvailable()
    {
        if (!LibreOfficeService::m_initialized)
        {
            logger_log_debug("Service not initialized");
            return false;
        }

        try
        {
            logger_log_debug("Checking service availability...");

            // 全面检查所有关键组件的有效性
            // 1. 检查上下文和服务管理器
            if (!LibreOfficeService::m_xContext.is())
            {
                logger_log_error("Context is not available");
                LibreOfficeService::m_initialized = false;
                return false;
            }

            auto mcf = LibreOfficeService::m_xContext->getServiceManager();
            if (!mcf.is())
            {
                logger_log_error("Service manager is not available");
                LibreOfficeService::m_initialized = false;
                return false;
            }

            // 2. 检查Desktop
            if (!LibreOfficeService::m_xDesktop.is())
            {
                logger_log_error("Desktop service is not available");
                LibreOfficeService::m_initialized = false;
                return false;
            }

            // 3. 检查ComponentLoader
            if (!LibreOfficeService::m_xComponentLoader.is())
            {
                logger_log_error("Component loader is not available");
                LibreOfficeService::m_initialized = false;
                return false;
            }

            // 4. 尝试一个简单的操作来验证连接的真正有效性
            try
            {
                // 尝试获取Desktop的状态来验证连接
                auto xComponent = Reference<lang::XComponent>(LibreOfficeService::m_xDesktop, UNO_QUERY);
                if (!xComponent.is())
                {
                    logger_log_error("Failed to get desktop component interface");
                    LibreOfficeService::m_initialized = false;
                    return false;
                }
            }
            catch (const uno::Exception &e)
            {
                logger_log_error("Connection validation failed: %s",
                                 rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
                LibreOfficeService::m_initialized = false;
                return false;
            }

            logger_log_debug("All service components are available");
            return true;
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO Exception while checking availability: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            LibreOfficeService::m_initialized = false;
            return false;
        }
        catch (const std::exception &e)
        {
            logger_log_error("STD Exception while checking availability: %s", e.what());
            LibreOfficeService::m_initialized = false;
            return false;
        }
        catch (...)
        {
            logger_log_error("Unknown exception while checking availability");
            LibreOfficeService::m_initialized = false;
            return false;
        }
    }
    // 加载文本文档
    uno::Reference<text::XTextDocument> LibreOfficeService::loadTextDocument(const rtl::OUString &filePath)
    {
        if (filePath.isEmpty())
        {
            logger_log_error("Empty file path provided");
            return nullptr;
        }

        try
        {
            logger_log_debug("Attempting to load text document: %s",
                             rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());

            auto loader = LibreOfficeService::getComponentLoader();
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

            auto xDoc = uno::Reference<text::XTextDocument>(xComponent, uno::UNO_QUERY);
            if (!xDoc.is())
            {
                closeDocument(xDoc);
                logger_log_error("Loaded file is not a text document: %s",
                                 rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
                return nullptr;
            }

            logger_log_info("Text document loaded successfully: %s",
                            rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
            return xDoc;
        }
        catch (const uno::Exception &e)
        {
            logger_log_error("UNO Exception while loading text document: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            return nullptr;
        }
        catch (const std::exception &e)
        {
            logger_log_error("STD Exception while loading text document: %s", e.what());
            return nullptr;
        }
        catch (...)
        {
            logger_log_error("Unknown exception while loading text document");
            return nullptr;
        }
    }

    // 另存文档
    bool LibreOfficeService::saveDocumentAs(const uno::Reference<frame::XModel> &document,
                                            const rtl::OUString &filePath)
    {
        if (!document.is())
        {
            logger_log_error("Invalid document reference");
            return false;
        }

        try
        {
            logger_log_info("Preparing to save document as...");
            auto xStorable = uno::Reference<XStorable>(document, uno::UNO_QUERY);
            if (!xStorable.is())
            {
                logger_log_error("Document is not storable");
                return false;
            }

            if (filePath.isEmpty())
            {
                logger_log_error("Empty file path provided");
                return false;
            }

            logger_log_info("Saving document to new location: %s",
                            rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());

            // 准备保存参数
            rtl::OUString url = convertStringToOUString("file://") + filePath;
            uno::Sequence<beans::PropertyValue> props(5);
            props[0].Name = "Overwrite";
            props[0].Value <<= true;
            props[1].Name = "FilterName";
            props[1].Value <<= convertStringToOUString("Writer Office Open XML");
            props[2].Name = "Compress";
            props[2].Value <<= true;
            props[3].Name = "Encrypt";
            props[3].Value <<= false;
            props[4].Name = "NoFileSync";
            props[4].Value <<= true; // 避免文件系统同步问题

            try
            {
                xStorable->storeAsURL(url, props);
                logger_log_info("Document saved successfully to: %s",
                                rtl::OUStringToOString(url, RTL_TEXTENCODING_UTF8).getStr());
                return true;
            }
            catch (const io::IOException &e)
            {
                // 处理文件已存在或权限问题
                logger_log_error("IO Exception while saving document: %s",
                                 rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
                return false;
            }
            catch (const uno::RuntimeException &e)
            {
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

    // 通用关闭文档方法
    bool LibreOfficeService::closeDocument(const uno::Reference<uno::XInterface> &doc)
    {
        if (!doc.is())
        {
            logger_log_error("Attempt to close null document reference");
            return false;
        }

        bool closedSuccessfully = false;

        const int maxRetries = 3;
        const int retryDelay = 500; // 0.5秒延迟

        for (int attempt = 1; attempt <= maxRetries; ++attempt)
        {
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
                            try
                            {
                                xCloseable->close(true); // 先尝试正常关闭
                                logger_log_info("Document closed gracefully");
                                closedSuccessfully = true;
                                return true; // 成功则直接返回
                            }
                            catch (const util::CloseVetoException &e)
                            {
                                logger_log_error("Close vetoed: %s, attempting forced close",
                                                 convertOUStringToString(e.Message).c_str());
                                try
                                {
                                    xCloseable->close(true);
                                    logger_log_info("Document forced closed successfully");
                                    closedSuccessfully = true;
                                    return true; // 成功则直接返回
                                }
                                catch (const uno::Exception &e)
                                {
                                    logger_log_error("Failed to force close document: %s",
                                                     convertOUStringToString(e.Message).c_str());
                                    // 继续执行外层异常处理
                                }
                            }
                            catch (const com::sun::star::uno::Exception &e)
                            {
                                logger_log_error("Failed to close document: %s",
                                                 convertOUStringToString(e.Message).c_str());
                            }
                        }

                        // 如果没有XCloseable接口，直接释放资源
                        try
                        {
                            xComponent->dispose();
                            logger_log_info("Document resources released - attempt %d/%d", attempt, maxRetries);
                            closedSuccessfully = true;
                            return true; // 成功则直接返回
                        }
                        catch (const uno::Exception &e)
                        {
                            logger_log_error("Failed to dispose document: %s",
                                             convertOUStringToString(e.Message).c_str());
                            // 继续重试
                        }
                    }
                    catch (const uno::Exception &e)
                    {
                        logger_log_error("UNO Exception while closing document: %s",
                                         convertOUStringToString(e.Message).c_str());
                        if (attempt >= maxRetries)
                            break;
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
                if (attempt >= maxRetries)
                    break;
#ifdef _WIN32
                Sleep(retryDelay);
#else
                usleep(retryDelay * 1000);
#endif
            }
            catch (const std::exception &e)
            {
                logger_log_error("STD Exception while closing document: %s", e.what());
                if (attempt >= maxRetries)
                    break;
#ifdef _WIN32
                Sleep(retryDelay);
#else
                usleep(retryDelay * 1000);
#endif
            }
            catch (...)
            {
                logger_log_error("Unknown exception while closing document");
                if (attempt >= maxRetries)
                    break;
#ifdef _WIN32
                Sleep(retryDelay);
#else
                usleep(retryDelay * 1000);
#endif
            }
        }

        logger_log_debug("Document closure completed with result: %s", closedSuccessfully ? "success" : "failed");
        return closedSuccessfully;
    }

    void LibreOfficeService::shutdown()
    {
        try
        {
            logger_log_info("Starting LibreOffice service shutdown...");

            // 按照正确的顺序清理资源（反向初始化顺序）
            // 1. 先释放组件加载器
            if (LibreOfficeService::m_xComponentLoader.is())
            {
                Reference<XComponent> xComp(LibreOfficeService::m_xComponentLoader, UNO_QUERY);
                if (xComp.is())
                {
                    try
                    {
                        logger_log_debug("Disposing component loader...");
                        xComp->dispose();
                    }
                    catch (const Exception &)
                    {
                        // 忽略清理过程中的异常，继续执行
                        logger_log_debug("Exception during component loader disposal");
                    }
                }
                LibreOfficeService::m_xComponentLoader.clear();
            }

            // 2. 释放Desktop
            if (LibreOfficeService::m_xDesktop.is())
            {
                Reference<XComponent> xComp(LibreOfficeService::m_xDesktop, UNO_QUERY);
                if (xComp.is())
                {
                    try
                    {
                        logger_log_debug("Disposing desktop...");
                        xComp->dispose();
                    }
                    catch (const Exception &)
                    {
                        // 忽略清理过程中的异常
                        logger_log_debug("Exception during desktop disposal");
                    }
                }
                LibreOfficeService::m_xDesktop.clear();
            }

            // 3. 释放ServiceManager
            if (LibreOfficeService::m_xServiceManager.is())
            {
                LibreOfficeService::m_xServiceManager.clear();
                logger_log_debug("Service manager cleared");
            }

            // 4. 最后释放上下文
            if (LibreOfficeService::m_xContext.is())
            {
                LibreOfficeService::m_xContext.clear();
                logger_log_debug("Context cleared");
            }

            // 重置初始化状态
            LibreOfficeService::m_initialized = false;

            // 添加短暂延迟确保资源完全释放，但处理可能的系统错误
            try
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            catch (const std::system_error& e)
            {
                // 当程序收到终止信号时，sleep操作可能会被中断，这是正常的
                logger_log_debug("Sleep interrupted during shutdown: %s", e.what());
            }

            logger_log_info("LibreOffice service shutdown completed");
        }
        catch (const Exception &e)
        {
            logger_log_error("Shutdown error: %s",
                             rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        }
        // 专门捕获libc++中的system_error异常
        catch (const std::__1::system_error& e)
        {
            logger_log_error("libc++ System error during shutdown: %s", e.what());
        }
        // 捕获标准库中的异常
        catch (const std::exception &e)
        {
            logger_log_error("STL Exception during shutdown: %s", e.what());
        }
        // 捕获所有其他异常
        catch (...)
        {
            logger_log_error("Unknown exception during LibreOffice shutdown");
        }
    }
}