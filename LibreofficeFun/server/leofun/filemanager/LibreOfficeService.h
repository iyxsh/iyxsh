#ifndef LIBREOFFICESERVICE_H
#define LIBREOFFICESERVICE_H

#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <rtl/ustring.hxx>

namespace filemanager {

class LibreOfficeService {
public:
    // 初始化服务连接
    static bool initialize();
    
    // 获取组件加载器接口
    static com::sun::star::uno::Reference<com::sun::star::frame::XComponentLoader> getComponentLoader();
    
    // 获取组件上下文
    static com::sun::star::uno::Reference<com::sun::star::uno::XComponentContext> getContext();
    
    static com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument>
    CreateNewSpreadsheetDocument();

    // 加载电子表格文档
    static com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument> 
    loadSpreadsheetDocument(const rtl::OUString& filePath);
    
    // 保存文档到指定路径(为空则保存到原位置)
    static bool saveDocument(
        const com::sun::star::uno::Reference<com::sun::star::uno::XInterface>& doc,
        const rtl::OUString& filePath = rtl::OUString());
    
    // 安全关闭文档(含异常处理)
    static void closeDocument(const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument>& doc);
    
    // 检查服务可用性
    static bool isAvailable();
    
    // 安全关闭服务并释放资源
    static void shutdown();
    
private:
    // 禁用构造和拷贝
    LibreOfficeService() = delete;
    LibreOfficeService(const LibreOfficeService&) = delete;
    LibreOfficeService& operator=(const LibreOfficeService&) = delete;
};

} // namespace filemanager

#endif // LIBREOFFICESERVICE_H