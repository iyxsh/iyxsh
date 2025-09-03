#ifndef INCLUDED_COM_SUN_STAR_SDB_ROWCHANGEEVENT_HPP
#define INCLUDED_COM_SUN_STAR_SDB_ROWCHANGEEVENT_HPP

#include "sal/config.h"

#include "com/sun/star/sdb/RowChangeEvent.hdl"

#include "com/sun/star/lang/EventObject.hpp"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "sal/types.h"
#include "typelib/typeclass.h"
#include "typelib/typedescription.h"

namespace com { namespace sun { namespace star { namespace sdb {

inline RowChangeEvent::RowChangeEvent()
    : ::css::lang::EventObject()
    , Action(0)
    , Rows(0)
{
}

inline RowChangeEvent::RowChangeEvent(const ::css::uno::Reference< ::css::uno::XInterface >& Source_, const ::sal_Int32& Action_, const ::sal_Int32& Rows_)
    : ::css::lang::EventObject(Source_)
    , Action(Action_)
    , Rows(Rows_)
{
}


inline bool operator==(const RowChangeEvent& the_lhs, const RowChangeEvent& the_rhs)
{
    return operator==( static_cast<const ::css::lang::EventObject&>(the_lhs), static_cast<const ::css::lang::EventObject&>(the_rhs) )

        && the_lhs.Action == the_rhs.Action
        && the_lhs.Rows == the_rhs.Rows;
}

inline bool operator!=(const RowChangeEvent& the_lhs, const RowChangeEvent& the_rhs)
{
return !operator==(the_lhs, the_rhs);
}
} } } }

namespace com { namespace sun { namespace star { namespace sdb {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::sdb::RowChangeEvent const *) {
    //TODO: On certain platforms with weak memory models, the following code can result in some threads observing that the_type points to garbage
    static ::typelib_TypeDescriptionReference * the_type = 0;
    if (the_type == 0) {
        ::typelib_TypeDescriptionReference * the_members[] = {
            ::cppu::UnoType< ::sal_Int32 >::get().getTypeLibType(),
            ::cppu::UnoType< ::sal_Int32 >::get().getTypeLibType() };
        ::typelib_static_struct_type_init(&the_type, "com.sun.star.sdb.RowChangeEvent", ::cppu::UnoType< ::css::lang::EventObject >::get().getTypeLibType(), 2, the_members, 0);
    }
    return *reinterpret_cast< ::css::uno::Type * >(&the_type);
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::sdb::RowChangeEvent const *) {
    return ::cppu::UnoType< ::css::sdb::RowChangeEvent >::get();
}

#endif // INCLUDED_COM_SUN_STAR_SDB_ROWCHANGEEVENT_HPP
