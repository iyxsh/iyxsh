#ifndef INCLUDED_COM_SUN_STAR_RENDERING_XCOLORSPACE_HPP
#define INCLUDED_COM_SUN_STAR_RENDERING_XCOLORSPACE_HPP

#include "sal/config.h"

#include "com/sun/star/rendering/XColorSpace.hdl"

#include "com/sun/star/beans/PropertyValue.hpp"
#include "com/sun/star/rendering/ARGBColor.hpp"
#include "com/sun/star/rendering/ColorComponent.hpp"
#include "com/sun/star/rendering/RGBColor.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace rendering {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::rendering::XColorSpace const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.rendering.XColorSpace", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::rendering::XColorSpace > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::rendering::XColorSpace > >::get();
}

::css::uno::Type const & ::css::rendering::XColorSpace::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::rendering::XColorSpace >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::rendering::XColorSpace>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_RENDERING_XCOLORSPACE_HPP
