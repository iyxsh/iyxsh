#ifndef INCLUDED_OOO_VBA_MSFORMS_XRADIOBUTTON_HPP
#define INCLUDED_OOO_VBA_MSFORMS_XRADIOBUTTON_HPP

#include "sal/config.h"

#include "ooo/vba/msforms/XRadioButton.hdl"

#include "com/sun/star/uno/XInterface.hpp"
#include "ooo/vba/msforms/XNewFont.hpp"
#include "com/sun/star/uno/Any.hxx"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace ooo { namespace vba { namespace msforms {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::ooo::vba::msforms::XRadioButton const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "ooo.vba.msforms.XRadioButton", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::ooo::vba::msforms::XRadioButton > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::ooo::vba::msforms::XRadioButton > >::get();
}

::css::uno::Type const & ::ooo::vba::msforms::XRadioButton::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::ooo::vba::msforms::XRadioButton >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::ooo::vba::msforms::XRadioButton>: ::std::true_type {};
}
#endif

#endif // INCLUDED_OOO_VBA_MSFORMS_XRADIOBUTTON_HPP
