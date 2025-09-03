#ifndef INCLUDED_COM_SUN_STAR_UCB_LISTEVENT_HPP
#define INCLUDED_COM_SUN_STAR_UCB_LISTEVENT_HPP

#include "sal/config.h"

#include "com/sun/star/ucb/ListEvent.hdl"

#include "com/sun/star/lang/EventObject.hpp"
#include "com/sun/star/ucb/ListAction.hpp"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "sal/types.h"
#include "typelib/typeclass.h"
#include "typelib/typedescription.h"

namespace com { namespace sun { namespace star { namespace ucb {

inline ListEvent::ListEvent()
    : ::css::lang::EventObject()
    , Changes()
{
}

inline ListEvent::ListEvent(const ::css::uno::Reference< ::css::uno::XInterface >& Source_, const ::css::uno::Sequence< ::css::ucb::ListAction >& Changes_)
    : ::css::lang::EventObject(Source_)
    , Changes(Changes_)
{
}


inline bool operator==(const ListEvent& the_lhs, const ListEvent& the_rhs)
{
    return operator==( static_cast<const ::css::lang::EventObject&>(the_lhs), static_cast<const ::css::lang::EventObject&>(the_rhs) )

        && the_lhs.Changes == the_rhs.Changes;
}

inline bool operator!=(const ListEvent& the_lhs, const ListEvent& the_rhs)
{
return !operator==(the_lhs, the_rhs);
}
} } } }

namespace com { namespace sun { namespace star { namespace ucb {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::ucb::ListEvent const *) {
    //TODO: On certain platforms with weak memory models, the following code can result in some threads observing that the_type points to garbage
    static ::typelib_TypeDescriptionReference * the_type = 0;
    if (the_type == 0) {
        ::typelib_TypeDescriptionReference * the_members[] = {
            ::cppu::UnoType< ::cppu::UnoSequenceType< ::css::ucb::ListAction > >::get().getTypeLibType() };
        ::typelib_static_struct_type_init(&the_type, "com.sun.star.ucb.ListEvent", ::cppu::UnoType< ::css::lang::EventObject >::get().getTypeLibType(), 1, the_members, 0);
    }
    return *reinterpret_cast< ::css::uno::Type * >(&the_type);
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::ucb::ListEvent const *) {
    return ::cppu::UnoType< ::css::ucb::ListEvent >::get();
}

#endif // INCLUDED_COM_SUN_STAR_UCB_LISTEVENT_HPP
