#ifndef INCLUDED_OOO_VBA_EXCEL_XWORKBOOK_HPP
#define INCLUDED_OOO_VBA_EXCEL_XWORKBOOK_HPP

#include "sal/config.h"

#include "ooo/vba/excel/XWorkbook.hdl"

#include "com/sun/star/lang/XUnoTunnel.hpp"
#include "ooo/vba/excel/XWorksheet.hpp"
#include "com/sun/star/uno/Any.hxx"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "rtl/ustring.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace ooo { namespace vba { namespace excel {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::ooo::vba::excel::XWorkbook const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::lang::XUnoTunnel > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "ooo.vba.excel.XWorkbook", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::ooo::vba::excel::XWorkbook > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::ooo::vba::excel::XWorkbook > >::get();
}

::css::uno::Type const & ::ooo::vba::excel::XWorkbook::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::ooo::vba::excel::XWorkbook >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::ooo::vba::excel::XWorkbook>: ::std::true_type {};
}
#endif

#endif // INCLUDED_OOO_VBA_EXCEL_XWORKBOOK_HPP
