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
#include <ctime>
#if __cplusplus >= 201703L
#include <filesystem>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace com::sun::star;

namespace filemanager
{
    // 内部辅助函数声明
    static uno::Reference<uno::XComponentContext> connectToLibreOffice();
    static std::string convertToAbsolutePath(const std::string &path);
    static rtl::OUString convertStringToOUString(const char *str);
    static void make_dirs(const std::string &path);
    static uno::Reference<frame::XComponentLoader> getComponentLoader();
    static void writeCellContent(const uno::Reference<sheet::XSpreadsheet> &sheet, int col, int row, cJSON *cellItem);

    // 自动检测文本内容的语言
    static std::string detectLanguage(const rtl::OUString &text)
    {
        // 获取文本长度
        sal_Int32 length = text.getLength();
        if (length == 0)
        {
            return "unknown";
        }

        // 统计各类字符
        int chineseChars = 0;
        int englishChars = 0;
        int digits = 0;
        int spaces = 0;
        int others = 0;

        for (sal_Int32 i = 0; i < length; ++i)
        {
            sal_Unicode c = text[i];

            // 中文字符范围判断（基本汉字）
            // 注意：sal_Unicode 是 unsigned short 类型，范围是 0x0000-0xFFFF
            // 所以只检查基本平面的中文字符范围
            if ((c >= 0x4E00 && c <= 0x9FFF) ||   // CJK统一汉字
                (c >= 0x3400 && c <= 0x4DBF))
            {  // CJK扩展A
                chineseChars++;
            }
            // 英文字母
            else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
            {
                englishChars++;
            }
            // 数字
            else if (c >= '0' && c <= '9')
            {
                digits++;
            }
            // 空格和制表符
            else if (c == ' ' || c == '\t')
            {
                spaces++;
            }
            // 其他字符
            else
            {
                others++;
            }
        }

        // 判断主要语言
        if (chineseChars > 0 && (chineseChars + others == length))
        {
            return "zh"; // 纯中文（包括标点符号等其他字符）
        }
        else if (englishChars > 0 && (englishChars + others == length))
        {
            return "en"; // 纯英文（包括标点符号等其他字符）
        }
        else if (digits > 0 && (digits + others == length))
        {
            return "num"; // 纯数字（包括标点符号等其他字符）
        }
        else if (spaces > 0 && (spaces == length))
        {
            return "space"; // 纯空格
        }
        else if (chineseChars > 0 && englishChars > 0)
        {
            return "zh-en"; // 中英文混合
        }
        else if (chineseChars > 0 && digits > 0)
        {
            return "zh-num"; // 中文数字混合
        }
        else if (englishChars > 0 && digits > 0)
        {
            return "en-num"; // 英文数字混合
        }
        else
        {
            return "other"; // 其他混合或无法识别的语言
        }
    }
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
    static void make_dirs(const std::string &path)
    {
#if __cplusplus >= 201703L
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path), ec);
        if (ec)
        {
            std::cerr << "[saveDocument] Failed to create directory: " << path << " : " << ec.message() << std::endl;
        }
