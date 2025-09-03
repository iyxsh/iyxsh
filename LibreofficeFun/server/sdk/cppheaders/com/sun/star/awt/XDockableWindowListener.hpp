#ifndef INCLUDED_COM_SUN_STAR_AWT_XDOCKABLEWINDOWLISTENER_HPP
#define INCLUDED_COM_SUN_STAR_AWT_XDOCKABLEWINDOWLISTENER_HPP

#include "sal/config.h"

#include "com/sun/star/awt/XDockableWindowListener.hdl"

#include "com/sun/star/awt/DockingData.hpp"
#include "com/sun/star/awt/DockingEvent.hpp"
#include "com/sun/star/awt/EndDockingEvent.hpp"
#include "com/sun/star/awt/EndPopupModeEvent.hpp"
#include "com/sun/star/lang/EventObject.hpp"
#include "com/sun/star/lang/XEventListener.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace awt {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::awt::XDockableWindowListener const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::lang::XEventListener > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.awt.XDockableWindowListener", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::awt::XDockableWindowListener > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::awt::XDockableWindowListener > >::get();
}

::css::uno::Type const & ::css::awt::XDockableWindowListener::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::awt::XDockableWindowListener >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::awt::XDockableWindowListener>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_AWT_XDOCKABLEWINDOWLISTENER_HPP
