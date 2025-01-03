////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_APPLICATIONLAUNCHER_H__
#define __APP_APPLICATIONLAUNCHER_H__


#include <NsCore/Noesis.h>
#include <NsApp/ApplicationLauncherApi.h>
#include <NsApp/DisplayLauncher.h>
#include <NsCore/Ptr.h>


namespace Noesis
{
class XamlProvider;
class FontProvider;
class TextureProvider;
}

namespace NoesisApp
{

class Application;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// XAML application launcher
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_APPLICATIONLAUNCHER_API ApplicationLauncher: public DisplayLauncher
{
public:
    ApplicationLauncher();
    ApplicationLauncher(const ApplicationLauncher&) = delete;
    ApplicationLauncher& operator=(const ApplicationLauncher&) = delete;
    ~ApplicationLauncher();

    /// Sets App.xaml path used when creating main window
    void SetApplicationFile(const char* filename);

    /// Disables the remote inspector functionality.
    void DisableInspector();

private:
    /// From DisplayLauncher
    //@{
    void OnStartUp() override final;
    void OnTick(double time) override final;
    void OnExit() override final;
    bool GetRunInBackgroundOverride() const override;
    //@}

    virtual Noesis::Ptr<Noesis::XamlProvider> GetXamlProvider() const;
    virtual Noesis::Ptr<Noesis::TextureProvider> GetTextureProvider() const;
    virtual Noesis::Ptr<Noesis::FontProvider> GetFontProvider() const;

private:
    char mAppFile[512];
    bool mLangServerEnabled;
    Noesis::Ptr<Application> mApplication;
};

NS_WARNING_POP

}

#endif
