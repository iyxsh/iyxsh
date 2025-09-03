#ifndef INCLUDED_COM_SUN_STAR_ACCESSIBILITY_XACCESSIBLEKEYBINDING_HPP
#define INCLUDED_COM_SUN_STAR_ACCESSIBILITY_XACCESSIBLEKEYBINDING_HPP

#include "sal/config.h"

#include "com/sun/star/accessibility/XAccessibleKeyBinding.hdl"

#include "com/sun/star/awt/KeyStroke.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace accessibility {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::accessibility::XAccessibleKeyBinding const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.accessibility.XAccessibleKeyBinding", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::accessibility::XAccessibleKeyBinding > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::accessibility::XAccessibleKeyBinding > >::get();
}

::css::uno::Type const & ::css::accessibility::XAccessibleKeyBinding::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::accessibility::XAccessibleKeyBinding >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::accessibility::XAccessibleKeyBinding>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_ACCESSIBILITY_XACCESSIBLEKEYBINDING_HPP
