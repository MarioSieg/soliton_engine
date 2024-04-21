////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/VignetteEffect.h>
#include <NsCore/ReflectionImplement.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UIPropertyMetadata.h>

#ifdef HAVE_CUSTOM_SHADERS
#include "Vignette.bin.h"
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
VignetteEffect::VignetteEffect()
{
  #ifdef HAVE_CUSTOM_SHADERS
    RenderContext::EnsureShaders(Shaders, "Vignette", Vignette_bin);
  #endif

    SetPixelShader(Shaders.shaders[0]);
    SetConstantBuffer(&mConstants, sizeof(mConstants));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const Color& VignetteEffect::GetColor() const
{
    return GetValue<Color>(ColorProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VignetteEffect::SetColor(const Color& value)
{
    SetValue<Color>(ColorProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float VignetteEffect::GetStrength() const
{
    return GetValue<float>(StrengthProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VignetteEffect::SetStrength(float value)
{
    SetValue<float>(StrengthProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float VignetteEffect::GetSize() const
{
    return GetValue<float>(SizeProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VignetteEffect::SetSize(float value)
{
    SetValue<float>(SizeProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(VignetteEffect, "NoesisGUIExtensions.VignetteEffect")
{
    auto OnColorChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        VignetteEffect* this_ = (VignetteEffect*)o;
        this_->mConstants.color = args.NewValue<Color>();
        this_->InvalidateConstantBuffer();
    };

    auto OnStrengthChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        VignetteEffect* this_ = (VignetteEffect*)o;
        this_->mConstants.strength = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    auto OnSizeChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        VignetteEffect* this_ = (VignetteEffect*)o;
        this_->mConstants.size = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<Color>(ColorProperty, "Color", UIPropertyMetadata::Create(
        Color::Black(), PropertyChangedCallback(OnColorChanged)));
    data->RegisterProperty<float>(StrengthProperty, "Strength", UIPropertyMetadata::Create(
        15.0f, PropertyChangedCallback(OnStrengthChanged)));
    data->RegisterProperty<float>(SizeProperty, "Size", UIPropertyMetadata::Create(
        0.25f, PropertyChangedCallback(OnSizeChanged)));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* VignetteEffect::ColorProperty;
const DependencyProperty* VignetteEffect::StrengthProperty;
const DependencyProperty* VignetteEffect::SizeProperty;
EffectShaders VignetteEffect::Shaders;
