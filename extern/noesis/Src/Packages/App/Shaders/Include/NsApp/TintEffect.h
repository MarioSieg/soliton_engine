////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_TINTEFFECT_H__
#define __APP_TINTEFFECT_H__


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
/// This effect tints the source image by multiplying the source image by the specified color
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
///        <noesis:TintEffect Color="Orange"/>
///      </Image.Effect>
///    </Image>
///  </StackPanel>
///
/// .. image:: Tint.jpg
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_SHADERS_API TintEffect final: public Noesis::ShaderEffect
{
public:
    TintEffect();

public:
    static const Noesis::DependencyProperty* ColorProperty;
    static EffectShaders Shaders;

    /// Gets or Sets the color used to multiply the source image
    //@{
    const Noesis::Color& GetColor() const;
    void SetColor(const Noesis::Color& value);
    //@}

private:
    struct Constants
    {
        Noesis::Color color = Noesis::Color::Blue();
    };

    Constants mConstants;

    NS_DECLARE_REFLECTION(TintEffect, ShaderEffect)
};

NS_WARNING_POP

}

#endif
