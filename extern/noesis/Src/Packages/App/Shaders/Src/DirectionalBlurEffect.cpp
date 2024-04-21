////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/DirectionalBlurEffect.h>
#include <NsCore/ReflectionImplement.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UIPropertyMetadata.h>

#ifdef HAVE_CUSTOM_SHADERS
#include "DirectionalBlur.bin.h"
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
DirectionalBlurEffect::DirectionalBlurEffect()
{
  #ifdef HAVE_CUSTOM_SHADERS
    RenderContext::EnsureShaders(Shaders, "DirectionalBlur", DirectionalBlur_bin);
  #endif

    SetPixelShader(Shaders.shaders[0]);
    SetConstantBuffer(&mConstants, sizeof(mConstants));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float DirectionalBlurEffect::GetRadius() const
{
    return GetValue<float>(RadiusProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DirectionalBlurEffect::SetRadius(float value)
{
    SetValue<float>(RadiusProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float DirectionalBlurEffect::GetAngle() const
{
    return GetValue<float>(AngleProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DirectionalBlurEffect::SetAngle(float value)
{
    SetValue<float>(AngleProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(DirectionalBlurEffect, "NoesisGUIExtensions.DirectionalBlurEffect")
{
    auto OnRadiusChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        float radius = args.NewValue<float>();

        DirectionalBlurEffect* this_ = (DirectionalBlurEffect*)o;
        this_->mConstants.scale = radius / 17.5f;
        this_->InvalidateConstantBuffer();

        this_->SetPadding(radius, radius, radius, radius);
    };

    auto OnAngleChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        float angle = args.NewValue<float>();

        DirectionalBlurEffect* this_ = (DirectionalBlurEffect*)o;
        this_->mConstants.dirX = cosf(angle * DegToRad);
        this_->mConstants.dirY = sinf(angle * DegToRad);

        this_->InvalidateConstantBuffer();
    };

    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<float>(RadiusProperty, "Radius", UIPropertyMetadata::Create(
        0.0f, PropertyChangedCallback(OnRadiusChanged)));
    data->RegisterProperty<float>(AngleProperty, "Angle", UIPropertyMetadata::Create(
        0.0f, PropertyChangedCallback(OnAngleChanged)));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* DirectionalBlurEffect::RadiusProperty;
const DependencyProperty* DirectionalBlurEffect::AngleProperty;
EffectShaders DirectionalBlurEffect::Shaders;
