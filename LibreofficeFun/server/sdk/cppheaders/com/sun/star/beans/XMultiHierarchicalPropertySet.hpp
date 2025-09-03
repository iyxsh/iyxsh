#ifndef INCLUDED_COM_SUN_STAR_BEANS_XMULTIHIERARCHICALPROPERTYSET_HPP
#define INCLUDED_COM_SUN_STAR_BEANS_XMULTIHIERARCHICALPROPERTYSET_HPP

#include "sal/config.h"

#include "com/sun/star/beans/XMultiHierarchicalPropertySet.hdl"

#include "com/sun/star/beans/XHierarchicalPropertySetInfo.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Any.hxx"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace beans {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::beans::XMultiHierarchicalPropertySet const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.beans.XMultiHierarchicalPropertySet", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::beans::XMultiHierarchicalPropertySet > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::beans::XMultiHierarchicalPropertySet > >::get();
}

::css::uno::Type const & ::css::beans::XMultiHierarchicalPropertySet::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::beans::XMultiHierarchicalPropertySet >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::beans::XMultiHierarchicalPropertySet>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_BEANS_XMULTIHIERARCHICALPROPERTYSET_HPP
