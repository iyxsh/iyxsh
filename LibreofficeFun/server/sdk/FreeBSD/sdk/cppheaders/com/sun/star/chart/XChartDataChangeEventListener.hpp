#ifndef INCLUDED_COM_SUN_STAR_CHART_XCHARTDATACHANGEEVENTLISTENER_HPP
#define INCLUDED_COM_SUN_STAR_CHART_XCHARTDATACHANGEEVENTLISTENER_HPP

#include "sal/config.h"

#include "com/sun/star/chart/XChartDataChangeEventListener.hdl"

#include "com/sun/star/chart/ChartDataChangeEvent.hpp"
#include "com/sun/star/lang/XEventListener.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace chart {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::chart::XChartDataChangeEventListener const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::lang::XEventListener > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.chart.XChartDataChangeEventListener", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::chart::XChartDataChangeEventListener > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::chart::XChartDataChangeEventListener > >::get();
}

::css::uno::Type const & ::css::chart::XChartDataChangeEventListener::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::chart::XChartDataChangeEventListener >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::chart::XChartDataChangeEventListener>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_CHART_XCHARTDATACHANGEEVENTLISTENER_HPP
