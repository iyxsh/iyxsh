#ifndef INCLUDED_COM_SUN_STAR_PACKAGES_ZIP_XZIPFILEACCESS2_HPP
#define INCLUDED_COM_SUN_STAR_PACKAGES_ZIP_XZIPFILEACCESS2_HPP

#include "sal/config.h"

#include "com/sun/star/packages/zip/XZipFileAccess2.hdl"

#include "com/sun/star/container/XNameAccess.hpp"
#include "com/sun/star/packages/zip/XZipFileAccess.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace packages { namespace zip {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::packages::zip::XZipFileAccess2 const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[2];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::packages::zip::XZipFileAccess > >::get().getTypeLibType();
        aSuperTypes[1] = ::cppu::UnoType< const ::css::uno::Reference< ::css::container::XNameAccess > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.packages.zip.XZipFileAccess2", 2, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::packages::zip::XZipFileAccess2 > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::packages::zip::XZipFileAccess2 > >::get();
}

::css::uno::Type const & ::css::packages::zip::XZipFileAccess2::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::packages::zip::XZipFileAccess2 >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::packages::zip::XZipFileAccess2>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_PACKAGES_ZIP_XZIPFILEACCESS2_HPP
