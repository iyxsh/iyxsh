#ifndef INCLUDED_COM_SUN_STAR_DRAWING_XCONNECTABLESHAPE_HPP
#define INCLUDED_COM_SUN_STAR_DRAWING_XCONNECTABLESHAPE_HPP

#include "sal/config.h"

#include "com/sun/star/drawing/XConnectableShape.hdl"

#include "com/sun/star/awt/Point.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace drawing {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::drawing::XConnectableShape const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.drawing.XConnectableShape", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::drawing::XConnectableShape > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::drawing::XConnectableShape > >::get();
}

::css::uno::Type const & ::css::drawing::XConnectableShape::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::drawing::XConnectableShape >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::drawing::XConnectableShape>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_DRAWING_XCONNECTABLESHAPE_HPP
