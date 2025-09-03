#ifndef INCLUDED_COM_SUN_STAR_AWT_XMOUSELISTENER_HPP
#define INCLUDED_COM_SUN_STAR_AWT_XMOUSELISTENER_HPP

#include "sal/config.h"

#include "com/sun/star/awt/XMouseListener.hdl"

#include "com/sun/star/awt/MouseEvent.hpp"
#include "com/sun/star/lang/XEventListener.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace awt {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::awt::XMouseListener const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::lang::XEventListener > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.awt.XMouseListener", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::awt::XMouseListener > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::awt::XMouseListener > >::get();
}

::css::uno::Type const & ::css::awt::XMouseListener::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::awt::XMouseListener >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::awt::XMouseListener>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_AWT_XMOUSELISTENER_HPP
