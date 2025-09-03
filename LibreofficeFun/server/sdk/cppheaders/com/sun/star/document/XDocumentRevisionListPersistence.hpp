#ifndef INCLUDED_COM_SUN_STAR_DOCUMENT_XDOCUMENTREVISIONLISTPERSISTENCE_HPP
#define INCLUDED_COM_SUN_STAR_DOCUMENT_XDOCUMENTREVISIONLISTPERSISTENCE_HPP

#include "sal/config.h"

#include "com/sun/star/document/XDocumentRevisionListPersistence.hdl"

#include "com/sun/star/embed/XStorage.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/util/RevisionTag.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace document {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::document::XDocumentRevisionListPersistence const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.document.XDocumentRevisionListPersistence", 0, 0 );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::document::XDocumentRevisionListPersistence > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::document::XDocumentRevisionListPersistence > >::get();
}

::css::uno::Type const & ::css::document::XDocumentRevisionListPersistence::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::document::XDocumentRevisionListPersistence >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::document::XDocumentRevisionListPersistence>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_DOCUMENT_XDOCUMENTREVISIONLISTPERSISTENCE_HPP
