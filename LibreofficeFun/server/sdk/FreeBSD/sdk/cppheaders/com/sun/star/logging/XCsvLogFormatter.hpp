#ifndef INCLUDED_COM_SUN_STAR_LOGGING_XCSVLOGFORMATTER_HPP
#define INCLUDED_COM_SUN_STAR_LOGGING_XCSVLOGFORMATTER_HPP

#include "sal/config.h"

#include "com/sun/star/logging/XCsvLogFormatter.hdl"

#include "com/sun/star/logging/XLogFormatter.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace logging {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::logging::XCsvLogFormatter const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::logging::XLogFormatter > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.logging.XCsvLogFormatter", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::logging::XCsvLogFormatter > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::logging::XCsvLogFormatter > >::get();
}

::css::uno::Type const & ::css::logging::XCsvLogFormatter::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::logging::XCsvLogFormatter >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::logging::XCsvLogFormatter>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_LOGGING_XCSVLOGFORMATTER_HPP
