#ifndef INCLUDED_COM_SUN_STAR_SDB_XROWSCHANGEBROADCASTER_HPP
#define INCLUDED_COM_SUN_STAR_SDB_XROWSCHANGEBROADCASTER_HPP

#include "sal/config.h"

#include "com/sun/star/sdb/XRowsChangeBroadcaster.hdl"

#include "com/sun/star/sdb/XRowsChangeListener.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace sdb {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::sdb::XRowsChangeBroadcaster const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.sdb.XRowsChangeBroadcaster", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::sdb::XRowsChangeBroadcaster > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::sdb::XRowsChangeBroadcaster > >::get();
}

::css::uno::Type const & ::css::sdb::XRowsChangeBroadcaster::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::sdb::XRowsChangeBroadcaster >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::sdb::XRowsChangeBroadcaster>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_SDB_XROWSCHANGEBROADCASTER_HPP
