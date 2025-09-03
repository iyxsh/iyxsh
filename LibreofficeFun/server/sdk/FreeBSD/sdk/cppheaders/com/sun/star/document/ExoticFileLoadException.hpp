#ifndef INCLUDED_COM_SUN_STAR_DOCUMENT_EXOTICFILELOADEXCEPTION_HPP
#define INCLUDED_COM_SUN_STAR_DOCUMENT_EXOTICFILELOADEXCEPTION_HPP

#include "sal/config.h"

#include "com/sun/star/document/ExoticFileLoadException.hdl"

#include "com/sun/star/uno/Exception.hpp"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"

namespace com { namespace sun { namespace star { namespace document {

inline ExoticFileLoadException::ExoticFileLoadException(
#if defined LIBO_USE_SOURCE_LOCATION
    LIBO_USE_SOURCE_LOCATION::source_location location
#endif
    )
    : ::css::uno::Exception(
#if defined LIBO_USE_SOURCE_LOCATION
    location
#endif
)
    , URL()
    , FilterUIName()
{ }

inline ExoticFileLoadException::ExoticFileLoadException(const ::rtl::OUString& Message_, const ::css::uno::Reference< ::css::uno::XInterface >& Context_, const ::rtl::OUString& URL_, const ::rtl::OUString& FilterUIName_
#if defined LIBO_USE_SOURCE_LOCATION
    , LIBO_USE_SOURCE_LOCATION::source_location location
#endif
)
    : ::css::uno::Exception(Message_, Context_
#if defined LIBO_USE_SOURCE_LOCATION
    , location
#endif
)
    , URL(URL_)
    , FilterUIName(FilterUIName_)
{ }

#if !defined LIBO_INTERNAL_ONLY
ExoticFileLoadException::ExoticFileLoadException(ExoticFileLoadException const & the_other): ::css::uno::Exception(the_other), URL(the_other.URL), FilterUIName(the_other.FilterUIName) {}

ExoticFileLoadException::~ExoticFileLoadException() {}

ExoticFileLoadException & ExoticFileLoadException::operator =(ExoticFileLoadException const & the_other) {
    //TODO: Just like its implicitly-defined counterpart, this function definition is not exception-safe
    ::css::uno::Exception::operator =(the_other);
    URL = the_other.URL;
    FilterUIName = the_other.FilterUIName;
    return *this;
}
#endif

} } } }

namespace com { namespace sun { namespace star { namespace document {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::document::ExoticFileLoadException const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aMemberRefs[2];
        const ::css::uno::Type& rMemberType_string = ::cppu::UnoType< ::rtl::OUString >::get();
        aMemberRefs[0] = rMemberType_string.getTypeLibType();
        aMemberRefs[1] = rMemberType_string.getTypeLibType();

        typelib_static_compound_type_init( &the_type, typelib_TypeClass_EXCEPTION, "com.sun.star.document.ExoticFileLoadException", * ::typelib_static_type_getByTypeClass( typelib_TypeClass_EXCEPTION ), 2,  aMemberRefs );
    }
    return * reinterpret_cast< const ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::document::ExoticFileLoadException const *) {
    return ::cppu::UnoType< ::css::document::ExoticFileLoadException >::get();
}

#endif // INCLUDED_COM_SUN_STAR_DOCUMENT_EXOTICFILELOADEXCEPTION_HPP
