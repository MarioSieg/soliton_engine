////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_DIRECTIONALBLUREFFECT_H__
#define __APP_DIRECTIONALBLUREFFECT_H__


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
/// Blurs an object along a specific angle, to create a blurred streaking effect.
///
/// The directional blur effect is similar to BlurEffect, except you can skew the blur in a
/// particular direction. You can use this effect to make an image look as if it is in motion or
/// to emphasize an animated image.
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
///        <noesis:DirectionalBlurEffect Radius="18" Angle="0"/>
///      </Image.Effect>
///    </Image>
///  </StackPanel>
///
/// .. image:: DirectionalBlur.jpg
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_SHADERS_API DirectionalBlurEffect final: public Noesis::ShaderEffect
{
public:
    DirectionalBlurEffect();

    /// Gets or sets the amount of blur to be applied to the image
    //@{
    float GetRadius() const;
    void SetRadius(float value);
    //@}

    /// Gets or sets the angle of the blur relative to the x-axis, in the counterclockwise
    /// direction. The units are specified in degrees
    //@{
    float GetAngle() const;
    void SetAngle(float value);
    //@}

public:
    static const Noesis::DependencyProperty* RadiusProperty;
    static const Noesis::DependencyProperty* AngleProperty;
    static EffectShaders Shaders;

private:
    struct Constants
    {
        float dirX = 1.0f;
        float dirY = 0.0f;
        float scale = 0.0f;
    };

    Constants mConstants;

    NS_DECLARE_REFLECTION(DirectionalBlurEffect, ShaderEffect)
};

NS_WARNING_POP

}

#endif
