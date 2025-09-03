#ifndef INCLUDED_COM_SUN_STAR_XML_DOM_XPROCESSINGINSTRUCTION_HPP
#define INCLUDED_COM_SUN_STAR_XML_DOM_XPROCESSINGINSTRUCTION_HPP

#include "sal/config.h"

#include "com/sun/star/xml/dom/XProcessingInstruction.hdl"

#include "com/sun/star/xml/dom/XNode.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace xml { namespace dom {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::xml::dom::XProcessingInstruction const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::xml::dom::XNode > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.xml.dom.XProcessingInstruction", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::xml::dom::XProcessingInstruction > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::xml::dom::XProcessingInstruction > >::get();
}

::css::uno::Type const & ::css::xml::dom::XProcessingInstruction::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::xml::dom::XProcessingInstruction >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::xml::dom::XProcessingInstruction>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_XML_DOM_XPROCESSINGINSTRUCTION_HPP
