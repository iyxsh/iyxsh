#ifndef INCLUDED_COM_SUN_STAR_DOCUMENT_XINTERACTIONFILTEROPTIONS_HPP
#define INCLUDED_COM_SUN_STAR_DOCUMENT_XINTERACTIONFILTEROPTIONS_HPP

#include "sal/config.h"

#include "com/sun/star/document/XInteractionFilterOptions.hdl"

#include "com/sun/star/beans/PropertyValue.hpp"
#include "com/sun/star/task/XInteractionContinuation.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace document {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::document::XInteractionFilterOptions const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::task::XInteractionContinuation > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.document.XInteractionFilterOptions", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::document::XInteractionFilterOptions > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::document::XInteractionFilterOptions > >::get();
}

::css::uno::Type const & ::css::document::XInteractionFilterOptions::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::document::XInteractionFilterOptions >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::document::XInteractionFilterOptions>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_DOCUMENT_XINTERACTIONFILTEROPTIONS_HPP
