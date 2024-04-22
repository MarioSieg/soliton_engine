////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_PLASMABRUSH_H__
#define __APP_PLASMABRUSH_H__


#include <NsApp/ShadersApi.h>
#include <NsGui/BrushShader.h>
#include <NsDrawing/Point.h>
#include <NsRender/RenderContext.h>


namespace Noesis { class DependencyProperty; struct Point3D; }

namespace NoesisApp
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251)
NS_CLANG_WARNING_DISABLE("-Wdocumentation")

////////////////////////////////////////////////////////////////////////////////////////////////////
/// BrushShader for old school plasma effect.
///
/// The effect is described at https://www.bidouille.org/prog/plasma.
///
/// The color gradient of the plasma can be controlled by changing the *scale*, *bias*, *frequency*
/// and *phase* properties. These properties specify a factor for each RGB component. More
/// information available at https://iquilezles.org/www/articles/palettes/palettes.htm
///
/// .. code-block:: xml
///
///  <Rectangle
///    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
///    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
///    xmlns:b="http://schemas.microsoft.com/xaml/behaviors"
///    xmlns:noesis="clr-namespace:NoesisGUIExtensions"
///    Width="600" Height="300">
///    <Rectangle.Resources>
///      <Storyboard x:Key="anim">
///        <DoubleAnimation By="864000" Duration="10.0:0:0" Storyboard.TargetName="shader" Storyboard.TargetProperty="Time"/>
///      </Storyboard>
///    </Rectangle.Resources>
///    <b:Interaction.Triggers>
///      <b:EventTrigger EventName="Loaded">
///        <b:ControlStoryboardAction Storyboard="{StaticResource anim}"/>
///      </b:EventTrigger>
///    </b:Interaction.Triggers>
///    <Rectangle.Fill>
///      <ImageBrush>
///        <noesis:Brush.Shader>
///          <noesis:PlasmaBrush x:name="shader" Scale="0.5,0.5,0.0" Bias="0.5,0.5,0.0" Frequency="1.0,1.0,0.0" Phase="0.5,0.0,0.0"/>
///        </noesis:Brush.Shader>
///      </ImageBrush>
///    </Rectangle.Fill>
///  </Rectangle>
///
/// .. image:: Plasma.jpg
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_SHADERS_API PlasmaBrush final: public Noesis::BrushShader
{
public:
    PlasmaBrush();

    /// Gets or sets the time of the effect
    //@{
    float GetTime() const;
    void SetTime(float value);
    //@}

    /// Gets or sets the size of the effect
    //@{
    const Noesis::Point& GetSize() const;
    void SetSize(const Noesis::Point& value);
    //@}

    /// Gets or sets the RGB scale for the color gradient
    //@{
    const Noesis::Point3D& GetScale() const;
    void SetScale(const Noesis::Point3D& value);
    //@}

    /// Gets or sets the RGB bias for the color gradient
    //@{
    const Noesis::Point3D& GetBias() const;
    void SetBias(const Noesis::Point3D& value);
    //@}

    /// Gets or sets the RGB frequency for the color gradient
    //@{
    const Noesis::Point3D& GetFrequency() const;
    void SetFrequency(const Noesis::Point3D& value);
    //@}

    /// Gets or sets the RGB phase for the color gradient
    //@{
    const Noesis::Point3D& GetPhase() const;
    void SetPhase(const Noesis::Point3D& value);
    //@}

public:
    static const Noesis::DependencyProperty* TimeProperty;
    static const Noesis::DependencyProperty* SizeProperty;
    static const Noesis::DependencyProperty* ScaleProperty;
    static const Noesis::DependencyProperty* BiasProperty;
    static const Noesis::DependencyProperty* FrequencyProperty;
    static const Noesis::DependencyProperty* PhaseProperty;
    static BrushShaders Shaders;

private:
    struct Constants
    { 
        float time = 0.0f;
        Noesis::Point3D size = Noesis::Point3D(15.0f, 15.0f, 0.0f);
        Noesis::Point4D scale = Noesis::Point4D(0.5f, 0.5f, 0.5f, 0.0f);
        Noesis::Point4D bias = Noesis::Point4D(0.5f, 0.5f, 0.5f, 0.0f);
        Noesis::Point4D frequency = Noesis::Point4D(1.0f, 1.0f, 1.0f, 0.0f);
        Noesis::Point4D phase = Noesis::Point4D(0.0f, 0.0f, 0.0f, 0.0f);
    };

    Constants mConstants;

    NS_DECLARE_REFLECTION(PlasmaBrush, BrushShader)
};

NS_WARNING_POP

}

#endif
