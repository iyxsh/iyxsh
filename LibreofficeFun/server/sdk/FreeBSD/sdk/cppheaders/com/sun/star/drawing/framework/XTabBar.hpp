#ifndef INCLUDED_COM_SUN_STAR_DRAWING_FRAMEWORK_XTABBAR_HPP
#define INCLUDED_COM_SUN_STAR_DRAWING_FRAMEWORK_XTABBAR_HPP

#include "sal/config.h"

#include "com/sun/star/drawing/framework/XTabBar.hdl"

#include "com/sun/star/drawing/framework/TabBarButton.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace drawing { namespace framework {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::drawing::framework::XTabBar const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.drawing.framework.XTabBar", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::drawing::framework::XTabBar > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::drawing::framework::XTabBar > >::get();
}

::css::uno::Type const & ::css::drawing::framework::XTabBar::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::drawing::framework::XTabBar >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::drawing::framework::XTabBar>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_DRAWING_FRAMEWORK_XTABBAR_HPP
