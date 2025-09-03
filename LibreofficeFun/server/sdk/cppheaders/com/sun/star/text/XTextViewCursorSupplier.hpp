#ifndef INCLUDED_COM_SUN_STAR_TEXT_XTEXTVIEWCURSORSUPPLIER_HPP
#define INCLUDED_COM_SUN_STAR_TEXT_XTEXTVIEWCURSORSUPPLIER_HPP

#include "sal/config.h"

#include "com/sun/star/text/XTextViewCursorSupplier.hdl"

#include "com/sun/star/text/XTextViewCursor.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace text {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::text::XTextViewCursorSupplier const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.text.XTextViewCursorSupplier", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::text::XTextViewCursorSupplier > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::text::XTextViewCursorSupplier > >::get();
}

::css::uno::Type const & ::css::text::XTextViewCursorSupplier::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::text::XTextViewCursorSupplier >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::text::XTextViewCursorSupplier>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_TEXT_XTEXTVIEWCURSORSUPPLIER_HPP
