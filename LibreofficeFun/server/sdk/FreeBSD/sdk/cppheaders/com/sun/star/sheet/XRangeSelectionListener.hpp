#ifndef INCLUDED_COM_SUN_STAR_SHEET_XRANGESELECTIONLISTENER_HPP
#define INCLUDED_COM_SUN_STAR_SHEET_XRANGESELECTIONLISTENER_HPP

#include "sal/config.h"

#include "com/sun/star/sheet/XRangeSelectionListener.hdl"

#include "com/sun/star/lang/XEventListener.hpp"
#include "com/sun/star/sheet/RangeSelectionEvent.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace sheet {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::sheet::XRangeSelectionListener const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::lang::XEventListener > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.sheet.XRangeSelectionListener", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::sheet::XRangeSelectionListener > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::sheet::XRangeSelectionListener > >::get();
}

::css::uno::Type const & ::css::sheet::XRangeSelectionListener::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::sheet::XRangeSelectionListener >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::sheet::XRangeSelectionListener>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_SHEET_XRANGESELECTIONLISTENER_HPP
