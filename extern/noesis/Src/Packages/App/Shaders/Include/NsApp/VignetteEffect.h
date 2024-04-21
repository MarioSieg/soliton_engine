////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_VIGNETTEEFFECT_H__
#define __APP_VIGNETTEEFFECT_H__


#include <NsApp/ShadersApi.h>
#include <NsGui/ShaderEffect.h>
#include <NsDrawing/Color.h>
#include <NsRender/RenderContext.h>


namespace Noesis { class DependencyProperty; }

namespace NoesisApp
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251)
NS_CLANG_WARNING_DISABLE("-Wdocumentation")

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Fades the input image at the edges to a user-set color.
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
///        <noesis:VignetteEffect Strength="30" Size="0.50"/>
///      </Image.Effect>
///    </Image>
///  </StackPanel>
///
/// .. image:: Vignette.jpg
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_SHADERS_API VignetteEffect final: public Noesis::ShaderEffect
{
public:
    VignetteEffect();

    /// Gets or sets the RGB tripplet that specifies the color to fade the image's edges to
    //@{
    const Noesis::Color& GetColor() const;
    void SetColor(const Noesis::Color& value);
    //@}

    /// Gets or sets a float value that specifies how much the vignette color bleeds in for a 
    /// given transition size
    //@{
    float GetStrength() const;
    void SetStrength(float value);
    //@}

    /// Gets or sets a float value that specifies the size of the vignette region as a percentage
    /// of the full image region
    //@{
    float GetSize() const;
    void SetSize(float value);
    //@}

public:
    static const Noesis::DependencyProperty* ColorProperty;
    static const Noesis::DependencyProperty* StrengthProperty;
    static const Noesis::DependencyProperty* SizeProperty;
    static EffectShaders Shaders;

private:
    struct Constants
    {
        Noesis::Color color = Noesis::Color::Black();
        float strength = 15.0f;
        float size = 0.25f;
    };

    Constants mConstants;

    NS_DECLARE_REFLECTION(VignetteEffect, ShaderEffect)
};

NS_WARNING_POP

}

#endif
