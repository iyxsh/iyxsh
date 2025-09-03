#ifndef INCLUDED_COM_SUN_STAR_UCB_REMOTECONTENTPROVIDERCHANGEEVENT_HPP
#define INCLUDED_COM_SUN_STAR_UCB_REMOTECONTENTPROVIDERCHANGEEVENT_HPP

#include "sal/config.h"

#include "com/sun/star/ucb/RemoteContentProviderChangeEvent.hdl"

#include "com/sun/star/lang/EventObject.hpp"
#include "com/sun/star/ucb/RemoteContentProviderChangeAction.hpp"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"
#include "sal/types.h"
#include "typelib/typeclass.h"
#include "typelib/typedescription.h"

namespace com { namespace sun { namespace star { namespace ucb {

inline RemoteContentProviderChangeEvent::RemoteContentProviderChangeEvent()
    : ::css::lang::EventObject()
    , Identifier()
    , Action(::css::ucb::RemoteContentProviderChangeAction_ADDED)
{
}

inline RemoteContentProviderChangeEvent::RemoteContentProviderChangeEvent(const ::css::uno::Reference< ::css::uno::XInterface >& Source_, const ::rtl::OUString& Identifier_, const ::css::ucb::RemoteContentProviderChangeAction& Action_)
    : ::css::lang::EventObject(Source_)
    , Identifier(Identifier_)
    , Action(Action_)
{
}


inline bool operator==(const RemoteContentProviderChangeEvent& the_lhs, const RemoteContentProviderChangeEvent& the_rhs)
{
    return operator==( static_cast<const ::css::lang::EventObject&>(the_lhs), static_cast<const ::css::lang::EventObject&>(the_rhs) )

        && the_lhs.Identifier == the_rhs.Identifier
        && the_lhs.Action == the_rhs.Action;
}

inline bool operator!=(const RemoteContentProviderChangeEvent& the_lhs, const RemoteContentProviderChangeEvent& the_rhs)
{
return !operator==(the_lhs, the_rhs);
}
} } } }

namespace com { namespace sun { namespace star { namespace ucb {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::ucb::RemoteContentProviderChangeEvent const *) {
    //TODO: On certain platforms with weak memory models, the following code can result in some threads observing that the_type points to garbage
    static ::typelib_TypeDescriptionReference * the_type = 0;
    if (the_type == 0) {
        ::typelib_TypeDescriptionReference * the_members[] = {
            ::cppu::UnoType< ::rtl::OUString >::get().getTypeLibType(),
            ::cppu::UnoType< ::css::ucb::RemoteContentProviderChangeAction >::get().getTypeLibType() };
        ::typelib_static_struct_type_init(&the_type, "com.sun.star.ucb.RemoteContentProviderChangeEvent", ::cppu::UnoType< ::css::lang::EventObject >::get().getTypeLibType(), 2, the_members, 0);
    }
    return *reinterpret_cast< ::css::uno::Type * >(&the_type);
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::ucb::RemoteContentProviderChangeEvent const *) {
    return ::cppu::UnoType< ::css::ucb::RemoteContentProviderChangeEvent >::get();
}

#endif // INCLUDED_COM_SUN_STAR_UCB_REMOTECONTENTPROVIDERCHANGEEVENT_HPP
