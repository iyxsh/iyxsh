#include "filemanager.h"
#include "../cJSON/cJSON.h"
// 兼容旧版cJSON宏
#ifndef cJSON_IsString
#define cJSON_IsString(item) ((item) && ((item)->type & cJSON_String))
#endif
#ifndef cJSON_IsArray
#define cJSON_IsArray(item) ((item) && ((item)->type & cJSON_Array))
#endif
#ifndef cJSON_IsObject
#define cJSON_IsObject(item) ((item) && ((item)->type & cJSON_Object))
#endif
#ifndef cJSON_IsNumber
#define cJSON_IsNumber(item) ((item) && ((item)->type & cJSON_Number))
#endif
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
#include <cppuhelper/bootstrap.hxx>
#include <rtl/ustrbuf.hxx>
#include <rtl/ustring.hxx>
#include <stdexcept>
#include <iostream>

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

    void saveDocument(const uno::Reference<uno::XInterface> &docIface, const rtl::OUString &filePath)
    {
        try
        {
            uno::Reference<frame::XStorable> xStorable(docIface, uno::UNO_QUERY);
            if (!xStorable.is())
                throw std::runtime_error("Invalid storable interface");
            uno::Sequence<beans::PropertyValue> props(2);
            props[0].Name = rtl::OUString::createFromAscii("FilterName");
            props[0].Value <<= rtl::OUString::createFromAscii("calc8");
            props[1].Name = rtl::OUString::createFromAscii("Overwrite");
            props[1].Value <<= true;
            rtl::OUString url = rtl::OUString::createFromAscii("file:///") + filePath.replaceAll("\\", "/");
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
            uno::Reference<uno::XComponentContext> xContext = cppu::defaultBootstrap_InitialComponentContext();
            uno::Reference<lang::XMultiComponentFactory> xMCF_base = xContext->getServiceManager();
            uno::Reference<lang::XMultiServiceFactory> xMCF(xMCF_base, uno::UNO_QUERY);
            uno::Reference<frame::XComponentLoader> xLoader(xMCF->createInstance(
                                                                rtl::OUString::createFromAscii("com.sun.star.frame.Desktop")),
                                                            uno::UNO_QUERY);
            uno::Sequence<beans::PropertyValue> args(0);
            uno::Reference<lang::XComponent> xComp(xLoader->loadComponentFromURL(
                                                       rtl::OUString::createFromAscii("private:factory/scalc"), rtl::OUString::createFromAscii("_blank"), 0, args),
                                                   uno::UNO_QUERY);
            uno::Reference<sheet::XSpreadsheetDocument> xDoc(xComp, uno::UNO_QUERY);
            uno::Reference<sheet::XSpreadsheets> sheets = xDoc->getSheets();
            sheets->insertNewByName(sheetName, 0);
            uno::Reference<container::XNameAccess> nameAccess(sheets, uno::UNO_QUERY);
            uno::Any any = nameAccess->getByName(sheetName);
            uno::Reference<sheet::XSpreadsheet> sheet;
            any >>= sheet;
            // 写入内容
            if (contentData && cJSON_IsArray(contentData))
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
                        else if (cJSON_IsString(cellItem))
                            sheet->getCellByPosition(col, row)->setFormula(rtl::OUString::createFromAscii(cellItem->valuestring));
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
            uno::Reference<sheet::XSpreadsheet> sheet = getSheet(xDoc, sheetName);
            sal_Int32 col, row;
            parseCellAddress(cellAddress, col, row);
            uno::Reference<table::XCell> cell = sheet->getCellByPosition(col, row);
            if (cellType.equalsIgnoreAsciiCase("number"))
                cell->setValue(newValue.toDouble());
            else
                cell->setFormula(newValue);
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
        // body 应为 JSON 字符串，包含文件名、sheet名、可选内容
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
        rtl::OUString filePath = rtl::OUString::createFromAscii(filenameItem->valuestring);
        rtl::OUString sheetName = rtl::OUString::createFromAscii(sheetnameItem->valuestring);
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
        //updatefile(results, body);
    }

} // namespace filemanager