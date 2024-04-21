  ////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/MonochromeBrush.h>
#include <NsCore/ReflectionImplement.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UIPropertyMetadata.h>
#include <NsGui/GradientStopCollection.h>
#include <NsGui/ContentPropertyMetaData.h>

#ifdef HAVE_CUSTOM_SHADERS
#include "Monochrome.bin.h"
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
MonochromeBrush::MonochromeBrush()
{
  #ifdef HAVE_CUSTOM_SHADERS
    RenderContext::EnsureShaders(Shaders, "Monochrome", Monochrome_bin);
  #endif

    SetConstantBuffer(&mConstants, sizeof(mConstants));

    for (uint32_t i = 0; i < NS_COUNTOF(Shaders.shaders); i++)
    {
        SetPixelShader(Shaders.shaders[i], (BrushShader::Target)i);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(MonochromeBrush, "NoesisGUIExtensions.MonochromeBrush")
{
    auto OnColorChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        MonochromeBrush* this_ = (MonochromeBrush*)o;
        this_->mConstants.color = args.NewValue<Color>();
        this_->InvalidateConstantBuffer();
    };

    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<Color>(ColorProperty, "Color",
        UIPropertyMetadata::Create(Color::White(), PropertyChangedCallback(OnColorChanged)));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* MonochromeBrush::ColorProperty;
BrushShaders MonochromeBrush::Shaders;
