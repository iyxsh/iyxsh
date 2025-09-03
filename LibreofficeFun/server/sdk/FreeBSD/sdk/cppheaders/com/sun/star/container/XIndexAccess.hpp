#ifndef INCLUDED_COM_SUN_STAR_CONTAINER_XINDEXACCESS_HPP
#define INCLUDED_COM_SUN_STAR_CONTAINER_XINDEXACCESS_HPP

#include "sal/config.h"

#include "com/sun/star/container/XIndexAccess.hdl"

#include "com/sun/star/container/XElementAccess.hpp"
#include "com/sun/star/uno/Any.hxx"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace container {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::container::XIndexAccess const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::container::XElementAccess > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.container.XIndexAccess", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::container::XIndexAccess > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::container::XIndexAccess > >::get();
}

::css::uno::Type const & ::css::container::XIndexAccess::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::container::XIndexAccess >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::container::XIndexAccess>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_CONTAINER_XINDEXACCESS_HPP
