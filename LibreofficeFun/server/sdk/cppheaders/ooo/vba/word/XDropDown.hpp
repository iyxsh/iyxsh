#ifndef INCLUDED_OOO_VBA_WORD_XDROPDOWN_HPP
#define INCLUDED_OOO_VBA_WORD_XDROPDOWN_HPP

#include "sal/config.h"

#include "ooo/vba/word/XDropDown.hdl"

#include "com/sun/star/script/XDefaultProperty.hpp"
#include "ooo/vba/XHelperInterface.hpp"
#include "com/sun/star/uno/Any.hxx"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace ooo { namespace vba { namespace word {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::ooo::vba::word::XDropDown const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[2];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::ooo::vba::XHelperInterface > >::get().getTypeLibType();
        aSuperTypes[1] = ::cppu::UnoType< const ::css::uno::Reference< ::css::script::XDefaultProperty > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "ooo.vba.word.XDropDown", 2, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::ooo::vba::word::XDropDown > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::ooo::vba::word::XDropDown > >::get();
}

::css::uno::Type const & ::ooo::vba::word::XDropDown::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::ooo::vba::word::XDropDown >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::ooo::vba::word::XDropDown>: ::std::true_type {};
}
#endif

#endif // INCLUDED_OOO_VBA_WORD_XDROPDOWN_HPP
