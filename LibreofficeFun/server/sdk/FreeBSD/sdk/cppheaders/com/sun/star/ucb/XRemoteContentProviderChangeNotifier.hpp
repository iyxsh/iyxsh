#ifndef INCLUDED_COM_SUN_STAR_UCB_XREMOTECONTENTPROVIDERCHANGENOTIFIER_HPP
#define INCLUDED_COM_SUN_STAR_UCB_XREMOTECONTENTPROVIDERCHANGENOTIFIER_HPP

#include "sal/config.h"

#include "com/sun/star/ucb/XRemoteContentProviderChangeNotifier.hdl"

#include "com/sun/star/ucb/XRemoteContentProviderChangeListener.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace ucb {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::ucb::XRemoteContentProviderChangeNotifier const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.ucb.XRemoteContentProviderChangeNotifier", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::ucb::XRemoteContentProviderChangeNotifier > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::ucb::XRemoteContentProviderChangeNotifier > >::get();
}

::css::uno::Type const & ::css::ucb::XRemoteContentProviderChangeNotifier::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::ucb::XRemoteContentProviderChangeNotifier >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::ucb::XRemoteContentProviderChangeNotifier>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_UCB_XREMOTECONTENTPROVIDERCHANGENOTIFIER_HPP
