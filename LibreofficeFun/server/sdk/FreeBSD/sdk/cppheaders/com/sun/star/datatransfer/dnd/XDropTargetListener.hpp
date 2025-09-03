#ifndef INCLUDED_COM_SUN_STAR_DATATRANSFER_DND_XDROPTARGETLISTENER_HPP
#define INCLUDED_COM_SUN_STAR_DATATRANSFER_DND_XDROPTARGETLISTENER_HPP

#include "sal/config.h"

#include "com/sun/star/datatransfer/dnd/XDropTargetListener.hdl"

#include "com/sun/star/datatransfer/dnd/DropTargetDragEnterEvent.hpp"
#include "com/sun/star/datatransfer/dnd/DropTargetDragEvent.hpp"
#include "com/sun/star/datatransfer/dnd/DropTargetDropEvent.hpp"
#include "com/sun/star/datatransfer/dnd/DropTargetEvent.hpp"
#include "com/sun/star/lang/XEventListener.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace datatransfer { namespace dnd {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::datatransfer::dnd::XDropTargetListener const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::lang::XEventListener > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.datatransfer.dnd.XDropTargetListener", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::datatransfer::dnd::XDropTargetListener > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::datatransfer::dnd::XDropTargetListener > >::get();
}

::css::uno::Type const & ::css::datatransfer::dnd::XDropTargetListener::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::datatransfer::dnd::XDropTargetListener >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::datatransfer::dnd::XDropTargetListener>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_DATATRANSFER_DND_XDROPTARGETLISTENER_HPP
