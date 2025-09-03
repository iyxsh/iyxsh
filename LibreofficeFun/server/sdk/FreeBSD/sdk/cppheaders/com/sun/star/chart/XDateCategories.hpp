#ifndef INCLUDED_COM_SUN_STAR_CHART_XDATECATEGORIES_HPP
#define INCLUDED_COM_SUN_STAR_CHART_XDATECATEGORIES_HPP

#include "sal/config.h"

#include "com/sun/star/chart/XDateCategories.hdl"

#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace chart {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::chart::XDateCategories const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.chart.XDateCategories", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::chart::XDateCategories > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::chart::XDateCategories > >::get();
}

::css::uno::Type const & ::css::chart::XDateCategories::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::chart::XDateCategories >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::chart::XDateCategories>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_CHART_XDATECATEGORIES_HPP
