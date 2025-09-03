#ifndef INCLUDED_COM_SUN_STAR_TASK_ERRORCODEREQUEST2_HPP
#define INCLUDED_COM_SUN_STAR_TASK_ERRORCODEREQUEST2_HPP

#include "sal/config.h"

#include "com/sun/star/task/ErrorCodeRequest2.hdl"

#include "com/sun/star/task/ErrorCodeRequest.hpp"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"
#include "sal/types.h"

namespace com { namespace sun { namespace star { namespace task {

inline ErrorCodeRequest2::ErrorCodeRequest2(
#if defined LIBO_USE_SOURCE_LOCATION
    LIBO_USE_SOURCE_LOCATION::source_location location
#endif
    )
    : ::css::task::ErrorCodeRequest(
#if defined LIBO_USE_SOURCE_LOCATION
    location
#endif
)
    , Arg1()
    , Arg2()
    , DialogMask(0)
{ }

inline ErrorCodeRequest2::ErrorCodeRequest2(const ::rtl::OUString& Message_, const ::css::uno::Reference< ::css::uno::XInterface >& Context_, const ::sal_Int32& ErrCode_, const ::rtl::OUString& Arg1_, const ::rtl::OUString& Arg2_, const ::sal_Int16& DialogMask_
#if defined LIBO_USE_SOURCE_LOCATION
    , LIBO_USE_SOURCE_LOCATION::source_location location
#endif
)
    : ::css::task::ErrorCodeRequest(Message_, Context_, ErrCode_
#if defined LIBO_USE_SOURCE_LOCATION
    , location
#endif
)
    , Arg1(Arg1_)
    , Arg2(Arg2_)
    , DialogMask(DialogMask_)
{ }

#if !defined LIBO_INTERNAL_ONLY
ErrorCodeRequest2::ErrorCodeRequest2(ErrorCodeRequest2 const & the_other): ::css::task::ErrorCodeRequest(the_other), Arg1(the_other.Arg1), Arg2(the_other.Arg2), DialogMask(the_other.DialogMask) {}

ErrorCodeRequest2::~ErrorCodeRequest2() {}

ErrorCodeRequest2 & ErrorCodeRequest2::operator =(ErrorCodeRequest2 const & the_other) {
    //TODO: Just like its implicitly-defined counterpart, this function definition is not exception-safe
    ::css::task::ErrorCodeRequest::operator =(the_other);
    Arg1 = the_other.Arg1;
    Arg2 = the_other.Arg2;
    DialogMask = the_other.DialogMask;
    return *this;
}
#endif

} } } }

namespace com { namespace sun { namespace star { namespace task {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::task::ErrorCodeRequest2 const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        const ::css::uno::Type& rBaseType = ::cppu::UnoType< const ::css::task::ErrorCodeRequest >::get();

        typelib_TypeDescriptionReference * aMemberRefs[3];
        const ::css::uno::Type& rMemberType_string = ::cppu::UnoType< ::rtl::OUString >::get();
        aMemberRefs[0] = rMemberType_string.getTypeLibType();
        aMemberRefs[1] = rMemberType_string.getTypeLibType();
        const ::css::uno::Type& rMemberType_short = ::cppu::UnoType< ::sal_Int16 >::get();
        aMemberRefs[2] = rMemberType_short.getTypeLibType();

        typelib_static_compound_type_init( &the_type, typelib_TypeClass_EXCEPTION, "com.sun.star.task.ErrorCodeRequest2", rBaseType.getTypeLibType(), 3,  aMemberRefs );
    }
    return * reinterpret_cast< const ::css::uno::Type * >( &the_type );
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::task::ErrorCodeRequest2 const *) {
    return ::cppu::UnoType< ::css::task::ErrorCodeRequest2 >::get();
}

#endif // INCLUDED_COM_SUN_STAR_TASK_ERRORCODEREQUEST2_HPP
