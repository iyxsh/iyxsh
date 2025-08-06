#include "filemanager.h"
#include "../cJSON/cJSON.h"
#include <com/sun/star/uno/Exception.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/sheet/XSpreadsheets.hpp>
#include <com/sun/star/table/XCell.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/bridge/UnoUrlResolver.hpp>
#include <cppuhelper/bootstrap.hxx>
#include <rtl/ustrbuf.hxx>
#include <rtl/ustring.hxx>
#include <stdexcept>
#include <iostream>
#include <cstdlib>
#if __cplusplus >= 201703L
#include <filesystem>
#endif
#include <sys/stat.h>
#include <sys/types.h>

using namespace com::sun::star;

namespace filemanager
{

    std::string columnIndexToName(int columnIndex)
    {
        std::string result;
        columnIndex++;
        while (columnIndex > 0)
        {
            int remainder = (columnIndex - 1) % 26;
            result = static_cast<char>('A' + remainder) + result;
            columnIndex = (columnIndex - 1) / 26;
        }
        return result;
    }

    static void parseCellAddress(const rtl::OUString &cellAddress, sal_Int32 &col, sal_Int32 &row)
    {
        col = 0;
        row = 0;
        int i = 0;
        // 解析列
        while (i < cellAddress.getLength() && iswalpha(cellAddress[i]))
        {
            col = col * 26 + (cellAddress[i] - 'A' + 1);
            ++i;
        }
        // 解析行
        while (i < cellAddress.getLength() && iswdigit(cellAddress[i]))
        {
            row = row * 10 + (cellAddress[i] - '0');
            ++i;
        }
        col -= 1;
        row -= 1;
    }

    cJSON *readCellToJson(const uno::Reference<uno::XInterface> &sheetIface, const rtl::OUString &cellAddress)
    {
        try
        {
            sal_Int32 col, row;
            parseCellAddress(cellAddress, col, row);
            uno::Reference<sheet::XSpreadsheet> sheet(sheetIface, uno::UNO_QUERY);
            if (!sheet.is())
                throw std::runtime_error("Invalid sheet interface");
            uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
            if (!cell.is())
                throw std::runtime_error("Cell not found");
            double value = cell->getValue();
            rtl::OUString formula = cell->getFormula();
            cJSON *json = cJSON_CreateObject();
            cJSON_AddStringToObject(json, "address", std::string(rtl::OUStringToOString(cellAddress, RTL_TEXTENCODING_UTF8).getStr()).c_str());
            cJSON_AddStringToObject(json, "formula", std::string(rtl::OUStringToOString(formula, RTL_TEXTENCODING_UTF8).getStr()).c_str());
            cJSON_AddNumberToObject(json, "value", value);
            return json;
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "readCellToJson error: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            return nullptr;
        }
    }

    // 递归创建目录（兼容无<filesystem>环境）
    static void make_dirs(const std::string& path) {
#if __cplusplus >= 201703L
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path), ec);
        if (ec) {
            std::cerr << "[saveDocument] Failed to create directory: " << path << " : " << ec.message() << std::endl;
        }
#else
        size_t pos = 0;
        do {
            pos = path.find_first_of("/\\", pos + 1);
            std::string sub = path.substr(0, pos);
            if (!sub.empty() && sub != "." && sub != "..") {
                mkdir(sub.c_str(), 0755);
            }
        } while (pos != std::string::npos);
