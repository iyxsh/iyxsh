#ifndef INCLUDED_OOO_VBA_WORD_XRANGE_HPP
#define INCLUDED_OOO_VBA_WORD_XRANGE_HPP

#include "sal/config.h"

#include "ooo/vba/word/XRange.hdl"

#include "com/sun/star/script/XDefaultProperty.hpp"
#include "com/sun/star/text/XTextRange.hpp"
#include "ooo/vba/XHelperInterface.hpp"
#include "ooo/vba/word/XFind.hpp"
#include "ooo/vba/word/XFont.hpp"
#include "ooo/vba/word/XListFormat.hpp"
#include "ooo/vba/word/XParagraphFormat.hpp"
#include "com/sun/star/uno/Any.hxx"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace ooo { namespace vba { namespace word {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::ooo::vba::word::XRange const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[2];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::ooo::vba::XHelperInterface > >::get().getTypeLibType();
        aSuperTypes[1] = ::cppu::UnoType< const ::css::uno::Reference< ::css::script::XDefaultProperty > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "ooo.vba.word.XRange", 2, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::ooo::vba::word::XRange > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::ooo::vba::word::XRange > >::get();
}

::css::uno::Type const & ::ooo::vba::word::XRange::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::ooo::vba::word::XRange >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::ooo::vba::word::XRange>: ::std::true_type {};
}
#endif

#endif // INCLUDED_OOO_VBA_WORD_XRANGE_HPP
