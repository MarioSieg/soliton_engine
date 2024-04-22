////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/TintEffect.h>
#include <NsCore/ReflectionImplement.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UIPropertyMetadata.h>

#ifdef HAVE_CUSTOM_SHADERS
#include "Tint.bin.h"
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
TintEffect::TintEffect()
{
  #ifdef HAVE_CUSTOM_SHADERS
    RenderContext::EnsureShaders(Shaders, "Tint", Tint_bin);
  #endif

    SetPixelShader(Shaders.shaders[0]);
    SetConstantBuffer(&mConstants, sizeof(mConstants));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const Color& TintEffect::GetColor() const
{
    return GetValue<Color>(ColorProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TintEffect::SetColor(const Color& value)
{
    SetValue<Color>(ColorProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(TintEffect, "NoesisGUIExtensions.TintEffect")
{
    auto OnColorChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        TintEffect* this_ = (TintEffect*)o;
        this_->mConstants.color = args.NewValue<Color>();
        this_->InvalidateConstantBuffer();
    };

    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<Color>(ColorProperty, "Color", UIPropertyMetadata::Create(
        Color::Blue(), PropertyChangedCallback(OnColorChanged)));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* TintEffect::ColorProperty;
EffectShaders TintEffect::Shaders;
