////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/ConicGradientBrush.h>
#include <NsCore/ReflectionImplement.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UIPropertyMetadata.h>
#include <NsGui/GradientStopCollection.h>
#include <NsGui/ContentPropertyMetaData.h>

#ifdef HAVE_CUSTOM_SHADERS
#include "ConicGradient.bin.h"
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
ConicGradientBrush::ConicGradientBrush()
{
  #ifdef HAVE_CUSTOM_SHADERS
    RenderContext::EnsureShaders(Shaders, "ConicGradient", ConicGradient_bin);
  #endif

    mConstants.n = 1;
    mConstants.stops[0].t = 0.0f;
    mConstants.stops[0].color = Color(0.0f, 0.0f, 0.0f, 0.0f);
    SetConstantBuffer(&mConstants, sizeof(uint32_t));

    for (uint32_t i = 0; i < NS_COUNTOF(Shaders.shaders); i++)
    {
        SetPixelShader(Shaders.shaders[i], (BrushShader::Target)i);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GradientStopCollection* ConicGradientBrush::GetGradientStops() const
{
    return GetValue<Ptr<GradientStopCollection>>(GradientStopsProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ConicGradientBrush::SetGradientStops(GradientStopCollection* stops)
{
    SetValue<Ptr<GradientStopCollection>>(GradientStopsProperty, stops);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(ConicGradientBrush, "NoesisGUIExtensions.ConicGradientBrush")
{
    auto OnGradientStopsChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        ConicGradientBrush* this_ = (ConicGradientBrush*)o;

        Ptr<GradientStopCollection> collection = args.NewValue<Ptr<GradientStopCollection>>();
        uint32_t count = collection->Count();

        if (count == 0)
        {
            this_->mConstants.n = 1;
            this_->mConstants.stops[0].t = 0.0f;
            this_->mConstants.stops[0].color = Color(0.0f, 0.0f, 0.0f, 0.0f);
            this_->SetConstantBuffer(&this_->mConstants, sizeof(uint32_t));
        }
        else
        {
            // Create extra stops at 0.0 and 1.0 if missing
            bool needFirst = collection->Get(0)->GetOffset() != 0.0f;
            bool needLast = collection->Get(count - 1)->GetOffset() != 1.0f;

            uint32_t base = needFirst ? 1 : 0;
            uint32_t n = count + base + (needLast ? 1 : 0);
            NS_ASSERT(n < NS_COUNTOF(this_->mConstants.stops));

            this_->mConstants.n = n;

            if (needFirst)
            {
                this_->mConstants.stops[0].t = 0.0f;
                this_->mConstants.stops[0].color = collection->Get(0)->GetColor();
            }

            for (uint32_t i = 0; i < count; i++)
            {
                GradientStop* stop = collection->Get(i);
                this_->mConstants.stops[base + i].t = stop->GetOffset();
                this_->mConstants.stops[base + i].color = stop->GetColor();
            }

            if (needLast)
            {
                this_->mConstants.stops[n - 1].t = 1.0f;
                this_->mConstants.stops[n - 1].color = collection->Get(count - 1)->GetColor();
            }

            this_->SetConstantBuffer(&this_->mConstants, 16 + n * 32);
        }

        this_->InvalidateConstantBuffer();
    };

    NsMeta<ContentPropertyMetaData>("GradientStops");

    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<Ptr<GradientStopCollection>>(GradientStopsProperty, "GradientStops",
        UIPropertyMetadata::Create(Ptr<GradientStopCollection>(),
        PropertyChangedCallback(OnGradientStopsChanged)));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* ConicGradientBrush::GradientStopsProperty;
BrushShaders ConicGradientBrush::Shaders;
