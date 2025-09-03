#ifndef INCLUDED_COM_SUN_STAR_FORM_VALIDATION_XVALIDATABLEFORMCOMPONENT_HPP
#define INCLUDED_COM_SUN_STAR_FORM_VALIDATION_XVALIDATABLEFORMCOMPONENT_HPP

#include "sal/config.h"

#include "com/sun/star/form/validation/XValidatableFormComponent.hdl"

#include "com/sun/star/form/validation/XFormComponentValidityListener.hpp"
#include "com/sun/star/form/validation/XValidatable.hpp"
#include "com/sun/star/uno/Any.hxx"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "sal/types.h"

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

namespace com { namespace sun { namespace star { namespace form { namespace validation {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::form::validation::XValidatableFormComponent const *) {
    static typelib_TypeDescriptionReference * the_type = 0;
    if ( !the_type )
    {
        typelib_TypeDescriptionReference * aSuperTypes[1];
        aSuperTypes[0] = ::cppu::UnoType< const ::css::uno::Reference< ::css::form::validation::XValidatable > >::get().getTypeLibType();
        typelib_static_mi_interface_type_init( &the_type, "com.sun.star.form.validation.XValidatableFormComponent", 1, aSuperTypes );
    }
    return * reinterpret_cast< ::css::uno::Type * >( &the_type );
}

} } } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::uno::Reference< ::css::form::validation::XValidatableFormComponent > const *) {
    return ::cppu::UnoType< ::css::uno::Reference< ::css::form::validation::XValidatableFormComponent > >::get();
}

::css::uno::Type const & ::css::form::validation::XValidatableFormComponent::static_type(SAL_UNUSED_PARAMETER void *) {
    return ::cppu::UnoType< ::css::form::validation::XValidatableFormComponent >::get();
}

#if defined LIBO_INTERNAL_ONLY
namespace cppu::detail {
template<> struct IsUnoInterfaceType<::css::form::validation::XValidatableFormComponent>: ::std::true_type {};
}
#endif

#endif // INCLUDED_COM_SUN_STAR_FORM_VALIDATION_XVALIDATABLEFORMCOMPONENT_HPP
