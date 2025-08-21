#include "LibreOfficeService.h"
#include "../logger/logger.h"
#include "utils.h"
#include "ErrorHandler.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cppuhelper/bootstrap.hxx>
#include <com/sun/star/bridge/XUnoUrlResolver.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>

using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::frame;
using namespace com::sun::star::sheet;
using namespace com::sun::star::bridge;
using namespace com::sun::star::lang;
using namespace com::sun::star::container;
using namespace com::sun::star::beans;

namespace filemanager {

namespace {
    // 私有静态成员变量
    uno::Reference<uno::XComponentContext> s_context;
    uno::Reference<frame::XComponentLoader> s_loader;
    bool s_initialized = false;
    
    // 内部连接函数
    bool connectToLibreOffice() {
        const int maxRetries = 3;
        for (int attempt = 1; attempt <= maxRetries; ++attempt) {
            if (attempt > 1) {
                std::this_thread::sleep_for(std::chrono::seconds(attempt));
                logger_log_info("Retrying LibreOffice connection (attempt %d/%d)...", attempt, maxRetries);
            }

            try {
                // 初始化本地上下文
                auto localContext = cppu::defaultBootstrap_InitialComponentContext();
                if (!localContext.is()) {
                    logger_log_error("Failed to create local component context");
                    continue;
                }

                // 获取服务管理器
                auto localMCF = localContext->getServiceManager();
                if (!localMCF.is()) {
                    logger_log_error("Failed to get local service manager");
                    continue;
                }

                // 创建UnoUrlResolver
                auto resolver = localMCF->createInstanceWithContext(
                    convertStringToOUString("com.sun.star.bridge.UnoUrlResolver"), 
                    localContext);
                auto xUrlResolver = uno::Reference<XUnoUrlResolver>(resolver, uno::UNO_QUERY);
                if (!xUrlResolver.is()) {
                    logger_log_error("Failed to create UnoUrlResolver");
                    continue;
                }

                // 解析远程上下文
                auto ctxIface = xUrlResolver->resolve(
                    convertStringToOUString("uno:socket,host=127.0.0.1,port=2002;urp;StarOffice.ComponentContext"));
                auto context = uno::Reference<uno::XComponentContext>(ctxIface, uno::UNO_QUERY);
                if (!context.is()) {
                    logger_log_error("Failed to resolve LibreOffice context");
                    continue;
                }

                // 获取桌面服务
                auto mcf = context->getServiceManager();
                auto desktop = mcf->createInstanceWithContext(
                    convertStringToOUString("com.sun.star.frame.Desktop"), context);
                auto loader = uno::Reference<frame::XComponentLoader>(desktop, uno::UNO_QUERY);
                if (!loader.is()) {
                    logger_log_error("Failed to create Desktop instance");
                    continue;
                }

                // 更新静态成员
                s_context = context;
                s_loader = loader;
                s_initialized = true;
                logger_log_info("Successfully connected to LibreOffice service");
                return true;
            }
            catch (const uno::Exception& e) {
                logger_log_error("UNO Exception: %s", 
                    rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
            }
            catch (const std::exception& e) {
                logger_log_error("STD Exception: %s", e.what());
            }
            catch (...) {
                logger_log_error("Unknown exception during connection");
            }
        }
        return false;
    }
}

bool LibreOfficeService::initialize() {
    if (s_initialized) return true;
    return connectToLibreOffice();
}

uno::Reference<frame::XComponentLoader> LibreOfficeService::getComponentLoader() {
    if (!initialize()) return nullptr;
    return s_loader;
}

uno::Reference<sheet::XSpreadsheetDocument> 
LibreOfficeService::loadSpreadsheetDocument(const rtl::OUString& filePath) {
    try {
        auto loader = getComponentLoader();
        if (!loader.is()) {
            logger_log_error("Invalid component loader");
            return nullptr;
        }

        rtl::OUString url = convertStringToOUString("file://") + filePath;
        auto xComponent = loader->loadComponentFromURL(
            url, convertStringToOUString("_blank"), 0, uno::Sequence<beans::PropertyValue>());
        
        if (!xComponent.is()) {
            logger_log_error("Failed to load document: %s", 
                rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
            return nullptr;
        }

        auto xDoc = uno::Reference<sheet::XSpreadsheetDocument>(xComponent, uno::UNO_QUERY);
        if (!xDoc.is()) {
            closeDocument(xComponent);
            logger_log_error("Loaded file is not a spreadsheet: %s", 
                rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
            return nullptr;
        }

        return xDoc;
    }
    catch (const uno::Exception& e) {
        logger_log_error("UNO Exception: %s", 
            rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        return nullptr;
    }
}

uno::Reference<sheet::XSpreadsheet> 
LibreOfficeService::getSheet(const uno::Reference<sheet::XSpreadsheetDocument>& doc,
                            const rtl::OUString& sheetName) {
    try {
        if (!doc.is()) {
            logger_log_error("Invalid document reference");
            return nullptr;
        }

        auto sheets = doc->getSheets();
        if (!sheets.is()) {
            logger_log_error("Failed to get sheets collection");
            return nullptr;
        }

        auto nameAccess = uno::Reference<container::XNameAccess>(sheets, uno::UNO_QUERY);
        if (!nameAccess->hasByName(sheetName)) {
            logger_log_error("Sheet not found: %s", 
                rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr());
            return nullptr;
        }

        uno::Any sheetAny = nameAccess->getByName(sheetName);
        uno::Reference<sheet::XSpreadsheet> sheet;
        sheetAny >>= sheet;
        return sheet;
    }
    catch (const uno::Exception& e) {
        logger_log_error("UNO Exception: %s", 
            rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        return nullptr;
    }
}

bool LibreOfficeService::saveDocument(const uno::Reference<uno::XInterface>& doc,
                                     const rtl::OUString& filePath) {
    try {
        auto xStorable = uno::Reference<XStorable>(doc, uno::UNO_QUERY);
        if (!xStorable.is()) {
            logger_log_error("Document is not storable");
            return false;
        }

        if (filePath.isEmpty()) {
            // 直接保存到原始位置
            xStorable->store();
            logger_log_info("Document saved to original location");
            return true;
        }

        // 保存到指定路径
        rtl::OUString url = convertStringToOUString("file://") + filePath;
        uno::Sequence<beans::PropertyValue> props(2);
        props[0].Name = "Overwrite";
        props[0].Value <<= true;
        props[1].Name = "FilterName";
        props[1].Value <<= convertStringToOUString("calc8");

        xStorable->storeAsURL(url, props);
        logger_log_info("Document saved to: %s", 
            rtl::OUStringToOString(url, RTL_TEXTENCODING_UTF8).getStr());
        return true;
    }
    catch (const uno::Exception& e) {
        logger_log_error("UNO Exception: %s", 
            rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        return false;
    }
}

void LibreOfficeService::closeDocument(const uno::Reference<uno::XInterface>& doc) {
    try {
        auto xComponent = uno::Reference<lang::XComponent>(doc, uno::UNO_QUERY);
        if (xComponent.is()) {
            xComponent->dispose();
            logger_log_info("Document closed successfully");
        }
    }
    catch (const uno::Exception& e) {
        logger_log_error("UNO Exception while closing document: %s", 
            rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
    }
}

bool LibreOfficeService::isAvailable() {
    if (!s_initialized) return false;
    
    try {
        // 简单验证连接是否仍然有效
        auto mcf = s_context->getServiceManager();
        return mcf.is();
    }
    catch (...) {
        s_initialized = false;
        return false;
    }
}

void LibreOfficeService::shutdown() {
    s_loader = nullptr;
    s_context = nullptr;
    s_initialized = false;
    logger_log_info("LibreOffice service shutdown");
}
}