#else
        size_t pos = 0;
        do
        {
            pos = path.find_first_of("/\\", pos + 1);
            std::string sub = path.substr(0, pos);
            if (!sub.empty() && sub != "." && sub != "..")
            {
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
            {
                std::cerr << "saveDocument error: Invalid storable interface" << std::endl;
                return;
            }

            // 规范路径，自动创建父目录
            std::string nativePath = std::string(rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
            std::cerr << "saveDocument: Saving to native path: " << nativePath << std::endl;

            // 标准化路径分隔符
            std::replace(nativePath.begin(), nativePath.end(), '\\', '/');

            // 处理相对路径
            std::string absolutePath = convertToAbsolutePath(nativePath);
            std::cerr << "saveDocument: Absolute path: " << absolutePath << std::endl;

            // 创建父目录
            size_t lastSlash = absolutePath.find_last_of('/');
            if (lastSlash != std::string::npos)
            {
                std::string dir = absolutePath.substr(0, lastSlash);
                std::cerr << "saveDocument: Creating directories for: " << dir << std::endl;
                make_dirs(dir);
            }

#if __cplusplus >= 201703L
            std::string urlPath = std::filesystem::path(absolutePath).generic_string();
#else
            std::string urlPath = absolutePath;
#endif
            std::cerr << "saveDocument: URL path: " << urlPath << std::endl;

            rtl::OUString url = rtl::OUString::createFromAscii("file:///") + rtl::OUString::createFromAscii(urlPath.c_str());
            std::cerr << "saveDocument: Full URL: " << rtl::OUStringToOString(url, RTL_TEXTENCODING_UTF8).getStr() << std::endl;

            uno::Sequence<beans::PropertyValue> props(2);
            props[0].Name = rtl::OUString::createFromAscii("FilterName");
            props[0].Value <<= rtl::OUString::createFromAscii("calc8");
            props[1].Name = rtl::OUString::createFromAscii("Overwrite");
            props[1].Value <<= true;

            xStorable->storeToURL(url, props);
            std::cerr << "saveDocument: File saved successfully" << std::endl;
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "saveDocument UNO error: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "saveDocument std error: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "saveDocument unknown error occurred" << std::endl;
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

    // 连接到LibreOffice服务
    static uno::Reference<uno::XComponentContext> connectToLibreOffice()
    {
        // 检查并输出 URE_BOOTSTRAP 环境变量
        const char *ure_bootstrap = std::getenv("URE_BOOTSTRAP");
        if (ure_bootstrap)
            std::cout << "[UNO] URE_BOOTSTRAP=" << ure_bootstrap << std::endl;
        else
            std::cout << "[UNO] URE_BOOTSTRAP not set!" << std::endl;

        // 通过 XUnoUrlResolver 远程连接外部 headless soffice 服务
        uno::Reference<uno::XComponentContext> xLocalContext = cppu::defaultBootstrap_InitialComponentContext();
        if (!xLocalContext.is())
        {
            std::cerr << "connectToLibreOffice: Failed to create local component context" << std::endl;
            return nullptr;
        }

        uno::Reference<lang::XMultiComponentFactory> xLocalMCF = xLocalContext->getServiceManager();
        if (!xLocalMCF.is())
        {
            std::cerr << "connectToLibreOffice: Failed to get local service manager" << std::endl;
            return nullptr;
        }

        uno::Reference<uno::XInterface> xResolverIface = xLocalMCF->createInstanceWithContext(
            rtl::OUString::createFromAscii("com.sun.star.bridge.UnoUrlResolver"), xLocalContext);
        if (!xResolverIface.is())
        {
            std::cerr << "connectToLibreOffice: Failed to create UnoUrlResolver instance" << std::endl;
            return nullptr;
        }

        uno::Reference<bridge::XUnoUrlResolver> xUrlResolver(xResolverIface, uno::UNO_QUERY);
        if (!xUrlResolver.is())
        {
            std::cerr << "connectToLibreOffice: Failed to query XUnoUrlResolver interface" << std::endl;
            return nullptr;
        }

        uno::Reference<uno::XInterface> xCtxIface;
        try
        {
            xCtxIface = xUrlResolver->resolve(
                rtl::OUString::createFromAscii("uno:socket,host=127.0.0.1,port=2002;urp;StarOffice.ComponentContext"));
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "[UNO] XUnoUrlResolver(socket) failed: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            std::cerr << "Please ensure LibreOffice is running with: soffice --headless --accept=\"socket,host=127.0.0.1,port=2002;urp;\"" << std::endl;
            return nullptr;
        }
        catch (...)
        {
            std::cerr << "connectToLibreOffice: Unknown exception during resolve" << std::endl;
            return nullptr;
        }

        uno::Reference<uno::XComponentContext> xContext(xCtxIface, uno::UNO_QUERY);
        if (!xContext.is())
        {
            std::cerr << "[UNO] XComponentContext (socket) is null! Please ensure LibreOffice is running with correct parameters." << std::endl;
            return nullptr;
        }

        return xContext;
    }

    // 获取ComponentLoader用于加载文档
    static uno::Reference<frame::XComponentLoader> getComponentLoader()
    {
        // 连接到LibreOffice服务
        uno::Reference<uno::XComponentContext> xContext = connectToLibreOffice();
        if (!xContext.is())
        {
            return nullptr;
        }

        uno::Reference<lang::XMultiComponentFactory> xMCF_base = xContext->getServiceManager();
        if (!xMCF_base.is())
        {
            std::cerr << "[UNO] getServiceManager() returned null!" << std::endl;
            return nullptr;
        }

        uno::Reference<lang::XMultiServiceFactory> xMCF(xMCF_base, uno::UNO_QUERY);
        uno::Reference<frame::XComponentLoader> xLoader(xMCF->createInstance(
                                                            rtl::OUString::createFromAscii("com.sun.star.frame.Desktop")),
                                                        uno::UNO_QUERY);
        if (!xLoader.is())
        {
            std::cerr << "getComponentLoader: XComponentLoader is null!" << std::endl;
            return nullptr;
        }

        return xLoader;
    }

    // 将相对路径转换为绝对路径
    static std::string convertToAbsolutePath(const std::string &path)
    {
        std::string absolutePath = path;
        std::cerr << "convertToAbsolutePath: input path: " << path << std::endl;

        // 首先检查是否是绝对路径，如果已经是绝对路径不再进行处理
        // 在Unix/Linux系统中，绝对路径以'/'开头
        if (!path.empty() && path[0] == '/')
        {
            std::cerr << "convertToAbsolutePath: path is already absolute, no conversion needed" << std::endl;
            return absolutePath;
        }

        if (path.substr(0, 3) == "../")
        {
            // 尝试获取当前工作目录
            char cwdBuffer[1024];
            if (getcwd(cwdBuffer, sizeof(cwdBuffer)))
            {
                std::string cwd(cwdBuffer);
                std::cerr << "convertToAbsolutePath: current working directory: " << cwd << std::endl;

                // 移除当前目录末尾的斜杠（如果存在）
                if (!cwd.empty() && cwd.back() == '/')
                {
                    cwd.pop_back();
                }

                // 查找最后一个斜杠以获取父目录
                size_t lastSlash = cwd.find_last_of('/');
                if (lastSlash != std::string::npos)
                {
                    std::string parentDir = cwd.substr(0, lastSlash);
                    absolutePath = parentDir + path.substr(2); // 去掉前面的".."
                    std::cerr << "convertToAbsolutePath: converted to absolute path: " << absolutePath << std::endl;
                }
                else
                {
                    std::cerr << "convertToAbsolutePath: could not find parent directory" << std::endl;
                }
            }
            else
            {
                std::cerr << "convertToAbsolutePath: failed to get current working directory" << std::endl;
            }
        }
        else
        {
            std::cerr << "convertToAbsolutePath: path is not relative to parent, no conversion needed" << std::endl;
        }

        return absolutePath;
    }

    // 将C字符串转换为OUString，自动处理UTF-8编码
    static rtl::OUString convertStringToOUString(const char *str)
    {
        if (!str)
            return rtl::OUString();

        // 检查是否包含非ASCII字符
        bool hasNonAscii = false;
        const char *s = str;
        while (*s)
        {
            if ((unsigned char)*s > 127)
            {
                hasNonAscii = true;
                break;
            }
            s++;
        }

        // 如果包含非ASCII字符，使用UTF-8方式创建OUString
        if (hasNonAscii)
        {
            return rtl::OStringToOUString(str, RTL_TEXTENCODING_UTF8);
        }
        else
        {
            return rtl::OUString::createFromAscii(str);
        }
    }

    // 写入单元格内容
    static void writeCellContent(const uno::Reference<sheet::XSpreadsheet> &sheet, int col, int row, cJSON *cellItem)
    {
        if (cJSON_IsNumber(cellItem))
        {
            printf("Setting cell(%d,%d) to number: %.2f\n", col, row, cellItem->valuedouble);
            sheet->getCellByPosition(col, row)->setValue(cellItem->valuedouble);
        }
        else if (cJSON_IsString(cellItem))
        {
            printf("Setting cell(%d,%d) to string: %s\n", col, row, cellItem->valuestring);
            rtl::OUString value = convertStringToOUString(cellItem->valuestring);
            sheet->getCellByPosition(col, row)->setFormula(value);
        }
        else if (cJSON_IsObject(cellItem))
        {
            // 处理从readSpreadsheetFile读取的对象格式 { "value": ..., "formula": ... }
            cJSON *value = cJSON_GetObjectItem(cellItem, "value");
            cJSON *formula = cJSON_GetObjectItem(cellItem, "formula");

            if (value && cJSON_IsNumber(value))
            {
                printf("Setting cell(%d,%d) to value: %.2f\n", col, row, value->valuedouble);
                sheet->getCellByPosition(col, row)->setValue(value->valuedouble);
            }

            if (formula && cJSON_IsString(formula) && strlen(formula->valuestring) > 0)
            {
                printf("Setting cell(%d,%d) formula: %s\n", col, row, formula->valuestring);
                rtl::OUString formulaStr = convertStringToOUString(formula->valuestring);
                sheet->getCellByPosition(col, row)->setFormula(formulaStr);
            }
        }
    }

    cJSON *readSpreadsheetFile(const rtl::OUString &filePath)
    {
        try
        {
            std::string filePathStr = std::string(rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr());
            std::cerr << "readSpreadsheetFile: Attempting to read file: " << filePathStr << std::endl;

            // 处理相对路径，转换为绝对路径
            std::string absolutePath = convertToAbsolutePath(filePathStr);
            std::cerr << "readSpreadsheetFile: Absolute path: " << absolutePath << std::endl;

            // 获取ComponentLoader
            uno::Reference<frame::XComponentLoader> xLoader = getComponentLoader();
            if (!xLoader.is())
            {
                return nullptr;
            }

            rtl::OUString url = rtl::OUString::createFromAscii("file:///") + rtl::OUString::createFromAscii(absolutePath.c_str());
            std::string urlStr = std::string(rtl::OUStringToOString(url, RTL_TEXTENCODING_UTF8).getStr());
            std::cerr << "readSpreadsheetFile: Loading from URL: " << urlStr << std::endl;

            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<lang::XComponent> xComp(xLoader->loadComponentFromURL(url, rtl::OUString::createFromAscii("_blank"), 0, args), uno::UNO_QUERY);
            if (!xComp.is())
            {
                std::cerr << "readSpreadsheetFile: Failed to load component from URL" << std::endl;
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is())
            {
                std::cerr << "readSpreadsheetFile: Loaded component is not a spreadsheet document" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheets> sheets = xDoc->getSheets();
            if (!sheets.is())
            {
                std::cerr << "readSpreadsheetFile: Failed to get sheets" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);
            if (!nameAccess.is())
            {
                std::cerr << "readSpreadsheetFile: Failed to get name access" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            cJSON *root = cJSON_CreateObject();
            uno::Sequence<rtl::OUString> names = nameAccess->getElementNames();
            std::cerr << "readSpreadsheetFile: Found " << names.getLength() << " sheets" << std::endl;

            for (sal_Int32 i = 0; i < names.getLength(); ++i)
            {
                std::string sheetName = std::string(rtl::OUStringToOString(names[i], RTL_TEXTENCODING_UTF8).getStr());
                std::cerr << "readSpreadsheetFile: Processing sheet: " << sheetName << std::endl;

                uno::Any any = nameAccess->getByName(names[i]);
                uno::Reference<sheet::XSpreadsheet> sheet;
                any >>= sheet;

                if (!sheet.is())
                {
                    std::cerr << "readSpreadsheetFile: Failed to get sheet: " << sheetName << std::endl;
                    continue;
                }

                cJSON *sheetJson = cJSON_CreateArray();
                for (sal_Int32 row = 0; row < 100; ++row)
                { // 只导出前100行
                    cJSON *rowJson = cJSON_CreateArray();
                    for (sal_Int32 col = 0; col < 20; ++col)
                    { // 只导出前20列
                        uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                        if (!cell.is())
                        {
                            std::cerr << "readSpreadsheetFile: Failed to get cell at " << col << "," << row << " in sheet " << sheetName << std::endl;
                            // 创建一个空的单元格对象
                            cJSON *cellJson = cJSON_CreateObject();
                            cJSON_AddNumberToObject(cellJson, "value", 0.0);
                            cJSON_AddStringToObject(cellJson, "formula", "");
                            cJSON_AddItemToArray(rowJson, cellJson);
                            continue;
                        }

                        cJSON *cellJson = cJSON_CreateObject();
                        cJSON_AddNumberToObject(cellJson, "value", cell->getValue());

                        rtl::OUString formula = cell->getFormula();
                        std::string formulaStr = std::string(rtl::OUStringToOString(formula, RTL_TEXTENCODING_UTF8).getStr());
                        cJSON_AddStringToObject(cellJson, "formula", formulaStr.c_str());
                        cJSON_AddItemToArray(rowJson, cellJson);
                    }
                    cJSON_AddItemToArray(sheetJson, rowJson);
                }

                cJSON_AddItemToObject(root, sheetName.c_str(), sheetJson);
                std::cerr << "readSpreadsheetFile: Finished processing sheet: " << sheetName << std::endl;
            }

            closeDocument(xComp);
            std::cerr << "readSpreadsheetFile: Successfully read file" << std::endl;
            return root;
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "readSpreadsheetFile UNO exception: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
            return nullptr;
        }
        catch (const std::exception &e)
        {
            std::cerr << "readSpreadsheetFile std exception: " << e.what() << std::endl;
            return nullptr;
        }
        catch (...)
        {
            std::cerr << "readSpreadsheetFile unknown exception occurred" << std::endl;
            return nullptr;
        }
    }

    cJSON *createNewSpreadsheetFile(const rtl::OUString &filePath, const rtl::OUString &sheetName, const cJSON *contentData)
    {
        try
        {
            // 获取ComponentLoader
            uno::Reference<frame::XComponentLoader> xLoader = getComponentLoader();
            if (!xLoader.is())
            {
                return nullptr;
            }

            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<uno::XInterface> loadedIface = xLoader->loadComponentFromURL(
                rtl::OUString::createFromAscii("private:factory/scalc"), rtl::OUString::createFromAscii("_blank"), 0, args);
            if (!loadedIface.is())
            {
                std::cerr << "createNewSpreadsheetFile: loadComponentFromURL returned null!" << std::endl;
                return nullptr;
            }

            uno::Reference<lang::XComponent> xComp(loadedIface, uno::UNO_QUERY);
            if (!xComp.is())
            {
                std::cerr << "createNewSpreadsheetFile: loaded interface is not XComponent!" << std::endl;
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is())
            {
                std::cerr << "createNewSpreadsheetFile: loaded component is not a spreadsheet document!" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheets> sheets = xDoc->getSheets();
            uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);
            uno::Reference<sheet::XSpreadsheet> sheet;
            if (nameAccess->hasByName(sheetName))
            {
                // 已存在则直接获取
                uno::Any any = nameAccess->getByName(sheetName);
                any >>= sheet;
            }
            else
            {
                // 不存在则插入
                sheets->insertNewByName(sheetName, 0);
                uno::Any any = nameAccess->getByName(sheetName);
                any >>= sheet;
            }

            // 写入内容
            if (sheet.is() && contentData && cJSON_IsObject(contentData))
            {
                printf("Writing content to sheet...\n");

                // 获取contentData中的所有键值对
                int contentSize = cJSON_GetArraySize(const_cast<cJSON *>(contentData));
                printf("Content has %d items\n", contentSize);

                // 遍历对象中的每个键值对
                cJSON *item = contentData->child;
                while (item)
                {
                    // 键是字符，值是位置（如"A1"）
                    const char *character = item->string;     // 字符作为键
                    const char *position = item->valuestring; // 位置作为值

                    if (character && position)
                    {
                        printf("Writing character '%s' to position %s\n", character, position);

                        // 解析位置字符串，如"A1" -> 列=0 (A), 行=0 (1-based, 实际是0)
                        sal_Int32 col = 0, row = 0;
                        parseCellAddress(rtl::OUString::createFromAscii(position), col, row);

                        // 将字符写入指定单元格
                        rtl::OUString charOUString = convertStringToOUString(character);
                        sheet->getCellByPosition(col, row)->setFormula(charOUString);
                    }

                    item = item->next;
                }
                printf("Finished writing content to sheet\n");
            }
            else if (sheet.is() && contentData && cJSON_IsArray(contentData))
            {
                // 保持原有的数组处理逻辑，以确保向后兼容
                printf("Writing content to sheet (array format)...\n");
                // 注意：cJSON_GetArraySize需要非const指针，所以我们需要一个非const副本
                cJSON *nonConstContent = const_cast<cJSON *>(contentData);
                int contentSize = cJSON_GetArraySize(nonConstContent);
                printf("Content has %d rows\n", contentSize);

                int row = 0;
                cJSON *rowItem = nullptr;
                cJSON_ArrayForEach(rowItem, nonConstContent)
                {
                    if (row >= 100)
                        break; // 限制行数

                    int col = 0;
                    cJSON *cellItem = nullptr;
                    cJSON_ArrayForEach(cellItem, rowItem)
                    {
                        if (col >= 20)
                            break; // 限制列数

                        writeCellContent(sheet, col, row, cellItem);
                        ++col;
                    }
                    ++row;
                }
                printf("Finished writing content to sheet\n");
            }
            else if (!sheet.is())
            {
                printf("Sheet is not valid\n");
            }
            else if (!contentData)
            {
                printf("No content data to write\n");
            }
            else
            {
                printf("Content data format is not supported\n");
                // 注意：cJSON_Print需要非const指针，所以我们需要一个非const副本
                cJSON *nonConstContent = const_cast<cJSON *>(contentData);
                char *contentStr = cJSON_Print(nonConstContent);
                printf("Content data: %s\n", contentStr);
                free(contentStr);
            }

            // 保存文档前输出文件路径信息
            std::string filePathStr = rtl::OUStringToOString(filePath, RTL_TEXTENCODING_UTF8).getStr();
            std::cerr << "createNewSpreadsheetFile: Saving document to: " << filePathStr << std::endl;
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
            // 获取ComponentLoader
            uno::Reference<frame::XComponentLoader> xLoader = getComponentLoader();
            if (!xLoader.is())
            {
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
                // 将newValue转为符合单元格公式的写法格式;
                // 从配置中获取默认工作表名
                const char *defaultSheetName = json_config_get_string("WordsSheet");
                if (!defaultSheetName)
                {
                    defaultSheetName = "WordsSheet"; // 默认值
                }
                rtl::OUString defaultSheetNameOUString = rtl::OUString::createFromAscii(defaultSheetName);
                logger_log("defaultSheetName: %s", rtl::OUStringToOString(defaultSheetNameOUString, RTL_TEXTENCODING_UTF8).getStr());
                rtl::OUString tmp = findCharPositions(newValue, readSheetData(filePath, defaultSheetNameOUString));
                logger_log("updateSpreadsheetContent: %s:", rtl::OUStringToOString(tmp, RTL_TEXTENCODING_UTF8).getStr());
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
        printf("Creating new spreadsheet file\n");

        // 从配置文件中获取数据路径和默认文件名
        const char *datapath = json_config_get_string("datapath");
        const char *defaultname = json_config_get_string("defaultname");
        const char *wordsSheetNameConfig = json_config_get_string("WordsSheet");

        if (!datapath)
        {
            datapath = "../data"; // 默认数据路径
        }

        if (!defaultname)
        {
            defaultname = "default.xlsx"; // 默认文件名
        }

        // 默认工作表名
        const char *defaultSheetName = wordsSheetNameConfig ? wordsSheetNameConfig : "WordsSheet";
        printf("datapath: %s\n", datapath);
        printf("defaultname: %s\n", defaultname);
        printf("defaultSheetName: %s\n", defaultSheetName);

        // 构建默认文件的完整路径
        std::string defaultFilePathStr = std::string(datapath) + "/" + std::string(defaultname);
        printf("defaultFilePathStr (before processing): %s\n", defaultFilePathStr.c_str());

        // 处理相对路径，转换为绝对路径
        std::string absoluteDefaultFilePathStr = convertToAbsolutePath(defaultFilePathStr);
        printf("absoluteDefaultFilePathStr (after processing): %s\n", absoluteDefaultFilePathStr.c_str());

        rtl::OString absoluteDefaultFilePathOStr = rtl::OString(absoluteDefaultFilePathStr.c_str());
        rtl::OUString defaultFilePath = rtl::OStringToOUString(absoluteDefaultFilePathOStr, RTL_TEXTENCODING_UTF8);
        printf("defaultFilePath OUString: %s\n", rtl::OUStringToOString(defaultFilePath, RTL_TEXTENCODING_UTF8).getStr());

        // 生成唯一的文件名，格式为 spreadsheet_时间戳.xlsx
        time_t now = time(0);
        struct tm *timeinfo = localtime(&now);
        char filename[100];
        strftime(filename, sizeof(filename), "spreadsheet_%Y%m%d_%H%M%S.xlsx", timeinfo);

        // 构建新文件的完整路径
        std::string newFilePathStr = std::string(datapath) + "/" + std::string(filename);
        printf("newFilePath (before processing): %s\n", newFilePathStr.c_str());

        // 处理相对路径，转换为绝对路径
        std::string absoluteNewFilePathStr = convertToAbsolutePath(newFilePathStr);
        rtl::OString absoluteNewFilePathOStr = rtl::OString(absoluteNewFilePathStr.c_str());
        rtl::OUString newFilePath = rtl::OStringToOUString(absoluteNewFilePathOStr, RTL_TEXTENCODING_UTF8);
        printf("newFilePath (after processing): %s\n", absoluteNewFilePathStr.c_str());

        // 默认sheet名
        rtl::OUString sheetName = rtl::OUString::createFromAscii(defaultSheetName);

        // 尝试从默认文件中读取WordsSheet的内容
        cJSON *contentData = nullptr;
        printf("Attempting to read %s from default spreadsheet file...\n", defaultSheetName);
        rtl::OUString wordsSheetName = rtl::OUString::createFromAscii(defaultSheetName);
        contentData = readSheetData(defaultFilePath, wordsSheetName);
        if (contentData)
        {
            logger_log("contentData: %s\n", cJSON_Print(contentData));
            printf("Successfully read WordsSheet directly\n");
            // 打印一些关于内容的信息
            // 注意：cJSON_GetArraySize需要非const指针，所以我们需要一个非const副本
            cJSON *nonConstSheet = const_cast<cJSON *>(contentData);
            int sheetSize = cJSON_GetArraySize(nonConstSheet);
            printf("WordsSheet content size: %d rows\n", sheetSize);

            if (sheetSize > 0)
            {
                cJSON *firstRow = cJSON_GetArrayItem(nonConstSheet, 0);
                if (firstRow)
                {
                    int rowSize = cJSON_GetArraySize(firstRow);
                    printf("First row has %d columns\n", rowSize);

                    // 打印前几个单元格的值
                    for (int i = 0; i < rowSize && i < 5; i++)
                    {
                        cJSON *cell = cJSON_GetArrayItem(firstRow, i);
                        if (cell)
                        {
                            cJSON *value = cJSON_GetObjectItem(cell, "value");
                            cJSON *formula = cJSON_GetObjectItem(cell, "formula");
                            if (value && formula)
                            {
                                printf("Cell[%d]: value=%.2f, formula=%s\n",
                                       i, value->valuedouble,
                                       formula->valuestring ? formula->valuestring : "NULL");
                            }
                        }
                    }
                }
            }
        }
        else
        {
            printf("Failed to read WordsSheet directly\n");
        }

        // 创建新的电子表格文件，包含从默认文件复制的内容
        printf("Creating new spreadsheet file with content...\n");
        if (contentData)
        {
            // 注意：cJSON_GetArraySize需要非const指针，所以我们需要一个非const副本
            cJSON *nonConstContent = const_cast<cJSON *>(contentData);
            int contentSize = cJSON_GetArraySize(nonConstContent);
            printf("Content data size: %d rows\n", contentSize);
        }
        else
        {
            printf("No content data to copy\n");
        }

        cJSON *createResult = filemanager::createNewSpreadsheetFile(newFilePath, sheetName, contentData);
        if (contentData)
        {
            cJSON_Delete(contentData);
        }

        if (createResult)
        {
            cJSON_AddItemToObject(results, "result", createResult);
            // 添加生成的文件名到返回结果中
            cJSON_AddStringToObject(results, "filename", filename);
            printf("Successfully created new spreadsheet file: %s\n", filename);
        }
        else
        {
            cJSON_AddStringToObject(results, "error", "Failed to create spreadsheet file");
            printf("Failed to create new spreadsheet file\n");
        }
    }

    void updatefile(cJSON *results, const char *body)
    {
        // body 应为 JSON 字符串，支持单个或批量更新
        if (!body)
        {
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

                rtl::OUString filePath = convertStringToOUString(filenameItem->valuestring);
                rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
                rtl::OUString cellAddr = convertStringToOUString(cellItem->valuestring);
                rtl::OUString value = convertStringToOUString(valueItem->valuestring);
                rtl::OUString type = convertStringToOUString(typeItem->valuestring);

                // 检测value的语言
                std::string language = detectLanguage(value);
                printf("Detected language for value '%s': %s\n",
                       rtl::OUStringToOString(value, RTL_TEXTENCODING_UTF8).getStr(),
                       language.c_str());

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

            rtl::OUString filePath = convertStringToOUString(filenameItem->valuestring);
            rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
            rtl::OUString cellAddr = convertStringToOUString(cellItem->valuestring);
            rtl::OUString value = convertStringToOUString(valueItem->valuestring);
            rtl::OUString type = convertStringToOUString(typeItem->valuestring);

            // 检测value的语言
            std::string language = detectLanguage(value);
            printf("Detected language for value '%s': %s\n",
                   rtl::OUStringToOString(value, RTL_TEXTENCODING_UTF8).getStr(),
                   language.c_str());

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
            // 获取ComponentLoader
            uno::Reference<frame::XComponentLoader> xLoader = getComponentLoader();
            if (!xLoader.is())
            {
                return nullptr;
            }

            rtl::OUString url = rtl::OUString::createFromAscii("file:///") + filePath.replaceAll("\\", "/");
            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<lang::XComponent> xComp(xLoader->loadComponentFromURL(url, rtl::OUString::createFromAscii("_blank"), 0, args), uno::UNO_QUERY);
            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is())
            {
                std::cerr << "findValueInSheet: Cannot open spreadsheet!" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheet> sheet = getSheet(xDoc, sheetName);
            if (!sheet.is())
            {
                std::cerr << "findValueInSheet: Cannot get sheet!" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            // 创建返回的 cJSON 数组，用于存储所有匹配的位置
            cJSON *positions = cJSON_CreateArray();

            // 在固定范围内搜索（前100行，前20列）
            for (sal_Int32 row = 0; row < 100; ++row)
            {
                for (sal_Int32 col = 0; col < 20; ++col)
                {
                    uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
                    if (!cell.is())
                    {
                        continue;
                    }

                    // 获取单元格的值和公式
                    double cellValue = cell->getValue();
                    rtl::OUString cellFormula = cell->getFormula();

                    // 检查单元格值是否匹配搜索值
                    bool found = false;
                    if (searchValue.getLength() == 0 && (cellValue == 0.0 && cellFormula.getLength() == 0))
                    {
                        // 查找空单元格
                        found = true;
                    }
                    else if (cellFormula.getLength() > 0)
                    {
                        // 如果有公式，比较公式文本
                        if (cellFormula.equals(searchValue))
                        {
                            found = true;
                        }
                    }
                    else
                    {
                        // 比较数值或文本值
                        rtl::OUString cellText = rtl::OUString::number(cellValue);
                        if (cellText.equals(searchValue))
                        {
                            found = true;
                        }
                    }

                    if (found)
                    {
                        // 计算列名 (A, B, ..., Z, AA, AB, ...)
                        std::string columnName;
                        int column = col;
                        do
                        {
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
        if (!body)
        {
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
        rtl::OUString filePath = convertStringToOUString(filenameItem->valuestring);
        rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);
        rtl::OUString searchValue = convertStringToOUString(valueItem->valuestring);

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
            // 获取ComponentLoader
            uno::Reference<frame::XComponentLoader> xLoader = getComponentLoader();
            if (!xLoader.is())
            {
                return nullptr;
            }

            rtl::OUString url = rtl::OUString::createFromAscii("file:///") + filePath.replaceAll("\\", "/");
            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<lang::XComponent> xComp(xLoader->loadComponentFromURL(url, rtl::OUString::createFromAscii("_blank"), 0, args), uno::UNO_QUERY);
            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            if (!xDoc.is())
            {
                std::cerr << "readSheetData: Cannot open spreadsheet!" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            uno::Reference<sheet::XSpreadsheet> sheet = getSheet(xDoc, sheetName);
            if (!sheet.is())
            {
                std::cerr << "readSheetData: Cannot get sheet!" << std::endl;
                closeDocument(xComp);
                return nullptr;
            }

            // 创建返回的 cJSON 对象，用于存储单元格内容和位置的映射
            cJSON *contentMap = cJSON_CreateObject();

            // 创建一个用于跟踪已添加内容的辅助对象，确保内容不重复
            cJSON *addedContents = cJSON_CreateObject();

            // 按列优先读取数据，每列读取到第一个空单元格为止
            sal_Int32 col = 0;
            const sal_Int32 maxCols = 1000; // 设置一个最大列数限制，防止无限循环
            const sal_Int32 maxRows = 30000; // 设置一个最大行数限制，防止无限循环
            
            // 循环处理每一列
            while (col < maxCols) {
                bool foundDataInColumn = false;
                
                // 处理当前列的每一行
                for (sal_Int32 row = 0; row < maxRows; ++row) {
                    uno::Reference<table::XCell> cell;
                    
                    try {
                        // 尝试获取单元格
                        cell = sheet->getCellByPosition(col, row);
                    } catch (const uno::Exception &) {
                        // 如果获取单元格失败，认为此列结束
                        break;
                    }
                    
                    if (!cell.is()) {
                        // 单元格无效，此列结束
                        break;
                    }

                    // 获取单元格的值和公式
                    double cellValue = cell->getValue();
                    rtl::OUString cellFormula = cell->getFormula();

                    // 检查单元格是否为空
                    if (cellValue == 0.0 && cellFormula.getLength() == 0) {
                        // 遇到空单元格，此列结束
                        break;
                    }

                    // 标记此列有数据
                    foundDataInColumn = true;

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
                
                // 如果此列没有数据，则认为是空列，停止处理后续列
                if (!foundDataInColumn) {
                    break;
                }
                
                ++col;
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
        if (!body)
        {
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
        rtl::OUString filePath = convertStringToOUString(filenameItem->valuestring);
        rtl::OUString sheetName = convertStringToOUString(sheetnameItem->valuestring);

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
        for (sal_Int32 i = 0; i < newValue.getLength(); ++i)
        {
            // 获取当前字符
            rtl::OUString charStr = newValue.copy(i, 1);
            logger_log("charStr: %s", rtl::OUStringToOString(charStr, RTL_TEXTENCODING_UTF8).getStr());
            // 将字符转换为UTF8字符串用于在sheetData中查找
            std::string charStdStr = rtl::OUStringToOString(charStr, RTL_TEXTENCODING_UTF8).getStr();
            logger_log("charStdStr: %s", charStdStr.c_str());
            // 在sheetData中查找该字符
            cJSON *positionItem = cJSON_GetObjectItem(sheetData, charStdStr.c_str());

            if (positionItem && positionItem->valuestring)
            {
                logger_log("positionItem: %s", positionItem->valuestring);
                // 如果找到位置信息，添加到结果中
                if (i > 0)
                {
                    // 除了第一个字符外，其他字符前面都加&
                    resultBuffer.append(rtl::OUString::createFromAscii("&"));
                }
                else
                {
                    resultBuffer.append(rtl::OUString::createFromAscii("="));
                }
                resultBuffer.append(rtl::OUString::createFromAscii("$"));
                // 从配置中获取默认工作表名
                const char *defaultSheetName = json_config_get_string("WordsSheet");
                if (!defaultSheetName)
                {
                    defaultSheetName = "WordsSheet"; // 默认值
                }
                resultBuffer.append(rtl::OUString::createFromAscii(defaultSheetName));
                resultBuffer.append(rtl::OUString::createFromAscii("."));
                resultBuffer.append(rtl::OUString::createFromAscii(positionItem->valuestring));
            }
            else
            {
                // 如果未找到字符位置，可以添加一个占位符或跳过
                if (i > 0)
                {
                    resultBuffer.append(rtl::OUString::createFromAscii(""));
                }
                resultBuffer.append(rtl::OUString::createFromAscii("N/A"));
            }
        }
        // 注意此处的打印和return 语句 makeStringAndClear 打印中调用后就清理了，return 会返回空串
        // logger_log("resultBuffer: %s", rtl::OUStringToOString(resultBuffer.makeStringAndClear(), RTL_TEXTENCODING_UTF8).getStr());
        //  返回组合的位置字符串
        return resultBuffer.makeStringAndClear();
    }
} // namespace filemanager