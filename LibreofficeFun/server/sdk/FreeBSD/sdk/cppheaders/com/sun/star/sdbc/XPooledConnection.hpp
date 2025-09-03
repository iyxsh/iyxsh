#ifndef INCLUDED_COM_SUN_STAR_SDBC_XPOOLEDCONNECTION_HPP
#define INCLUDED_COM_SUN_STAR_SDBC_XPOOLEDCONNECTION_HPP

#include "sal/config.h"

#include "com/sun/star/sdbc/XPooledConnection.hdl"

#include "com/sun/star/sdbc/XConnection.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace sdbc {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::sdbc::XPooledConnection const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.sdbc.XPooledConnection", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::sdbc::XPooledConnection > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::sdbc::XPooledConnection > >::get();
}

::css::uno::Type const & ::css::sdbc::XPooledConnection::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::sdbc::XPooledConnection >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::sdbc::XPooledConnection>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_SDBC_XPOOLEDCONNECTION_HPP
