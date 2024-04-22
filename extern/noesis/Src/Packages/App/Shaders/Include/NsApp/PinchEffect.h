////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_PINCHEFFECT_H__
#define __APP_PINCHEFFECT_H__


#include <NsApp/ShadersApi.h>
#include <NsGui/ShaderEffect.h>
#include <NsDrawing/Point.h>
#include <NsRender/RenderContext.h>


namespace Noesis { class DependencyProperty; }

namespace NoesisApp
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251)
NS_CLANG_WARNING_DISABLE("-Wdocumentation")

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Effect to warp or pinch a specific area in a visual.
///
/// .. code-block:: xml
///
///  <StackPanel Orientation="Horizontal"
///    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
///    xmlns:noesis="clr-namespace:NoesisGUIExtensions">
///
///    <Image Source="Images/tulip.png"/>
///    <Image Source="Images/tulip.png">
///      <Image.Effect>
///        <noesis:PinchEffect Center="150,150" Radius="140" Amount="2.0"/>
///      </Image.Effect>
///    </Image>
///
///  </StackPanel>
///
/// .. image:: Pinch.jpg
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_SHADERS_API PinchEffect final: public Noesis::ShaderEffect
{
public:
    PinchEffect();

    /// Gets or sets the origin of the pinch effect in pixels
    //@{
    const Noesis::Point& GetCenter() const;
    void SetCenter(const Noesis::Point& value);
    //@}

    /// Gets or sets the radius of the pinch effect in pixels
    //@{
    float GetRadius() const;
    void SetRadius(float value);
    //@}

    /// Gets or sets the intensity of the pinch effect
    //@{
    float GetAmount() const;
    void SetAmount(float value);
    //@}

public:
    static const Noesis::DependencyProperty* CenterProperty;
    static const Noesis::DependencyProperty* RadiusProperty;
    static const Noesis::DependencyProperty* AmountProperty;
    static EffectShaders Shaders;

private:
    struct Constants
    {
        Noesis::Point center;
        float radius = 0.0f;
        float amount = 0.0f;
    };

    Constants mConstants;

    NS_DECLARE_REFLECTION(PinchEffect, ShaderEffect)
};

NS_WARNING_POP

}

#endif
