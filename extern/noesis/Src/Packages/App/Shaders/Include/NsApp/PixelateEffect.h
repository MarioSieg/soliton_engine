////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_PIXELATEEFFECT_H__
#define __APP_PIXELATEEFFECT_H__


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
/// ShaderEffect that pixelates a visual.
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
///       <noesis:PixelateEffect Size="5"/>
///      </Image.Effect>
///    </Image>
///  </StackPanel>
///
/// .. image:: Pixelate.jpg
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_SHADERS_API PixelateEffect final: public Noesis::ShaderEffect
{
public:
    PixelateEffect();

    /// Gets or sets the amount of pixelation inside the shader
    //@{
    float GetSize() const;
    void SetSize(float value);
    //@}

public:
    static const Noesis::DependencyProperty* SizeProperty;
    static EffectShaders Shaders;

private:
    struct Constants
    {
        float size = 5.0f;
    };

    Constants mConstants;

    NS_DECLARE_REFLECTION(PixelateEffect, ShaderEffect)
};

NS_WARNING_POP

}

#endif
