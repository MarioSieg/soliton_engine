////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/CrossFadeBrush.h>
#include <NsCore/ReflectionImplement.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UIPropertyMetadata.h>
#include <NsGui/ImageSource.h>

#ifdef HAVE_CUSTOM_SHADERS
#include "CrossFade.bin.h"
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
CrossFadeBrush::CrossFadeBrush()
{
  #ifdef HAVE_CUSTOM_SHADERS
    RenderContext::EnsureShaders(Shaders, "CrossFade", CrossFade_bin);
  #endif

    SetConstantBuffer(&mConstants, sizeof(mConstants));

    for (uint32_t i = 0; i < NS_COUNTOF(Shaders.shaders); i++)
    {
        SetPixelShader(Shaders.shaders[i], (BrushShader::Target)i);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float CrossFadeBrush::GetWeight() const
{
    return GetValue<float>(WeightProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CrossFadeBrush::SetWeight(float value)
{
    SetValue<float>(WeightProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ImageSource* CrossFadeBrush::GetSource() const
{
    return GetValue<Ptr<ImageSource>>(SourceProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CrossFadeBrush::SetSource(ImageSource* source)
{
    SetValue<Ptr<ImageSource>>(SourceProperty, source);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(CrossFadeBrush, "NoesisGUIExtensions.CrossFadeBrush")
{
    auto OnWeightChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        CrossFadeBrush* this_ = (CrossFadeBrush*)o;
        this_->mConstants.weight = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    auto OnSourceChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        CrossFadeBrush* this_ = (CrossFadeBrush*)o;
        this_->SetTexture(args.NewValue<Ptr<ImageSource>>(), 0);
    };

    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<float>(WeightProperty, "Weight", UIPropertyMetadata::Create(
        0.5f, PropertyChangedCallback(OnWeightChanged)));
    data->RegisterProperty<Ptr<ImageSource>>(SourceProperty, "Source", UIPropertyMetadata::Create(
        Ptr<ImageSource>(), PropertyChangedCallback(OnSourceChanged)));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* CrossFadeBrush::WeightProperty;
const DependencyProperty* CrossFadeBrush::SourceProperty;
BrushShaders CrossFadeBrush::Shaders;
