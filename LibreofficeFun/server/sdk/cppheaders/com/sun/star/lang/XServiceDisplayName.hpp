#ifndef INCLUDED_COM_SUN_STAR_LANG_XSERVICEDISPLAYNAME_HPP
#define INCLUDED_COM_SUN_STAR_LANG_XSERVICEDISPLAYNAME_HPP

#include "sal/config.h"

#include "com/sun/star/lang/XServiceDisplayName.hdl"

#include "com/sun/star/lang/Locale.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace lang {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::lang::XServiceDisplayName const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.lang.XServiceDisplayName", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::lang::XServiceDisplayName > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::lang::XServiceDisplayName > >::get();
}

::css::uno::Type const & ::css::lang::XServiceDisplayName::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::lang::XServiceDisplayName >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::lang::XServiceDisplayName>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_LANG_XSERVICEDISPLAYNAME_HPP
