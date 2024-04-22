////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_THEMEPROVIDERS_H__
#define __APP_THEMEPROVIDERS_H__


#include <NsCore/Noesis.h>
#include <NsApp/ThemeApi.h>


namespace Noesis
{
struct Uri;
}

namespace NoesisApp
{

/// Installs providers for the theme and sets default primary and fallback fonts
/// After calling this function, LoadApplicationResources can be invoked with the given URIs below
NS_APP_THEME_API void SetThemeProviders();

namespace Theme
{
    NS_APP_THEME_API Noesis::Uri DarkRed();
    NS_APP_THEME_API Noesis::Uri LightRed();
    NS_APP_THEME_API Noesis::Uri DarkGreen();
    NS_APP_THEME_API Noesis::Uri LightGreen();
    NS_APP_THEME_API Noesis::Uri DarkBlue();
    NS_APP_THEME_API Noesis::Uri LightBlue();
    NS_APP_THEME_API Noesis::Uri DarkOrange();
    NS_APP_THEME_API Noesis::Uri LightOrange();
    NS_APP_THEME_API Noesis::Uri DarkEmerald();
    NS_APP_THEME_API Noesis::Uri LightEmerald();
    NS_APP_THEME_API Noesis::Uri DarkPurple();
    NS_APP_THEME_API Noesis::Uri LightPurple();
    NS_APP_THEME_API Noesis::Uri DarkCrimson();
    NS_APP_THEME_API Noesis::Uri LightCrimson();
    NS_APP_THEME_API Noesis::Uri DarkLime();
    NS_APP_THEME_API Noesis::Uri LightLime();
    NS_APP_THEME_API Noesis::Uri DarkAqua();
    NS_APP_THEME_API Noesis::Uri LightAqua();
}

}

#endif
