////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/SaturationEffect.h>
#include <NsCore/ReflectionImplement.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UIPropertyMetadata.h>

#ifdef HAVE_CUSTOM_SHADERS
#include "Saturation.bin.h"
#endif

using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
SaturationEffect::SaturationEffect()
{
  #ifdef HAVE_CUSTOM_SHADERS
    RenderContext::EnsureShaders(Shaders, "Saturation", Saturation_bin);
  #endif

    SetPixelShader(Shaders.shaders[0]);
    SetConstantBuffer(&mConstants, sizeof(mConstants));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float SaturationEffect::GetSaturation() const
{
    return GetValue<float>(SaturationProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void SaturationEffect::SetSaturation(float value)
{
    SetValue<float>(SaturationProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(SaturationEffect, "NoesisGUIExtensions.SaturationEffect")
{
    auto OnSaturationChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        SaturationEffect* this_ = (SaturationEffect*)o;
        this_->mConstants.saturation = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<float>(SaturationProperty, "Saturation", UIPropertyMetadata::Create(
        1.0f, PropertyChangedCallback(OnSaturationChanged)));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* SaturationEffect::SaturationProperty;
EffectShaders SaturationEffect::Shaders;