#endif
    }

    void saveDocument(const uno::Reference<uno::XInterface> &docIface, const rtl::OUString &filePath)
    {
        try
        {
            uno::Reference<frame::XStorable> xStorable(docIface, uno::UNO_QUERY);
            if (!xStorable.is())
                throw std::runtime_error("Invalid storable interface");
            // 规范路径，自动创建父目录
            std::string nativePath = std::string(rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
            std::replace(nativePath.begin(), nativePath.end(), '\\', '/');
            size_t lastSlash = nativePath.find_last_of('/');
            if (lastSlash != std::string::npos) {
                std::string dir = nativePath.substr(0, lastSlash);
                make_dirs(dir);
            }
#if __cplusplus >= 201703L
            std::string urlPath = std::filesystem::path(nativePath).generic_string();
#else
            std::string urlPath = nativePath;
#endif
            rtl::OUString url = rtl::OUString::createFromAscii("file:///") + rtl::OUString::createFromAscii(urlPath.c_str());
            uno::Sequence<beans::PropertyValue> props(2);
            props[0].Name = rtl::OUString::createFromAscii("FilterName");
            props[0].Value <<= rtl::OUString::createFromAscii("calc8");
            props[1].Name = rtl::OUString::createFromAscii("Overwrite");
            props[1].Value <<= true;
            xStorable->storeToURL(url, props);
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "saveDocument error: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
        }
    }

    void closeDocument(const uno::Reference<uno::XInterface> &docIface)
    {
        try
        {
            uno::Reference<lang::XComponent> xComp(docIface, uno::UNO_QUERY);
            if (xComp.is())
                xComp->dispose();
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "closeDocument error: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
        }
    }

    cJSON *readSpreadsheetFile(const rtl::OUString &filePath)
    {
        try
        {
            uno::Reference<uno::XComponentContext> xContext = cppu::defaultBootstrap_InitialComponentContext();
            uno::Reference<lang::XMultiComponentFactory> xMCF_base = xContext->getServiceManager();
            uno::Reference<lang::XMultiServiceFactory> xMCF(xMCF_base, uno::UNO_QUERY);
            uno::Reference<frame::XComponentLoader> xLoader(xMCF->createInstance(
                                                                rtl::OUString::createFromAscii("com.sun.star.frame.Desktop")),
                                                            uno::UNO_QUERY);
            rtl::OUString url = rtl::OUString::createFromAscii("file:///") + filePath.replaceAll("\\", "/");
            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<lang::XComponent> xComp(xLoader->loadComponentFromURL(url, rtl::OUString::createFromAscii("_blank"), 0, args), uno::UNO_QUERY);
            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is())
                throw std::runtime_error("Cannot open spreadsheet");
            uno::Reference<sheet::XSpreadsheets> sheets = xDoc->getSheets();
            uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);
            cJSON *root = cJSON_CreateObject();
            uno::Sequence<rtl::OUString> names = nameAccess->getElementNames();
            for (sal_Int32 i = 0; i < names.getLength(); ++i)
            {
                uno::Any any = nameAccess->getByName(names[i]);
                uno::Reference<sheet::XSpreadsheet> sheet;
                any >>= sheet;
                cJSON *sheetJson = cJSON_CreateArray();
                for (sal_Int32 row = 0; row < 100; ++row)
                { // 只导出前100行
                    cJSON *rowJson = cJSON_CreateArray();
                    for (sal_Int32 col = 0; col < 20; ++col)
                    { // 只导出前20列
                        uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                        cJSON *cellJson = cJSON_CreateObject();
                        cJSON_AddNumberToObject(cellJson, "value", cell->getValue());
                        cJSON_AddStringToObject(cellJson, "formula", std::string(rtl::OUStringToOString(cell->getFormula(), RTL_TEXTENCODING_UTF8).getStr()).c_str());
                        cJSON_AddItemToArray(rowJson, cellJson);
                    }
                    cJSON_AddItemToArray(sheetJson, rowJson);
                }
                cJSON_AddItemToObject(root, std::string(rtl::OUStringToOString(names[i], RTL_TEXTENCODING_UTF8).getStr()).c_str(), sheetJson);
            }
            closeDocument(xComp);
            return root;
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "readSpreadsheetFile error: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            return nullptr;
        }
    }

    cJSON *createNewSpreadsheetFile(const rtl::OUString &filePath, const rtl::OUString &sheetName, const cJSON *contentData)
    {
        try
        {
            // 检查并输出 URE_BOOTSTRAP 环境变量
            const char *ure_bootstrap = std::getenv("URE_BOOTSTRAP");
            if (ure_bootstrap)
                std::cout << "[UNO] URE_BOOTSTRAP=" << ure_bootstrap << std::endl;
            else
                std::cout << "[UNO] URE_BOOTSTRAP not set!" << std::endl;
            // 通过 XUnoUrlResolver 远程连接外部 headless soffice 服务
            uno::Reference<uno::XComponentContext> xLocalContext = cppu::defaultBootstrap_InitialComponentContext();
            uno::Reference<lang::XMultiComponentFactory> xLocalMCF = xLocalContext->getServiceManager();
            uno::Reference<uno::XInterface> xResolverIface = xLocalMCF->createInstanceWithContext(
                rtl::OUString::createFromAscii("com.sun.star.bridge.UnoUrlResolver"), xLocalContext);
            uno::Reference<bridge::XUnoUrlResolver> xUrlResolver(xResolverIface, uno::UNO_QUERY);
            uno::Reference<uno::XInterface> xCtxIface;
            try {
                xCtxIface = xUrlResolver->resolve(
                    rtl::OUString::createFromAscii("uno:socket,host=127.0.0.1,port=2002;urp;StarOffice.ComponentContext"));
            } catch (const uno::Exception &e) {
                std::cerr << "[UNO] XUnoUrlResolver(socket) failed: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
                return nullptr;
            }
            uno::Reference<uno::XComponentContext> xContext(xCtxIface, uno::UNO_QUERY);
            if (!xContext.is()) {
                std::cerr << "[UNO] XComponentContext (socket) is null! 请确保已启动 soffice --headless --accept=\"socket,host=127.0.0.1,port=2002;urp;\"" << std::endl;
                return nullptr;
            }
            uno::Reference<lang::XMultiComponentFactory> xMCF_base = xContext->getServiceManager();
            if (!xMCF_base.is()) {
                std::cerr << "[UNO] getServiceManager() returned null!" << std::endl;
                return nullptr;
            }
            uno::Reference<lang::XMultiServiceFactory> xMCF(xMCF_base, uno::UNO_QUERY);
            uno::Reference<frame::XComponentLoader> xLoader(xMCF->createInstance(
                                                                rtl::OUString::createFromAscii("com.sun.star.frame.Desktop")),
                                                            uno::UNO_QUERY);
            if (!xLoader.is()) {
                std::cerr << "createNewSpreadsheetFile: XComponentLoader is null!" << std::endl;
                return nullptr;
            }
            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<uno::XInterface> loadedIface = xLoader->loadComponentFromURL(
                rtl::OUString::createFromAscii("private:factory/scalc"), rtl::OUString::createFromAscii("_blank"), 0, args);
            if (!loadedIface.is()) {
                std::cerr << "createNewSpreadsheetFile: loadComponentFromURL returned null!" << std::endl;
                return nullptr;
            }
            uno::Reference<lang::XComponent> xComp(loadedIface, uno::UNO_QUERY);
            if (!xComp.is()) {
                std::cerr << "createNewSpreadsheetFile: loaded interface is not XComponent!" << std::endl;
                return nullptr;
            }
            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is()) {
                std::cerr << "createNewSpreadsheetFile: loaded component is not a spreadsheet document!" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }
            uno::Reference<sheet::XSpreadsheets> sheets = xDoc->getSheets();
            uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);
            uno::Reference<sheet::XSpreadsheet> sheet;
            if (nameAccess->hasByName(sheetName)) {
                // 已存在则直接获取
                uno::Any any = nameAccess->getByName(sheetName);
                any >>= sheet;
            } else {
                // 不存在则插入
                sheets->insertNewByName(sheetName, 0);
                uno::Any any = nameAccess->getByName(sheetName);
                any >>= sheet;
            }
            // 写入内容
            if (sheet.is() && contentData && cJSON_IsArray(contentData))
            {
                int row = 0;
                cJSON *rowItem = nullptr;
                cJSON_ArrayForEach(rowItem, contentData)
                {
                    int col = 0;
                    cJSON *cellItem = nullptr;
                    cJSON_ArrayForEach(cellItem, rowItem)
                    {
                        if (cJSON_IsNumber(cellItem))
                            sheet->getCellByPosition(col, row)->setValue(cellItem->valuedouble);
                        else if (cJSON_IsString(cellItem)) {
                            // 检查cellItem是否包含非ASCII字符
                            bool hasNonAscii = false;
                            const char* str = cellItem->valuestring;
                            while (*str) {
                                if ((unsigned char)*str > 127) {
                                    hasNonAscii = true;
                                    break;
                                }
                                str++;
                            }
                            
                            // 如果包含非ASCII字符，使用UTF-8方式创建OUString
                            if (hasNonAscii) {
                                sheet->getCellByPosition(col, row)->setFormula(rtl::OStringToOUString(cellItem->valuestring, RTL_TEXTENCODING_UTF8));
                            } else {
                                sheet->getCellByPosition(col, row)->setFormula(rtl::OUString::createFromAscii(cellItem->valuestring));
                            }
                        }
                        ++col;
                    }
                    ++row;
                }
            }
            saveDocument(xDoc, filePath);
            closeDocument(xComp);
            return cJSON_CreateString("success");
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "createNewSpreadsheetFile error: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            return nullptr;
        }
    }

    cJSON *updateSpreadsheetContent(const rtl::OUString &filePath, const rtl::OUString &sheetName, const rtl::OUString &cellAddress, const rtl::OUString &newValue, const rtl::OUString &cellType)
    {
        try
        {
            // 通过 XUnoUrlResolver 远程连接外部 headless soffice 服务
            uno::Reference<uno::XComponentContext> xLocalContext = cppu::defaultBootstrap_InitialComponentContext();
            uno::Reference<lang::XMultiComponentFactory> xLocalMCF = xLocalContext->getServiceManager();
            uno::Reference<uno::XInterface> xResolverIface = xLocalMCF->createInstanceWithContext(
                rtl::OUString::createFromAscii("com.sun.star.bridge.UnoUrlResolver"), xLocalContext);
            uno::Reference<bridge::XUnoUrlResolver> xUrlResolver(xResolverIface, uno::UNO_QUERY);
            uno::Reference<uno::XInterface> xCtxIface;
            try {
                xCtxIface = xUrlResolver->resolve(
                    rtl::OUString::createFromAscii("uno:socket,host=127.0.0.1,port=2002;urp;StarOffice.ComponentContext"));
            } catch (const uno::Exception &e) {
                std::cerr << "[UNO] XUnoUrlResolver(socket) failed: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
                return nullptr;
            }
            uno::Reference<uno::XComponentContext> xContext(xCtxIface, uno::UNO_QUERY);
            if (!xContext.is()) {
                std::cerr << "[UNO] XComponentContext (socket) is null! 请确保已启动 soffice --headless --accept=\"socket,host=127.0.0.1,port=2002;urp;\"" << std::endl;
                return nullptr;
            }
            uno::Reference<lang::XMultiComponentFactory> xMCF_base = xContext->getServiceManager();
            if (!xMCF_base.is()) {
                std::cerr << "[UNO] getServiceManager() returned null!" << std::endl;
                return nullptr;
            }
            uno::Reference<lang::XMultiServiceFactory> xMCF(xMCF_base, uno::UNO_QUERY);
            uno::Reference<frame::XComponentLoader> xLoader(xMCF->createInstance(
                                                                rtl::OUString::createFromAscii("com.sun.star.frame.Desktop")),
                                                            uno::UNO_QUERY);
            if (!xLoader.is()) {
                std::cerr << "updateSpreadsheetContent: XComponentLoader is null!" << std::endl;
                return nullptr;
            }
            rtl::OUString url = rtl::OUString::createFromAscii("file:///") + filePath.replaceAll("\\", "/");
            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<lang::XComponent> xComp(xLoader->loadComponentFromURL(url, rtl::OUString::createFromAscii("_blank"), 0, args), uno::UNO_QUERY);
            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            uno::Reference<sheet::XSpreadsheet> sheet = getSheet(xDoc, sheetName);
            sal_Int32 col, row;
            parseCellAddress(cellAddress, col, row);
            uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
            if (cellType.equalsIgnoreAsciiCase("number"))
                cell->setValue(newValue.toDouble());
            else
            {
                logger_log("filePath: %s", rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
                logger_log("newValue: %s", rtl::OUStringToOString(newValue, RTL_TEXTENCODING_UTF8).getStr());
                //将newValue转为符合单元格公式的写法格式;
                rtl::OUString tmp = findCharPositions(newValue, readSheetData(filePath, rtl::OUString::createFromAscii("Sheet1")));
                logger_log("updateSpreadsheetContent: %s",rtl::OUStringToOString(tmp, RTL_TEXTENCODING_UTF8).getStr());
                cell->setFormula(tmp);
            }
            saveDocument(xDoc, filePath);
            closeDocument(xComp);
            return cJSON_CreateString("success");
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "updateSpreadsheetContent error: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            return nullptr;
        }
    }

    uno::Reference<sheet::XSpreadsheet> getSheet(const uno::Reference<sheet::XSpreadsheetDocument> &document, const rtl::OUString &sheetName)
    {
        if (!document.is())
            throw std::runtime_error("Invalid document reference");
        uno::Reference<sheet::XSpreadsheets> sheets = document->getSheets();
        uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);
        if (!nameAccess->hasByName(sheetName))
            throw std::runtime_error("Sheet not found");
        uno::Any any = nameAccess->getByName(sheetName);
        uno::Reference<sheet::XSpreadsheet> sheet;
        any >>= sheet;
        return sheet;
    }

    void newfileCreate(cJSON *results, const char *body)
    {
        printf("Creating new spreadsheet file with body: %s\n", body);
        // body 应为 JSON 字符串，包含文件名、sheet名、可选内容
        if (!body) {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }
        
        cJSON *root = cJSON_Parse(body);
        if (!root)
        {
            cJSON_AddStringToObject(results, "error", "Invalid JSON body");
            return;
        }
        cJSON *filenameItem = cJSON_GetObjectItem(root, "filename");
        cJSON *sheetnameItem = cJSON_GetObjectItem(root, "sheetname");
        cJSON *contentItem = cJSON_GetObjectItem(root, "content");
        if (!filenameItem || !cJSON_IsString(filenameItem) || !sheetnameItem || !cJSON_IsString(sheetnameItem))
        {
            cJSON_AddStringToObject(results, "error", "Missing filename or sheetname");
            cJSON_Delete(root);
            return;
        }
        
        // 使用UTF-8编码处理文件名和sheet名
        rtl::OUString filePath;
        rtl::OUString sheetName;
        
        // 检查filename是否包含非ASCII字符
        bool filenameHasNonAscii = false;
        const char* filenameStr = filenameItem->valuestring;
        while (*filenameStr) {
            if ((unsigned char)*filenameStr > 127) {
                filenameHasNonAscii = true;
                break;
            }
            filenameStr++;
        }
        if (filenameHasNonAscii) {
            filePath = rtl::OStringToOUString(filenameItem->valuestring, RTL_TEXTENCODING_UTF8);
        } else {
            filePath = rtl::OUString::createFromAscii(filenameItem->valuestring);
        }
        
        // 检查sheetname是否包含非ASCII字符
        bool sheetnameHasNonAscii = false;
        const char* sheetnameStr = sheetnameItem->valuestring;
        while (*sheetnameStr) {
            if ((unsigned char)*sheetnameStr > 127) {
                sheetnameHasNonAscii = true;
                break;
            }
            sheetnameStr++;
        }
        if (sheetnameHasNonAscii) {
            sheetName = rtl::OStringToOUString(sheetnameItem->valuestring, RTL_TEXTENCODING_UTF8);
        } else {
            sheetName = rtl::OUString::createFromAscii(sheetnameItem->valuestring);
        }
        
        // 如果有content，检查是否包含中文字符
        if (contentItem) {
            // 可以在这里添加对contentItem中中文字符的特殊处理
        }
        
        cJSON *createResult = filemanager::createNewSpreadsheetFile(filePath, sheetName, contentItem);
        if (createResult)
        {
            cJSON_AddItemToObject(results, "result", createResult);
        }
        else
        {
            cJSON_AddStringToObject(results, "error", "Failed to create spreadsheet file");
        }
        cJSON_Delete(root);
    }

    void updatefile(cJSON *results, const char *body)
    {
        // body 应为 JSON 字符串，支持单个或批量更新
        if (!body) {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }
        
        cJSON *root = cJSON_Parse(body);
        if (!root)
        {
            cJSON_AddStringToObject(results, "error", "Invalid JSON body");
            return;
        }
        // 支持批量：body为数组，或单对象
        if (cJSON_IsArray(root))
        {
            cJSON *item = nullptr;
            cJSON_ArrayForEach(item, root)
            {
                if (!cJSON_IsObject(item))
                    continue;
                cJSON *filenameItem = cJSON_GetObjectItem(item, "filename");
                cJSON *sheetnameItem = cJSON_GetObjectItem(item, "sheetname");
                cJSON *cellItem = cJSON_GetObjectItem(item, "cell");
                cJSON *valueItem = cJSON_GetObjectItem(item, "value");
                cJSON *typeItem = cJSON_GetObjectItem(item, "type");
                if (!filenameItem || !sheetnameItem || !cellItem || !valueItem || !typeItem ||
                    !cJSON_IsString(filenameItem) || !cJSON_IsString(sheetnameItem) || !cJSON_IsString(cellItem) ||
                    !cJSON_IsString(valueItem) || !cJSON_IsString(typeItem))
                {
                    cJSON *err = cJSON_CreateObject();
                    cJSON_AddStringToObject(err, "error", "Missing or invalid fields");
                    cJSON_AddItemToArray(results, err);
                    continue;
                }
                rtl::OUString filePath = rtl::OUString::createFromAscii(filenameItem->valuestring);
                rtl::OUString sheetName = rtl::OUString::createFromAscii(sheetnameItem->valuestring);
                rtl::OUString cellAddr = rtl::OUString::createFromAscii(cellItem->valuestring);
                rtl::OUString value = rtl::OUString::createFromAscii(valueItem->valuestring);
                rtl::OUString type = rtl::OUString::createFromAscii(typeItem->valuestring);
                
                // 如果值包含中文字符，需要特殊处理
                if (valueItem->valuestring) {
                    // 检查是否包含非ASCII字符（可能的中文字符）
                    bool hasNonAscii = false;
                    const char* str = valueItem->valuestring;
                    while (*str) {
                        if ((unsigned char)*str > 127) {
                            hasNonAscii = true;
                            break;
                        }
                        str++;
                    }
                    
                    // 如果包含非ASCII字符，使用RTL_TEXTENCODING_UTF8方式创建OUString
                    if (hasNonAscii) {
                        value = rtl::OStringToOUString(valueItem->valuestring, RTL_TEXTENCODING_UTF8);
                    }
                }
                
                cJSON *res = filemanager::updateSpreadsheetContent(filePath, sheetName, cellAddr, value, type);
                if (res)
                    cJSON_AddItemToArray(results, res);
                else
                {
                    cJSON *err = cJSON_CreateObject();
                    cJSON_AddStringToObject(err, "error", "Update failed");
                    cJSON_AddItemToArray(results, err);
                }
            }
        }
        else if (cJSON_IsObject(root))
        {
            cJSON *filenameItem = cJSON_GetObjectItem(root, "filename");
            cJSON *sheetnameItem = cJSON_GetObjectItem(root, "sheetname");
            cJSON *cellItem = cJSON_GetObjectItem(root, "cell");
            cJSON *valueItem = cJSON_GetObjectItem(root, "value");
            cJSON *typeItem = cJSON_GetObjectItem(root, "type");
            if (!filenameItem || !sheetnameItem || !cellItem || !valueItem || !typeItem ||
                !cJSON_IsString(filenameItem) || !cJSON_IsString(sheetnameItem) || !cJSON_IsString(cellItem) ||
                !cJSON_IsString(valueItem) || !cJSON_IsString(typeItem))
            {
                cJSON_AddStringToObject(results, "error", "Missing or invalid fields");
                cJSON_Delete(root);
                return;
            }
            rtl::OUString filePath = rtl::OUString::createFromAscii(filenameItem->valuestring);
            rtl::OUString sheetName = rtl::OUString::createFromAscii(sheetnameItem->valuestring);
            rtl::OUString cellAddr = rtl::OUString::createFromAscii(cellItem->valuestring);
            rtl::OUString value = rtl::OUString::createFromAscii(valueItem->valuestring);
            rtl::OUString type = rtl::OUString::createFromAscii(typeItem->valuestring);
            
            // 如果值包含中文字符，需要特殊处理
            if (valueItem->valuestring) {
                // 检查是否包含非ASCII字符（可能的中文字符）
                bool hasNonAscii = false;
                const char* str = valueItem->valuestring;
                while (*str) {
                    if ((unsigned char)*str > 127) {
                        hasNonAscii = true;
                        break;
                    }
                    str++;
                }
                
                // 如果包含非ASCII字符，使用RTL_TEXTENCODING_UTF8方式创建OUString
                if (hasNonAscii) {
                    value = rtl::OStringToOUString(valueItem->valuestring, RTL_TEXTENCODING_UTF8);
                }
            }
            
            cJSON *res = filemanager::updateSpreadsheetContent(filePath, sheetName, cellAddr, value, type);
            if (res)
                cJSON_AddItemToArray(results, res);
            else
                cJSON_AddStringToObject(results, "error", "Update failed");
        }
        else
        {
            cJSON_AddStringToObject(results, "error", "Invalid JSON structure");
        }
        cJSON_Delete(root);
    }

    void editfile(cJSON *results, const char *body)
    {
        // body 应为 JSON 字符串，支持批量编辑（如批量写入单元格）
        // 这里实现与 updatefile 类似，支持多单元格写入
        updatefile(results, body);
    }
    
    cJSON *findValueInSheet(const rtl::OUString &filePath, const rtl::OUString &sheetName, const rtl::OUString &searchValue)
    {
        try
        {
            // 通过 XUnoUrlResolver 远程连接外部 headless soffice 服务
            uno::Reference<uno::XComponentContext> xLocalContext = cppu::defaultBootstrap_InitialComponentContext();
            uno::Reference<lang::XMultiComponentFactory> xLocalMCF = xLocalContext->getServiceManager();
            uno::Reference<uno::XInterface> xResolverIface = xLocalMCF->createInstanceWithContext(
                rtl::OUString::createFromAscii("com.sun.star.bridge.UnoUrlResolver"), xLocalContext);
            uno::Reference<bridge::XUnoUrlResolver> xUrlResolver(xResolverIface, uno::UNO_QUERY);
            uno::Reference<uno::XInterface> xCtxIface;
            try {
                xCtxIface = xUrlResolver->resolve(
                    rtl::OUString::createFromAscii("uno:socket,host=127.0.0.1,port=2002;urp;StarOffice.ComponentContext"));
            } catch (const uno::Exception &e) {
                std::cerr << "[UNO] XUnoUrlResolver(socket) failed: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
                return nullptr;
            }
            uno::Reference<uno::XComponentContext> xContext(xCtxIface, uno::UNO_QUERY);
            if (!xContext.is()) {
                std::cerr << "[UNO] XComponentContext (socket) is null! 请确保已启动 soffice --headless --accept=\"socket,host=127.0.0.1,port=2002;urp;\"" << std::endl;
                return nullptr;
            }
            uno::Reference<lang::XMultiComponentFactory> xMCF_base = xContext->getServiceManager();
            if (!xMCF_base.is()) {
                std::cerr << "[UNO] getServiceManager() returned null!" << std::endl;
                return nullptr;
            }
            uno::Reference<lang::XMultiServiceFactory> xMCF(xMCF_base, uno::UNO_QUERY);
            uno::Reference<frame::XComponentLoader> xLoader(xMCF->createInstance(
                                                                rtl::OUString::createFromAscii("com.sun.star.frame.Desktop")),
                                                            uno::UNO_QUERY);
            if (!xLoader.is()) {
                std::cerr << "findValueInSheet: XComponentLoader is null!" << std::endl;
                return nullptr;
            }
            rtl::OUString url = rtl::OUString::createFromAscii("file:///") + filePath.replaceAll("\\", "/");
            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<lang::XComponent> xComp(xLoader->loadComponentFromURL(url, rtl::OUString::createFromAscii("_blank"), 0, args), uno::UNO_QUERY);
            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is()) {
                std::cerr << "findValueInSheet: Cannot open spreadsheet!" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }
            
            uno::Reference<sheet::XSpreadsheet> sheet = getSheet(xDoc, sheetName);
            if (!sheet.is()) {
                std::cerr << "findValueInSheet: Cannot get sheet!" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }
            
            // 创建返回的 cJSON 数组，用于存储所有匹配的位置
            cJSON *positions = cJSON_CreateArray();
            
            // 在固定范围内搜索（前100行，前20列）
            for (sal_Int32 row = 0; row < 100; ++row) {
                for (sal_Int32 col = 0; col < 20; ++col) {
                    uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                    if (!cell.is()) {
                        continue;
                    }
                    
                    // 获取单元格的值和公式
                    double cellValue = cell->getValue();
                    rtl::OUString cellFormula = cell->getFormula();
                    
                    // 检查单元格值是否匹配搜索值
                    bool found = false;
                    if (searchValue.getLength() == 0 && (cellValue == 0.0 && cellFormula.getLength() == 0)) {
                        // 查找空单元格
                        found = true;
                    } else if (cellFormula.getLength() > 0) {
                        // 如果有公式，比较公式文本
                        if (cellFormula.equals(searchValue)) {
                            found = true;
                        }
                    } else {
                        // 比较数值或文本值
                        rtl::OUString cellText = rtl::OUString::number(cellValue);
                        if (cellText.equals(searchValue)) {
                            found = true;
                        }
                    }
                    
                    if (found) {
                        // 计算列名 (A, B, ..., Z, AA, AB, ...)
                        std::string columnName;
                        int column = col;
                        do {
                            columnName = static_cast<char>('A' + (column % 26)) + columnName;
                            column = (column / 26) - 1;
                        } while (column >= 0);
                        
                        // 计算行号 (1-based)
                        int rowNumber = row + 1;
                        
                        // 创建位置字符串，如 "A1", "B2" 等
                        std::string position = columnName + std::to_string(rowNumber);
                        
                        // 添加到结果数组
                        cJSON *pos = cJSON_CreateString(position.c_str());
                        cJSON_AddItemToArray(positions, pos);
                    }
                }
            }
            
            closeDocument(xComp);
            return positions;
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "findValueInSheet error: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            return nullptr;
        }
        catch (const std::exception &e)
        {
            std::cerr << "findValueInSheet error: " << e.what() << std::endl;
            return nullptr;
        }
    }

    void findInSheet(cJSON *results, const char *body)
    {
        // 解析输入的 JSON 字符串
        if (!body) {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }
        
        cJSON *root = cJSON_Parse(body);
        if (!root)
        {
            cJSON_AddStringToObject(results, "error", "Invalid JSON body");
            return;
        }
        
        cJSON *filenameItem = cJSON_GetObjectItem(root, "filename");
        cJSON *sheetnameItem = cJSON_GetObjectItem(root, "sheetname");
        cJSON *valueItem = cJSON_GetObjectItem(root, "value");
        
        if (!filenameItem || !cJSON_IsString(filenameItem) || 
            !sheetnameItem || !cJSON_IsString(sheetnameItem) ||
            !valueItem || !cJSON_IsString(valueItem))
        {
            cJSON_AddStringToObject(results, "error", "Missing filename, sheetname or value");
            cJSON_Delete(root);
            return;
        }
        
        // 使用UTF-8编码处理文件名和sheet名
        rtl::OUString filePath;
        rtl::OUString sheetName;
        rtl::OUString searchValue;
        
        // 检查filename是否包含非ASCII字符
        bool filenameHasNonAscii = false;
        const char* filenameStr = filenameItem->valuestring;
        while (*filenameStr) {
            if ((unsigned char)*filenameStr > 127) {
                filenameHasNonAscii = true;
                break;
            }
            filenameStr++;
        }
        if (filenameHasNonAscii) {
            filePath = rtl::OStringToOUString(filenameItem->valuestring, RTL_TEXTENCODING_UTF8);
        } else {
            filePath = rtl::OUString::createFromAscii(filenameItem->valuestring);
        }
        
        // 检查sheetname是否包含非ASCII字符
        bool sheetnameHasNonAscii = false;
        const char* sheetnameStr = sheetnameItem->valuestring;
        while (*sheetnameStr) {
            if ((unsigned char)*sheetnameStr > 127) {
                sheetnameHasNonAscii = true;
                break;
            }
            sheetnameStr++;
        }
        if (sheetnameHasNonAscii) {
            sheetName = rtl::OStringToOUString(sheetnameItem->valuestring, RTL_TEXTENCODING_UTF8);
        } else {
            sheetName = rtl::OUString::createFromAscii(sheetnameItem->valuestring);
        }
        
        // 检查搜索值是否包含非ASCII字符
        bool valueHasNonAscii = false;
        const char* valueStr = valueItem->valuestring;
        while (*valueStr) {
            if ((unsigned char)*valueStr > 127) {
                valueHasNonAscii = true;
                break;
            }
            valueStr++;
        }
        if (valueHasNonAscii) {
            searchValue = rtl::OStringToOUString(valueItem->valuestring, RTL_TEXTENCODING_UTF8);
        } else {
            searchValue = rtl::OUString::createFromAscii(valueItem->valuestring);
        }
        
        cJSON *positions = filemanager::findValueInSheet(filePath, sheetName, searchValue);
        if (positions)
        {
            cJSON_AddItemToObject(results, "positions", positions);
        }
        else
        {
            cJSON_AddStringToObject(results, "error", "Failed to search in spreadsheet");
        }
        
        cJSON_Delete(root);
    }
    
    cJSON *readSheetData(const rtl::OUString &filePath, const rtl::OUString &sheetName)
    {
        logger_log("filePath: %s", rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
        logger_log("sheetName: %s", rtl::OUStringToOString(sheetName, RTL_TEXTENCODING_UTF8).getStr());
        try
        {
            // 通过 XUnoUrlResolver 远程连接外部 headless soffice 服务
            uno::Reference<uno::XComponentContext> xLocalContext = cppu::defaultBootstrap_InitialComponentContext();
            uno::Reference<lang::XMultiComponentFactory> xLocalMCF = xLocalContext->getServiceManager();
            uno::Reference<uno::XInterface> xResolverIface = xLocalMCF->createInstanceWithContext(
                rtl::OUString::createFromAscii("com.sun.star.bridge.UnoUrlResolver"), xLocalContext);
            uno::Reference<bridge::XUnoUrlResolver> xUrlResolver(xResolverIface, uno::UNO_QUERY);
            uno::Reference<uno::XInterface> xCtxIface;
            try {
                xCtxIface = xUrlResolver->resolve(
                    rtl::OUString::createFromAscii("uno:socket,host=127.0.0.1,port=2002;urp;StarOffice.ComponentContext"));
            } catch (const uno::Exception &e) {
                std::cerr << "[UNO] XUnoUrlResolver(socket) failed: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
                return nullptr;
            }
            uno::Reference<uno::XComponentContext> xContext(xCtxIface, uno::UNO_QUERY);
            if (!xContext.is()) {
                std::cerr << "[UNO] XComponentContext (socket) is null! 请确保已启动 soffice --headless --accept=\"socket,host=127.0.0.1,port=2002;urp;\"" << std::endl;
                return nullptr;
            }
            uno::Reference<lang::XMultiComponentFactory> xMCF_base = xContext->getServiceManager();
            if (!xMCF_base.is()) {
                std::cerr << "[UNO] getServiceManager() returned null!" << std::endl;
                return nullptr;
            }
            uno::Reference<lang::XMultiServiceFactory> xMCF(xMCF_base, uno::UNO_QUERY);
            uno::Reference<frame::XComponentLoader> xLoader(xMCF->createInstance(
                                                                rtl::OUString::createFromAscii("com.sun.star.frame.Desktop")),
                                                            uno::UNO_QUERY);
            if (!xLoader.is()) {
                std::cerr << "readSheetData: XComponentLoader is null!" << std::endl;
                return nullptr;
            }
            rtl::OUString url = rtl::OUString::createFromAscii("file:///") + filePath.replaceAll("\\", "/");
            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<lang::XComponent> xComp(xLoader->loadComponentFromURL(url, rtl::OUString::createFromAscii("_blank"), 0, args), uno::UNO_QUERY);
            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is()) {
                std::cerr << "readSheetData: Cannot open spreadsheet!" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }
            
            uno::Reference<sheet::XSpreadsheet> sheet = getSheet(xDoc, sheetName);
            if (!sheet.is()) {
                std::cerr << "readSheetData: Cannot get sheet!" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }
            
            // 创建返回的 cJSON 对象，用于存储单元格内容和位置的映射
            cJSON *contentMap = cJSON_CreateObject();
            
            // 创建一个用于跟踪已添加内容的辅助对象，确保内容不重复
            cJSON *addedContents = cJSON_CreateObject();
            
            // 在固定范围内读取（前100行，前20列）
            for (sal_Int32 row = 0; row < 100; ++row) {
                for (sal_Int32 col = 0; col < 20; ++col) {
                    uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                    if (!cell.is()) {
                        continue;
                    }
                    
                    // 获取单元格的值和公式
                    double cellValue = cell->getValue();
                    rtl::OUString cellFormula = cell->getFormula();
                    
                    // 检查单元格是否为空
                    if (cellValue == 0.0 && cellFormula.getLength() == 0) {
                        continue; // 跳过空单元格
                    }
                    
                    // 获取单元格内容
                    rtl::OUString cellContent;
                    if (cellFormula.getLength() > 0) {
                        // 如果有公式，使用公式文本
                        cellContent = cellFormula;
                    } else {
                        // 否则使用数值或文本值
                        cellContent = rtl::OUString::number(cellValue);
                    }
                    
                    // 将内容转换为字符串用于比较
                    std::string contentStr = rtl::OUStringToOString(cellContent, RTL_TEXTENCODING_UTF8).getStr();
                    
                    // 检查内容是否已经添加过（避免重复）
                    if (cJSON_GetObjectItem(addedContents, contentStr.c_str()) != nullptr) {
                        continue; // 内容已存在，跳过
                    }
                    
                    // 计算列名 (A, B, ..., Z, AA, AB, ...)
                    std::string columnName;
                    int column = col;
                    do {
                        columnName = static_cast<char>('A' + (column % 26)) + columnName;
                        column = (column / 26) - 1;
                    } while (column >= 0);
                    
                    // 计算行号 (1-based)
                    int rowNumber = row + 1;
                    
                    // 创建位置字符串，如 "A1", "B2" 等
                    std::string position = columnName + std::to_string(rowNumber);
                    
                    // 添加内容到结果对象（内容作为key，位置作为value）
                    cJSON_AddStringToObject(contentMap, contentStr.c_str(), position.c_str());
                    
                    // 标记内容已添加
                    cJSON_AddStringToObject(addedContents, contentStr.c_str(), "added");
                }
            }
            
            // 清理辅助对象
            cJSON_Delete(addedContents);
            
            closeDocument(xComp);
            return contentMap;
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "readSheetData error: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            return nullptr;
        }
        catch (const std::exception &e)
        {
            std::cerr << "readSheetData error: " << e.what() << std::endl;
            return nullptr;
        }
    }
    
    void readSheetContents(cJSON *results, const char *body)
    {
        // 解析输入的 JSON 字符串
        if (!body) {
            cJSON_AddStringToObject(results, "error", "Empty request body");
            return;
        }
        
        cJSON *root = cJSON_Parse(body);
        if (!root)
        {
            cJSON_AddStringToObject(results, "error", "Invalid JSON body");
            return;
        }
        
        cJSON *filenameItem = cJSON_GetObjectItem(root, "filename");
        cJSON *sheetnameItem = cJSON_GetObjectItem(root, "sheetname");
        
        if (!filenameItem || !cJSON_IsString(filenameItem) || 
            !sheetnameItem || !cJSON_IsString(sheetnameItem))
        {
            cJSON_AddStringToObject(results, "error", "Missing filename or sheetname");
            cJSON_Delete(root);
            return;
        }
        
        // 使用UTF-8编码处理文件名和sheet名
        rtl::OUString filePath;
        rtl::OUString sheetName;
        
        // 检查filename是否包含非ASCII字符
        bool filenameHasNonAscii = false;
        const char* filenameStr = filenameItem->valuestring;
        while (*filenameStr) {
            if ((unsigned char)*filenameStr > 127) {
                filenameHasNonAscii = true;
                break;
            }
            filenameStr++;
        }
        if (filenameHasNonAscii) {
            filePath = rtl::OStringToOUString(filenameItem->valuestring, RTL_TEXTENCODING_UTF8);
        } else {
            filePath = rtl::OUString::createFromAscii(filenameItem->valuestring);
        }
        
        // 检查sheetname是否包含非ASCII字符
        bool sheetnameHasNonAscii = false;
        const char* sheetnameStr = sheetnameItem->valuestring;
        while (*sheetnameStr) {
            if ((unsigned char)*sheetnameStr > 127) {
                sheetnameHasNonAscii = true;
                break;
            }
            sheetnameStr++;
        }
        if (sheetnameHasNonAscii) {
            sheetName = rtl::OStringToOUString(sheetnameItem->valuestring, RTL_TEXTENCODING_UTF8);
        } else {
            sheetName = rtl::OUString::createFromAscii(sheetnameItem->valuestring);
        }
        
        cJSON *contentMap = filemanager::readSheetData(filePath, sheetName);
        if (contentMap)
        {
            cJSON_AddItemToObject(results, "contents", contentMap);
        }
        else
        {
            cJSON_AddStringToObject(results, "error", "Failed to read spreadsheet contents");
        }
        
        cJSON_Delete(root);
    }
    
    rtl::OUString findCharPositions(const rtl::OUString &newValue, cJSON *sheetData)
    {
        logger_log("newValue: %s", rtl::OUStringToOString(newValue, RTL_TEXTENCODING_UTF8).getStr());
        logger_log("sheetData:%s", cJSON_Print(sheetData));
        // 创建一个OUStringBuffer来构建结果字符串
        rtl::OUStringBuffer resultBuffer;
        
        // 遍历输入字符串中的每个字符
        for (sal_Int32 i = 0; i < newValue.getLength(); ++i) {
            // 获取当前字符
            rtl::OUString charStr = newValue.copy(i, 1);
            logger_log("charStr: %s", rtl::OUStringToOString(charStr, RTL_TEXTENCODING_UTF8).getStr());
            // 将字符转换为UTF8字符串用于在sheetData中查找
            std::string charStdStr = rtl::OUStringToOString(charStr, RTL_TEXTENCODING_UTF8).getStr();
            logger_log("charStdStr: %s", charStdStr.c_str());
            // 在sheetData中查找该字符
            cJSON *positionItem = cJSON_GetObjectItem(sheetData, charStdStr.c_str());
            
            if (positionItem && positionItem->valuestring) {
                logger_log("positionItem: %s", positionItem->valuestring);
                // 如果找到位置信息，添加到结果中
                if (i > 0) {
                    // 除了第一个字符外，其他字符前面都加&
                    resultBuffer.append(rtl::OUString::createFromAscii("&"));
                }
                else
                {
                    resultBuffer.append(rtl::OUString::createFromAscii("="));
                }
                resultBuffer.append(rtl::OUString::createFromAscii("$Sheet1."));
                resultBuffer.append(rtl::OUString::createFromAscii(positionItem->valuestring));
            } else {
                // 如果未找到字符位置，可以添加一个占位符或跳过
                if (i > 0 && resultBuffer.getLength() > 0) {
                    resultBuffer.append(rtl::OUString::createFromAscii("&"));
                }
                resultBuffer.append(rtl::OUString::createFromAscii("N/A"));
            }
        }
        // 返回组合的位置字符串，此处使用后会清空
        return resultBuffer.makeStringAndClear();
    }

} // namespace filemanager