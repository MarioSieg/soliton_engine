////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_SATURATIONEFFECT_H__
#define __APP_SATURATIONEFFECT_H__


#include <NsApp/ShadersApi.h>
#include <NsGui/ShaderEffect.h>
#include <NsRender/RenderContext.h>


namespace Noesis { class DependencyProperty; }

namespace NoesisApp
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251)
NS_CLANG_WARNING_DISABLE("-Wdocumentation")

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Use this effect to alter the saturation of an image.
///
/// .. code-block:: xml
///
///  <StackPanel
///    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
///    xmlns:noesis="clr-namespace:NoesisGUIExtensions"
///    Orientation="Horizontal">
///    <Image Source="Images/tulip.png"/>
///    <Image Source="Images/tulip.png">
///      <Image.Effect>
///       <noesis:SaturationEffect Saturation="0"/>
///      </Image.Effect>
///    </Image>
///  </StackPanel>
///
/// .. image:: Saturation.jpg
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_SHADERS_API SaturationEffect final: public Noesis::ShaderEffect
{
public:
    SaturationEffect();

    /// Gets or sets the saturation of the image.  You can set the saturation to a value between 
    /// 0 and 1. If you set it to 1 the output image is fully saturated. If you set it to 0 the 
    /// output image is monochrome
    //@{
    float GetSaturation() const;
    void SetSaturation(float value);
    //@}

public:
    static const Noesis::DependencyProperty* SaturationProperty;
    static EffectShaders Shaders;

private:
    struct Constants
    {
        float saturation = 1.0f;
    };

    Constants mConstants;

    NS_DECLARE_REFLECTION(SaturationEffect, ShaderEffect)
};

NS_WARNING_POP

}

#endif
