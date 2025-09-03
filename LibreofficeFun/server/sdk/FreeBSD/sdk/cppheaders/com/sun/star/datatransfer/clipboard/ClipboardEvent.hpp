#ifndef INCLUDED_COM_SUN_STAR_DATATRANSFER_CLIPBOARD_CLIPBOARDEVENT_HPP
#define INCLUDED_COM_SUN_STAR_DATATRANSFER_CLIPBOARD_CLIPBOARDEVENT_HPP

#include "sal/config.h"

#include "com/sun/star/datatransfer/clipboard/ClipboardEvent.hdl"

#include "com/sun/star/datatransfer/XTransferable.hpp"
#include "com/sun/star/lang/EventObject.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "sal/types.h"
#include "typelib/typeclass.h"
#include "typelib/typedescription.h"

namespace com { namespace sun { namespace star { namespace datatransfer { namespace clipboard {

inline ClipboardEvent::ClipboardEvent()
    : ::css::lang::EventObject()
    , Contents()
{
}

inline ClipboardEvent::ClipboardEvent(const ::css::uno::Reference< ::css::uno::XInterface >& Source_, const ::css::uno::Reference< ::css::datatransfer::XTransferable >& Contents_)
    : ::css::lang::EventObject(Source_)
    , Contents(Contents_)
{
}


inline bool operator==(const ClipboardEvent& the_lhs, const ClipboardEvent& the_rhs)
{
    return operator==( static_cast<const ::css::lang::EventObject&>(the_lhs), static_cast<const ::css::lang::EventObject&>(the_rhs) )

        && the_lhs.Contents == the_rhs.Contents;
}

inline bool operator!=(const ClipboardEvent& the_lhs, const ClipboardEvent& the_rhs)
{
return !operator==(the_lhs, the_rhs);
}
} } } } }

namespace com { namespace sun { namespace star { namespace datatransfer { namespace clipboard {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::datatransfer::clipboard::ClipboardEvent const *) {
    //TODO: On certain platforms with weak memory models, the following code can result in some threads observing that the_type points to garbage
    static ::typelib_TypeDescriptionReference * the_type = 0;
    if (the_type == 0) {
        ::typelib_TypeDescriptionReference * the_members[] = {
            ::cppu::UnoType< ::css::uno::Reference< ::css::datatransfer::XTransferable > >::get().getTypeLibType() };
        ::typelib_static_struct_type_init(&the_type, "com.sun.star.datatransfer.clipboard.ClipboardEvent", ::cppu::UnoType< ::css::lang::EventObject >::get().getTypeLibType(), 1, the_members, 0);
    }
    return *reinterpret_cast< ::css::uno::Type * >(&the_type);
}

} } } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::datatransfer::clipboard::ClipboardEvent const *) {
    return ::cppu::UnoType< ::css::datatransfer::clipboard::ClipboardEvent >::get();
}

#endif // INCLUDED_COM_SUN_STAR_DATATRANSFER_CLIPBOARD_CLIPBOARDEVENT_HPP
