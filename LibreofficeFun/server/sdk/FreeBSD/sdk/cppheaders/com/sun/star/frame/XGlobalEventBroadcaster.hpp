#ifndef INCLUDED_COM_SUN_STAR_FRAME_XGLOBALEVENTBROADCASTER_HPP
#define INCLUDED_COM_SUN_STAR_FRAME_XGLOBALEVENTBROADCASTER_HPP

#include "sal/config.h"

#include "com/sun/star/frame/XGlobalEventBroadcaster.hdl"

#include "com/sun/star/container/XSet.hpp"
#include "com/sun/star/document/XDocumentEventBroadcaster.hpp"
#include "com/sun/star/document/XDocumentEventListener.hpp"
#include "com/sun/star/document/XEventsSupplier.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace frame {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::frame::XGlobalEventBroadcaster const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[4];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::document::XEventsSupplier > >::get().getTypeLibType();
        aSuperTypes[1] = ::cppu::UnoType< const ::css::uno::Reference< ::css::document::XDocumentEventBroadcaster > >::get().getTypeLibType();
        aSuperTypes[2] = ::cppu::UnoType< const ::css::uno::Reference< ::css::container::XSet > >::get().getTypeLibType();
        aSuperTypes[3] = ::cppu::UnoType< const ::css::uno::Reference< ::css::document::XDocumentEventListener > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.frame.XGlobalEventBroadcaster", 4, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::frame::XGlobalEventBroadcaster > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::frame::XGlobalEventBroadcaster > >::get();
}

::css::uno::Type const & ::css::frame::XGlobalEventBroadcaster::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::frame::XGlobalEventBroadcaster >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::frame::XGlobalEventBroadcaster>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_FRAME_XGLOBALEVENTBROADCASTER_HPP
