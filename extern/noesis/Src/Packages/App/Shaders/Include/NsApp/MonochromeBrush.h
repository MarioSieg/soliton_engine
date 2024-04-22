////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_MONOCHROMEBRUSH_H__
#define __APP_MONOCHROMEBRUSH_H__


#include <NsApp/ShadersApi.h>
#include <NsGui/BrushShader.h>
#include <NsDrawing/Color.h>
#include <NsRender/RenderContext.h>


namespace Noesis { class DependencyProperty; }

namespace NoesisApp
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251)
NS_CLANG_WARNING_DISABLE("-Wdocumentation")

////////////////////////////////////////////////////////////////////////////////////////////////////
/// BrushShader that turns an image into a monochrome color
///
/// .. code-block:: xml
///
///  <StackPanel
///    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
///    xmlns:noesis="clr-namespace:NoesisGUIExtensions"
///    Orientation="Horizontal">
///    <Image Source="Images/tulip.png"/>
///    <Rectangle Width="300" Height="300">
///      <Rectangle.Fill>
///        <ImageBrush ImageSource="Images/tulip.png">
///          <noesis:Brush.Shader>
///            <noesis:MonochromeBrush />
///          </noesis:Brush.Shader>
///        </ImageBrush>
///      </Rectangle.Fill>
///    </Rectangle>
///  </StackPanel>
///
/// .. image:: Monochrome.jpg
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_SHADERS_API MonochromeBrush final: public Noesis::BrushShader
{
public:
    MonochromeBrush();

public:
    static const Noesis::DependencyProperty* ColorProperty;
    static BrushShaders Shaders;

private:
    struct Constants
    { 
        Noesis::Color color = Noesis::Color::White();
    };

    Constants mConstants;

    NS_DECLARE_REFLECTION(MonochromeBrush, BrushShader)
};

NS_WARNING_POP

}

#endif
