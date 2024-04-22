////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/PinchEffect.h>
#include <NsCore/ReflectionImplement.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UIPropertyMetadata.h>

#ifdef HAVE_CUSTOM_SHADERS
#include "Pinch.bin.h"
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
PinchEffect::PinchEffect()
{
  #ifdef HAVE_CUSTOM_SHADERS
    RenderContext::EnsureShaders(Shaders, "Pinch", Pinch_bin);
  #endif

    SetPixelShader(Shaders.shaders[0]);
    SetConstantBuffer(&mConstants, sizeof(mConstants));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const Point& PinchEffect::GetCenter() const
{
    return GetValue<Point>(CenterProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void PinchEffect::SetCenter(const Point& value)
{
    SetValue<Point>(CenterProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float PinchEffect::GetRadius() const
{
    return GetValue<float>(RadiusProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void PinchEffect::SetRadius(float value)
{
    SetValue<float>(RadiusProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float PinchEffect::GetAmount() const
{
    return GetValue<float>(AmountProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void PinchEffect::SetAmount(float value)
{
    SetValue<float>(AmountProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(PinchEffect, "NoesisGUIExtensions.PinchEffect")
{
    auto OnCenterChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        PinchEffect* this_ = (PinchEffect*)o;
        this_->mConstants.center = args.NewValue<Point>();
        this_->InvalidateConstantBuffer();
    };

    auto OnRadiusChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        PinchEffect* this_ = (PinchEffect*)o;
        this_->mConstants.radius = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    auto OnAmountChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        PinchEffect* this_ = (PinchEffect*)o;
        this_->mConstants.amount = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<Point>(CenterProperty, "Center", UIPropertyMetadata::Create(
        Point(0.0f, 0.0f), PropertyChangedCallback(OnCenterChanged)));
    data->RegisterProperty<float>(RadiusProperty, "Radius", UIPropertyMetadata::Create(
        0.0f, PropertyChangedCallback(OnRadiusChanged)));
    data->RegisterProperty<float>(AmountProperty, "Amount", UIPropertyMetadata::Create(
        0.0f, PropertyChangedCallback(OnAmountChanged)));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* PinchEffect::CenterProperty;
const DependencyProperty* PinchEffect::RadiusProperty;
const DependencyProperty* PinchEffect::AmountProperty;
EffectShaders PinchEffect::Shaders;
