#ifndef INCLUDED_COM_SUN_STAR_DRAWING_FRAMEWORK_XTOOLBAR_HPP
#define INCLUDED_COM_SUN_STAR_DRAWING_FRAMEWORK_XTOOLBAR_HPP

#include "sal/config.h"

#include "com/sun/star/drawing/framework/XToolBar.hdl"

#include "com/sun/star/drawing/framework/XResource.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace drawing { namespace framework {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::drawing::framework::XToolBar const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::drawing::framework::XResource > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.drawing.framework.XToolBar", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::drawing::framework::XToolBar > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::drawing::framework::XToolBar > >::get();
}

::css::uno::Type const & ::css::drawing::framework::XToolBar::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::drawing::framework::XToolBar >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::drawing::framework::XToolBar>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_DRAWING_FRAMEWORK_XTOOLBAR_HPP
