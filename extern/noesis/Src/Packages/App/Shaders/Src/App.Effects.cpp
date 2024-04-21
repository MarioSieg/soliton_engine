////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/Package.h>

#include <NsApp/PixelateEffect.h>
#include <NsApp/DirectionalBlurEffect.h>
#include <NsApp/SaturationEffect.h>
#include <NsApp/TintEffect.h>
#include <NsApp/PinchEffect.h>
#include <NsApp/VignetteEffect.h>
#include <NsApp/PlasmaBrush.h>
#include <NsApp/ConicGradientBrush.h>
#include <NsApp/MonochromeBrush.h>
#include <NsApp/WavesBrush.h>
#include <NsApp/CrossFadeBrush.h>


using namespace Noesis;


NS_BEGIN_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_REGISTER_REFLECTION(App, Shaders)
{
    NS_REGISTER_COMPONENT(NoesisApp::PixelateEffect)
    NS_REGISTER_COMPONENT(NoesisApp::DirectionalBlurEffect)
    NS_REGISTER_COMPONENT(NoesisApp::SaturationEffect)
    NS_REGISTER_COMPONENT(NoesisApp::TintEffect)
    NS_REGISTER_COMPONENT(NoesisApp::PinchEffect)
    NS_REGISTER_COMPONENT(NoesisApp::VignetteEffect)
    NS_REGISTER_COMPONENT(NoesisApp::PlasmaBrush)
    NS_REGISTER_COMPONENT(NoesisApp::ConicGradientBrush)
    NS_REGISTER_COMPONENT(NoesisApp::MonochromeBrush)
    NS_REGISTER_COMPONENT(NoesisApp::WavesBrush)
    NS_REGISTER_COMPONENT(NoesisApp::CrossFadeBrush)
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_INIT_PACKAGE(App, Shaders)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_SHUTDOWN_PACKAGE(App, Shaders)
{
}

NS_END_COLD_REGION
