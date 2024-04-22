////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_WAVESBRUSH_H__
#define __APP_WAVESBRUSH_H__


#include <NsApp/ShadersApi.h>
#include <NsGui/BrushShader.h>
#include <NsRender/RenderContext.h>


namespace Noesis { class DependencyProperty; }

namespace NoesisApp
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251)
NS_CLANG_WARNING_DISABLE("-Wdocumentation")

////////////////////////////////////////////////////////////////////////////////////////////////////
/// BrushShader that paints a PSP wavy background effect.
///
/// .. code-block:: xml
///
///  <Rectangle
///    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
///    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
///    xmlns:noesis="clr-namespace:NoesisGUIExtensions;assembly=Noesis.GUI.Extensions"
///    Width="600" Height="300">
///    <Rectangle.Resources>
///      <Storyboard x:Key="anim">
///        <DoubleAnimation By="864000" Duration="10.0:0:0" Storyboard.TargetName="shader" Storyboard.TargetProperty="Time"/>
///      </Storyboard>
///    </Rectangle.Resources>
///    <Rectangle.Triggers>
///      <EventTrigger RoutedEvent="FrameworkElement.Loaded">
///        <BeginStoryboard Storyboard="{StaticResource anim}"/>
///      </EventTrigger>
///    </Rectangle.Triggers>
///    <Rectangle.Fill>
///      <ImageBrush>
///        <noesis:Brush.Shader>
///          <noesis:WavesBrush x:name="shader" />
///        </noesis:Brush.Shader>
///      </ImageBrush>
///    </Rectangle.Fill>
///  </Rectangle>
///
/// .. image:: Waves.jpg
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_SHADERS_API WavesBrush final: public Noesis::BrushShader
{
public:
    WavesBrush();

    /// Gets or sets the time of the effect
    //@{
    float GetTime() const;
    void SetTime(float value);
    //@}

public:
    static const Noesis::DependencyProperty* TimeProperty;
    static BrushShaders Shaders;

private:
    struct Constants
    { 
        float time = 0.0f;
    };

    Constants mConstants;

    NS_DECLARE_REFLECTION(WavesBrush, BrushShader)
};

NS_WARNING_POP

}

#endif
