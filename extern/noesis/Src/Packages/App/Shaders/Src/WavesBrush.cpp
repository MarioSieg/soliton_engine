////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/WavesBrush.h>
#include <NsCore/ReflectionImplement.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UIPropertyMetadata.h>

#ifdef HAVE_CUSTOM_SHADERS
#include "Waves.bin.h"
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
WavesBrush::WavesBrush()
{
  #ifdef HAVE_CUSTOM_SHADERS
    RenderContext::EnsureShaders(Shaders, "Waves", Waves_bin);
  #endif

    SetConstantBuffer(&mConstants, sizeof(mConstants));

    for (uint32_t i = 0; i < NS_COUNTOF(Shaders.shaders); i++)
    {
        SetPixelShader(Shaders.shaders[i], (BrushShader::Target)i);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float WavesBrush::GetTime() const
{
    return GetValue<float>(TimeProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void WavesBrush::SetTime(float value)
{
    SetValue<float>(TimeProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(WavesBrush, "NoesisGUIExtensions.WavesBrush")
{
    auto OnTimeChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        WavesBrush* this_ = (WavesBrush*)o;
        this_->mConstants.time = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<float>(TimeProperty, "Time", UIPropertyMetadata::Create(
        0.0f, PropertyChangedCallback(OnTimeChanged)));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* WavesBrush::TimeProperty;
BrushShaders WavesBrush::Shaders;
