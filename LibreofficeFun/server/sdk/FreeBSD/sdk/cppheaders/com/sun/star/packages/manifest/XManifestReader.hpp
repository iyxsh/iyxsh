#ifndef INCLUDED_COM_SUN_STAR_PACKAGES_MANIFEST_XMANIFESTREADER_HPP
#define INCLUDED_COM_SUN_STAR_PACKAGES_MANIFEST_XMANIFESTREADER_HPP

#include "sal/config.h"

#include "com/sun/star/packages/manifest/XManifestReader.hdl"

#include "com/sun/star/beans/PropertyValue.hpp"
#include "com/sun/star/io/XInputStream.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace packages { namespace manifest {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::packages::manifest::XManifestReader const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.packages.manifest.XManifestReader", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::packages::manifest::XManifestReader > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::packages::manifest::XManifestReader > >::get();
}

::css::uno::Type const & ::css::packages::manifest::XManifestReader::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::packages::manifest::XManifestReader >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::packages::manifest::XManifestReader>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_PACKAGES_MANIFEST_XMANIFESTREADER_HPP
