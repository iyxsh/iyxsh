#ifndef INCLUDED_COM_SUN_STAR_SDB_TOOLS_XINDEXALTERATION_HPP
#define INCLUDED_COM_SUN_STAR_SDB_TOOLS_XINDEXALTERATION_HPP

#include "sal/config.h"

#include "com/sun/star/sdb/tools/XIndexAlteration.hdl"

#include "com/sun/star/beans/XPropertySet.hpp"
#include "com/sun/star/sdb/tools/XConnectionSupplier.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace sdb { namespace tools {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::sdb::tools::XIndexAlteration const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::sdb::tools::XConnectionSupplier > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.sdb.tools.XIndexAlteration", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::sdb::tools::XIndexAlteration > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::sdb::tools::XIndexAlteration > >::get();
}

::css::uno::Type const & ::css::sdb::tools::XIndexAlteration::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::sdb::tools::XIndexAlteration >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::sdb::tools::XIndexAlteration>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_SDB_TOOLS_XINDEXALTERATION_HPP
