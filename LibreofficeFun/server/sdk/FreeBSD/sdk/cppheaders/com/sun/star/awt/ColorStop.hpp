#ifndef INCLUDED_COM_SUN_STAR_AWT_COLORSTOP_HPP
#define INCLUDED_COM_SUN_STAR_AWT_COLORSTOP_HPP

#include "sal/config.h"

#include "com/sun/star/awt/ColorStop.hdl"

#include "com/sun/star/rendering/RGBColor.hpp"
#include "com/sun/star/uno/Type.hxx"
#include "cppu/unotype.hxx"
#include "sal/types.h"
#include "typelib/typeclass.h"
#include "typelib/typedescription.h"

namespace com { namespace sun { namespace star { namespace awt {

inline ColorStop::ColorStop()
    : StopOffset(0)
    , StopColor()
{
}

inline ColorStop::ColorStop(const double& StopOffset_, const ::css::rendering::RGBColor& StopColor_)
    : StopOffset(StopOffset_)
    , StopColor(StopColor_)
{
}


inline bool operator==(const ColorStop& the_lhs, const ColorStop& the_rhs)
{
    return the_lhs.StopOffset == the_rhs.StopOffset
        && the_lhs.StopColor == the_rhs.StopColor;
}

inline bool operator!=(const ColorStop& the_lhs, const ColorStop& the_rhs)
{
return !operator==(the_lhs, the_rhs);
}
} } } }

namespace com { namespace sun { namespace star { namespace awt {

inline ::css::uno::Type const & cppu_detail_getUnoType(SAL_UNUSED_PARAMETER ::css::awt::ColorStop const *) {
    //TODO: On certain platforms with weak memory models, the following code can result in some threads observing that the_type points to garbage
    static ::typelib_TypeDescriptionReference * the_type = 0;
    if (the_type == 0) {
        ::typelib_TypeDescriptionReference * the_members[] = {
            ::cppu::UnoType< double >::get().getTypeLibType(),
            ::cppu::UnoType< ::css::rendering::RGBColor >::get().getTypeLibType() };
        ::typelib_static_struct_type_init(&the_type, "com.sun.star.awt.ColorStop", 0, 2, the_members, 0);
    }
    return *reinterpret_cast< ::css::uno::Type * >(&the_type);
}

} } } }

SAL_DEPRECATED("use cppu::UnoType") inline ::css::uno::Type const & SAL_CALL getCppuType(SAL_UNUSED_PARAMETER ::css::awt::ColorStop const *) {
    return ::cppu::UnoType< ::css::awt::ColorStop >::get();
}

#endif // INCLUDED_COM_SUN_STAR_AWT_COLORSTOP_HPP
