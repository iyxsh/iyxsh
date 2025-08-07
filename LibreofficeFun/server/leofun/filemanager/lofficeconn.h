#ifndef LOFFICECONN_H
#define LOFFICECONN_H

#include <com/sun/star/uno/Exception.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/bridge/UnoUrlResolver.hpp>
#include <cppuhelper/bootstrap.hxx>
#include <rtl/ustring.hxx>

using namespace com::sun::star;

namespace filemanager
{
    // LibreOffice连接管理器实现
    class LibreOfficeConnectionManager
    {
    public:
        static bool initialize();
        static void release();
        static uno::Reference<frame::XComponentLoader> getComponentLoader();
        static uno::Reference<uno::XComponentContext> getContext();
        static bool isConnected() { return mIsConnected; }

    private:
        static uno::Reference<uno::XComponentContext> mContext;
        static uno::Reference<frame::XComponentLoader> mLoader;
        static bool mIsConnected;
    };
} // namespace filemanager

#endif // LOFFICECONN_H