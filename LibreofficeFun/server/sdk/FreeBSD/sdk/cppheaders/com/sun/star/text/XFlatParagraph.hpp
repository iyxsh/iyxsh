#ifndef INCLUDED_COM_SUN_STAR_TEXT_XFLATPARAGRAPH_HPP
#define INCLUDED_COM_SUN_STAR_TEXT_XFLATPARAGRAPH_HPP

#include "sal/config.h"

#include "com/sun/star/text/XFlatParagraph.hdl"

#include "com/sun/star/beans/PropertyValue.hpp"
#include "com/sun/star/lang/Locale.hpp"
#include "com/sun/star/text/XTextMarkup.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace text {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::text::XFlatParagraph const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::text::XTextMarkup > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.text.XFlatParagraph", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::text::XFlatParagraph > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::text::XFlatParagraph > >::get();
}

::css::uno::Type const & ::css::text::XFlatParagraph::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::text::XFlatParagraph >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::text::XFlatParagraph>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_TEXT_XFLATPARAGRAPH_HPP
