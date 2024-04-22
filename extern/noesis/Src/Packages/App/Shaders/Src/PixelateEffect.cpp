////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/PixelateEffect.h>
#include <NsCore/ReflectionImplement.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UIPropertyMetadata.h>

#ifdef HAVE_CUSTOM_SHADERS
#include "Pixelate.bin.h"
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
PixelateEffect::PixelateEffect()
{
  #ifdef HAVE_CUSTOM_SHADERS
    RenderContext::EnsureShaders(Shaders, "Pixelate", Pixelate_bin);
  #endif

    SetPixelShader(Shaders.shaders[0]);
    SetConstantBuffer(&mConstants, sizeof(mConstants));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float PixelateEffect::GetSize() const
{
    return GetValue<float>(SizeProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void PixelateEffect::SetSize(float value)
{
    SetValue<float>(SizeProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(PixelateEffect, "NoesisGUIExtensions.PixelateEffect")
{
    auto OnBrickSizeChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        PixelateEffect* this_ = (PixelateEffect*)o;
        this_->mConstants.size = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<float>(SizeProperty, "Size", UIPropertyMetadata::Create(
        5.0f, PropertyChangedCallback(OnBrickSizeChanged)));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* PixelateEffect::SizeProperty;
EffectShaders PixelateEffect::Shaders;
