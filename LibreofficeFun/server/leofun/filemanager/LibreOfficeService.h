#ifndef LIBREOFFICESERVICE_H
#define LIBREOFFICESERVICE_H

#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <rtl/ustring.hxx>

namespace filemanager {

class LibreOfficeService {
public:
    // 初始化服务
    static bool initialize();
    
    // 获取组件加载器
    static com::sun::star::uno::Reference<com::sun::star::frame::XComponentLoader> getComponentLoader();
    
    // 加载电子表格文档
    static com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument> 
    loadSpreadsheetDocument(const rtl::OUString& filePath);
    
    // 获取工作表
    static com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet> 
    getSheet(const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument>& doc,
             const rtl::OUString& sheetName);
    
    // 保存文档
    static bool saveDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface>& doc,
                            const rtl::OUString& filePath = rtl::OUString());
    
    // 关闭文档
    static void closeDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface>& doc);
    
    // 检查服务状态
    static bool isAvailable();
    
    // 释放资源
    static void shutdown();
};

} // namespace filemanager

#endif // LIBREOFFICESERVICE_H