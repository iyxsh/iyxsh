#ifndef INCLUDED_COM_SUN_STAR_TEXT_XTEXTGRAPHICOBJECTSSUPPLIER_HPP
#define INCLUDED_COM_SUN_STAR_TEXT_XTEXTGRAPHICOBJECTSSUPPLIER_HPP

#include "sal/config.h"

#include "com/sun/star/text/XTextGraphicObjectsSupplier.hdl"

#include "com/sun/star/container/XNameAccess.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace text {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::text::XTextGraphicObjectsSupplier const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.text.XTextGraphicObjectsSupplier", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::text::XTextGraphicObjectsSupplier > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::text::XTextGraphicObjectsSupplier > >::get();
}

::css::uno::Type const & ::css::text::XTextGraphicObjectsSupplier::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::text::XTextGraphicObjectsSupplier >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::text::XTextGraphicObjectsSupplier>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_TEXT_XTEXTGRAPHICOBJECTSSUPPLIER_HPP
