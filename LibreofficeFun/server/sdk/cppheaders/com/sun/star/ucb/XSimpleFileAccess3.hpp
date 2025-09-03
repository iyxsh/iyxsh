#ifndef INCLUDED_COM_SUN_STAR_UCB_XSIMPLEFILEACCESS3_HPP
#define INCLUDED_COM_SUN_STAR_UCB_XSIMPLEFILEACCESS3_HPP

#include "sal/config.h"

#include "com/sun/star/ucb/XSimpleFileAccess3.hdl"

#include "com/sun/star/ucb/XSimpleFileAccess2.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace ucb {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::ucb::XSimpleFileAccess3 const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::ucb::XSimpleFileAccess2 > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.ucb.XSimpleFileAccess3", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::ucb::XSimpleFileAccess3 > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::ucb::XSimpleFileAccess3 > >::get();
}

::css::uno::Type const & ::css::ucb::XSimpleFileAccess3::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::ucb::XSimpleFileAccess3 >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::ucb::XSimpleFileAccess3>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_UCB_XSIMPLEFILEACCESS3_HPP
