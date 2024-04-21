////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_CROSSFADEBRUSH_H__
#define __APP_CROSSFADEBRUSH_H__


#include <NsApp/ShadersApi.h>
#include <NsGui/BrushShader.h>
#include <NsRender/RenderContext.h>


namespace Noesis { class DependencyProperty; class ImageSource; }

namespace NoesisApp
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251)
NS_CLANG_WARNING_DISABLE("-Wdocumentation")

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Interpolates between two images.
///
/// .. code-block:: xml
///
///  <StackPanel
///    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
///    xmlns:noesis="clr-namespace:NoesisGUIExtensions;assembly=Noesis.GUI.Extensions"
///    Orientation="Horizontal"
///    Height="300">
///    <Image Source="tulip.png"/>
///    <Image Source="pinkrose.png"/>
///    <Rectangle Width="300">
///      <Rectangle.Fill>
///        <ImageBrush ImageSource="tulip.png">
///          <noesis:Brush.Shader>
///            <noesis:CrossFadeBrush Source="pinkrose.png" Weight="0.4" />
///          </noesis:Brush.Shader>
///        </ImageBrush>
///      </Rectangle.Fill>
///    </Rectangle>
///  </StackPanel>
///
/// .. image:: CrossFade.jpg
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_SHADERS_API CrossFadeBrush final: public Noesis::BrushShader
{
public:
    CrossFadeBrush();

    /// Gets or sets how much to weigh the source image color values
    //@{
    float GetWeight() const;
    void SetWeight(float value);
    //@}

    /// Gets or sets the source image
    //@{
    Noesis::ImageSource* GetSource() const;
    void SetSource(Noesis::ImageSource* source);
    //@}

public:
    static const Noesis::DependencyProperty* WeightProperty;
    static const Noesis::DependencyProperty* SourceProperty;
    static BrushShaders Shaders;

private:
    struct Constants
    { 
        float weight = 0.5f;
    };

    Constants mConstants;

    NS_DECLARE_REFLECTION(CrossFadeBrush, BrushShader)
};

NS_WARNING_POP

}

#endif
