#ifndef LIBREOFFICESERVICE_H
#define LIBREOFFICESERVICE_H

#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/frame/XDesktop.hpp>
#include <rtl/ustring.hxx>
#include <string>

#include "DocumentManager.h"

namespace filemanager
{

    class LibreOfficeService
    {
    public:
        // 初始化服务连接
        static bool initialize();

        // 建立LibreOffice连接
        static bool establishConnection();

        // 获取组件加载器接口
        static com::sun::star::uno::Reference<com::sun::star::frame::XComponentLoader> getComponentLoader();

        // 获取组件上下文
        static com::sun::star::uno::Reference<com::sun::star::uno::XComponentContext> getContext();

        // 创建新的电子表格文档
        static com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument>
        CreateNewSpreadsheetDocument();

        // 加载电子表格文档
        static com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument>
        loadSpreadsheetDocument(const rtl::OUString &filePath);

        // 加载文本文档
        static com::sun::star::uno::Reference<com::sun::star::text::XTextDocument>
        loadTextDocument(const rtl::OUString &filePath);

        // 保存文档到指定路径(为空则保存到原位置)
        static bool saveDocument(
            const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &doc,
            const rtl::OUString &filePath = rtl::OUString());

        // 另存文档
        static bool saveDocumentAs(
            const com::sun::star::uno::Reference<com::sun::star::frame::XModel> &document,
            const rtl::OUString &filePath);

        // 安全关闭文档(通用版本)
        static bool closeDocument(const com::sun::star::uno::Reference<com::sun::star::uno::XInterface> &document);

        // 检查服务可用性
        static bool isAvailable();

        // 安全关闭服务并释放资源
        static void shutdown();

    private:
        // 禁用构造和拷贝
        LibreOfficeService() = delete;
        LibreOfficeService(const LibreOfficeService &) = delete;
        LibreOfficeService &operator=(const LibreOfficeService &) = delete;

        // 组件上下文
        static com::sun::star::uno::Reference<com::sun::star::uno::XComponentContext> m_xContext;
        // 服务管理器
        static com::sun::star::uno::Reference<com::sun::star::lang::XMultiComponentFactory> m_xServiceManager;
        // 桌面服务
        static com::sun::star::uno::Reference<com::sun::star::frame::XDesktop> m_xDesktop;
        // 组件加载器
        static com::sun::star::uno::Reference<com::sun::star::frame::XComponentLoader> m_xComponentLoader;
        // 初始化标志
        static bool m_initialized;
    };

} // namespace filemanager

#endif // LIBREOFFICESERVICE_H