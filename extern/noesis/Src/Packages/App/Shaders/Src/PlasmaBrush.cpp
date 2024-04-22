////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/PlasmaBrush.h>
#include <NsCore/ReflectionImplement.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UIPropertyMetadata.h>

#ifdef HAVE_CUSTOM_SHADERS
#include "Plasma.bin.h"
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
PlasmaBrush::PlasmaBrush()
{
  #ifdef HAVE_CUSTOM_SHADERS
    RenderContext::EnsureShaders(Shaders, "Plasma", Plasma_bin);
  #endif

    SetConstantBuffer(&mConstants, sizeof(mConstants));

    for (uint32_t i = 0; i < NS_COUNTOF(Shaders.shaders); i++)
    {
        SetPixelShader(Shaders.shaders[i], (BrushShader::Target)i);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float PlasmaBrush::GetTime() const
{
    return GetValue<float>(TimeProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void PlasmaBrush::SetTime(float value)
{
    SetValue<float>(TimeProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const Point& PlasmaBrush::GetSize() const
{
    return GetValue<Point>(SizeProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void PlasmaBrush::SetSize(const Point& value)
{
    SetValue<Point>(SizeProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const Point3D& PlasmaBrush::GetScale() const
{
    return GetValue<Point3D>(ScaleProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void PlasmaBrush::SetScale(const Point3D& value)
{
    SetValue<Point3D>(ScaleProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const Point3D& PlasmaBrush::GetBias() const
{
    return GetValue<Point3D>(BiasProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void PlasmaBrush::SetBias(const Point3D& value)
{
    SetValue<Point3D>(BiasProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const Point3D& PlasmaBrush::GetFrequency() const
{
    return GetValue<Point3D>(FrequencyProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void PlasmaBrush::SetFrequency(const Point3D& value)
{
    SetValue<Point3D>(FrequencyProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const Point3D& PlasmaBrush::GetPhase() const
{
    return GetValue<Point3D>(PhaseProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void PlasmaBrush::SetPhase(const Point3D& value)
{
    SetValue<Point3D>(PhaseProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(PlasmaBrush, "NoesisGUIExtensions.PlasmaBrush")
{
    auto OnTimeChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        PlasmaBrush* this_ = (PlasmaBrush*)o;
        this_->mConstants.time = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    auto OnSizeChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        PlasmaBrush* this_ = (PlasmaBrush*)o;
        this_->mConstants.size = args.NewValue<Point>();
        this_->InvalidateConstantBuffer();
    };

    auto OnScaleChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        PlasmaBrush* this_ = (PlasmaBrush*)o;
        this_->mConstants.scale = args.NewValue<Point3D>();
        this_->InvalidateConstantBuffer();
    };

    auto OnBiasChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        PlasmaBrush* this_ = (PlasmaBrush*)o;
        this_->mConstants.bias = args.NewValue<Point3D>();
        this_->InvalidateConstantBuffer();
    };

    auto OnFrequencyChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        PlasmaBrush* this_ = (PlasmaBrush*)o;
        this_->mConstants.frequency = args.NewValue<Point3D>();
        this_->InvalidateConstantBuffer();
    };

    auto OnPhaseChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        PlasmaBrush* this_ = (PlasmaBrush*)o;
        this_->mConstants.phase = args.NewValue<Point3D>();
        this_->InvalidateConstantBuffer();
    };

    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<float>(TimeProperty, "Time", UIPropertyMetadata::Create(
        0.0f, PropertyChangedCallback(OnTimeChanged)));
    data->RegisterProperty<Point>(SizeProperty, "Size", UIPropertyMetadata::Create(
        Point(15.0f, 15.0f), PropertyChangedCallback(OnSizeChanged)));
    data->RegisterProperty<Point3D>(ScaleProperty, "Scale", UIPropertyMetadata::Create(
        Point3D(0.5f, 0.5f, 0.5f), PropertyChangedCallback(OnScaleChanged)));
    data->RegisterProperty<Point3D>(BiasProperty, "Bias", UIPropertyMetadata::Create(
        Point3D(0.5f, 0.5f, 0.5f), PropertyChangedCallback(OnBiasChanged)));
    data->RegisterProperty<Point3D>(FrequencyProperty, "Frequency", UIPropertyMetadata::Create(
        Point3D(1.0f, 1.0f, 1.0f), PropertyChangedCallback(OnFrequencyChanged)));
    data->RegisterProperty<Point3D>(PhaseProperty, "Phase", UIPropertyMetadata::Create(
        Point3D(0.0f, 0.0f, 0.0f), PropertyChangedCallback(OnPhaseChanged)));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* PlasmaBrush::TimeProperty;
const DependencyProperty* PlasmaBrush::SizeProperty;
const DependencyProperty* PlasmaBrush::ScaleProperty;
const DependencyProperty* PlasmaBrush::BiasProperty;
const DependencyProperty* PlasmaBrush::FrequencyProperty;
const DependencyProperty* PlasmaBrush::PhaseProperty;
BrushShaders PlasmaBrush::Shaders;
