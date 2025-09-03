#ifndef INCLUDED_COM_SUN_STAR_SDB_XSINGLESELECTQUERYCOMPOSER_HPP
#define INCLUDED_COM_SUN_STAR_SDB_XSINGLESELECTQUERYCOMPOSER_HPP

#include "sal/config.h"

#include "com/sun/star/sdb/XSingleSelectQueryComposer.hdl"

#include "com/sun/star/beans/PropertyValue.hpp"
#include "com/sun/star/beans/XPropertySet.hpp"
#include "com/sun/star/sdb/XSingleSelectQueryAnalyzer.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace sdb {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::sdb::XSingleSelectQueryComposer const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::sdb::XSingleSelectQueryAnalyzer > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.sdb.XSingleSelectQueryComposer", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::sdb::XSingleSelectQueryComposer > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::sdb::XSingleSelectQueryComposer > >::get();
}

::css::uno::Type const & ::css::sdb::XSingleSelectQueryComposer::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::sdb::XSingleSelectQueryComposer >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::sdb::XSingleSelectQueryComposer>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_SDB_XSINGLESELECTQUERYCOMPOSER_HPP
