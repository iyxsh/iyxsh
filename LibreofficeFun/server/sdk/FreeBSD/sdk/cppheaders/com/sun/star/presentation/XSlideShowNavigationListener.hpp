#ifndef INCLUDED_COM_SUN_STAR_PRESENTATION_XSLIDESHOWNAVIGATIONLISTENER_HPP
#define INCLUDED_COM_SUN_STAR_PRESENTATION_XSLIDESHOWNAVIGATIONLISTENER_HPP

#include "sal/config.h"

#include "com/sun/star/presentation/XSlideShowNavigationListener.hdl"

#include "com/sun/star/awt/Point.hpp"
#include "com/sun/star/presentation/XSlideShowListener.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace presentation {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::presentation::XSlideShowNavigationListener const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::presentation::XSlideShowListener > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.presentation.XSlideShowNavigationListener", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::presentation::XSlideShowNavigationListener > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::presentation::XSlideShowNavigationListener > >::get();
}

::css::uno::Type const & ::css::presentation::XSlideShowNavigationListener::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::presentation::XSlideShowNavigationListener >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::presentation::XSlideShowNavigationListener>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_PRESENTATION_XSLIDESHOWNAVIGATIONLISTENER_HPP
