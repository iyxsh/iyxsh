#ifndef INCLUDED_COM_SUN_STAR_TABLE_XMERGEABLECELLRANGE_HPP
#define INCLUDED_COM_SUN_STAR_TABLE_XMERGEABLECELLRANGE_HPP

#include "sal/config.h"

#include "com/sun/star/table/XMergeableCellRange.hdl"

#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace table {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::table::XMergeableCellRange const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.table.XMergeableCellRange", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::table::XMergeableCellRange > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::table::XMergeableCellRange > >::get();
}

::css::uno::Type const & ::css::table::XMergeableCellRange::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::table::XMergeableCellRange >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::table::XMergeableCellRange>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_TABLE_XMERGEABLECELLRANGE_HPP
