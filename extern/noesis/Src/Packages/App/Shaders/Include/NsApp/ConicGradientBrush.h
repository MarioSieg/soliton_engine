////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_CONICGRADIENTBRUSH_H__
#define __APP_CONICGRADIENTBRUSH_H__


#include <NsApp/ShadersApi.h>
#include <NsGui/BrushShader.h>
#include <NsDrawing/Color.h>
#include <NsDrawing/Point.h>
#include <NsRender/RenderContext.h>


namespace Noesis { class DependencyProperty; class GradientStopCollection; }

namespace NoesisApp
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251)
NS_CLANG_WARNING_DISABLE("-Wdocumentation")

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Creates an pattern consisting of a gradient with color transitions rotated around a center point
///
/// Conic gradients are similar to radial gradients, except that the color stops are on the outer
/// edge of the circle that gets created.
///
/// .. code-block:: xml
///
///  <StackPanel
///    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
///    xmlns:noesis="clr-namespace:NoesisGUIExtensions"
///    Orientation="Horizontal">
///    <Rectangle Width="600" Height="300">
///      <Rectangle.Fill>
///        <ImageBrush>
///          <noesis:Brush.Shader>
///            <noesis:ConicGradientBrush>
///              <GradientStop Color="Red" Offset="0.0" />
///              <GradientStop Color="Orange" Offset="0.25" />
///              <GradientStop Color="Yellow" Offset="0.50" />
///              <GradientStop Color="Green" Offset="0.75" />
///              <GradientStop Color="Blue" Offset="1.0" />
///            </noesis:ConicGradientBrush>
///          </noesis:Brush.Shader>
///        </ImageBrush>
///      </Rectangle.Fill>
///    </Rectangle>
///  </StackPanel>
///
/// .. image:: Conic.jpg
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_SHADERS_API ConicGradientBrush final: public Noesis::BrushShader
{
public:
    ConicGradientBrush();

    /// Gets or sets the brush's gradient stops
    //@{
    Noesis::GradientStopCollection* GetGradientStops() const;
    void SetGradientStops(Noesis::GradientStopCollection* stops);
    //@}

public:
    static const Noesis::DependencyProperty* GradientStopsProperty;
    static BrushShaders Shaders;

private:
    struct Stop
    {
        float t, padding[3];
        Noesis::Color color;
    };

    struct Constants
    {
        int32_t n, padding[3];
        Stop stops[8];
    };

    Constants mConstants;

    NS_DECLARE_REFLECTION(ConicGradientBrush, BrushShader)
};

NS_WARNING_POP

}

#endif
