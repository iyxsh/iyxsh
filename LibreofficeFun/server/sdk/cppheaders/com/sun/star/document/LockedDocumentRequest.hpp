#ifndef INCLUDED_COM_SUN_STAR_DOCUMENT_LOCKEDDOCUMENTREQUEST_HPP
#define INCLUDED_COM_SUN_STAR_DOCUMENT_LOCKEDDOCUMENTREQUEST_HPP

#include "sal/config.h"

#include "com/sun/star/document/LockedDocumentRequest.hdl"

#include "com/sun/star/uno/Exception.hpp"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"

namespace com { namespace sun { namespace star { namespace document {

inline LockedDocumentRequest::LockedDocumentRequest(
#if defined LIBO_USE_SOURCE_LOCATION
    LIBO_USE_SOURCE_LOCATION::source_location location
#endif
    )
    : ::css::uno::Exception(
#if defined LIBO_USE_SOURCE_LOCATION
    location
#endif
)
    , DocumentURL()
    , UserInfo()
{ }

inline LockedDocumentRequest::LockedDocumentRequest(const ::rtl::OUString& Message_, const ::css::uno::Reference< ::css::uno::XInterface >& Context_, const ::rtl::OUString& DocumentURL_, const ::rtl::OUString& UserInfo_
#if defined LIBO_USE_SOURCE_LOCATION
    , LIBO_USE_SOURCE_LOCATION::source_location location
#endif
)
    : ::css::uno::Exception(Message_, Context_
#if defined LIBO_USE_SOURCE_LOCATION
    , location
#endif
)
    , DocumentURL(DocumentURL_)
    , UserInfo(UserInfo_)
{ }

#if !defined LIBO_INTERNAL_ONLY
LockedDocumentRequest::LockedDocumentRequest(LockedDocumentRequest const & the_other): ::css::uno::Exception(the_other), DocumentURL(the_other.DocumentURL), UserInfo(the_other.UserInfo) {}

LockedDocumentRequest::~LockedDocumentRequest() {}

LockedDocumentRequest & LockedDocumentRequest::operator =(LockedDocumentRequest const & the_other) {
    //TODO: Just like its implicitly-defined counterpart, this function definition is not exception-safe
    ::css::uno::Exception::operator =(the_other);
    DocumentURL = the_other.DocumentURL;
    UserInfo = the_other.UserInfo;
    return *this;
}
#endif

} } } }

namespace com { namespace sun { namespace star { namespace document {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::document::LockedDocumentRequest const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aMemberRefs[2];
        const ::css::uno::Type& rMemberType_string = ::cppu::UnoType< ::rtl::OUString >::get();
        aMemberRefs[0] = rMemberType_string.getTypeLibType();
        aMemberRefs[1] = rMemberType_string.getTypeLibType();

        typelib_static_compound_type_init( &the_type, typelib_TypeClass_EXCEPTION, "com.sun.star.document.LockedDocumentRequest", * ::typelib_static_type_getByTypeClass( typelib_TypeClass_EXCEPTION ), 2,  aMemberRefs );
    }
    return * reinterpret_cast< const ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::document::LockedDocumentRequest const *) {
    return ::cppu::UnoType< ::css::document::LockedDocumentRequest >::get();
}

#endif // INCLUDED_COM_SUN_STAR_DOCUMENT_LOCKEDDOCUMENTREQUEST_HPP
