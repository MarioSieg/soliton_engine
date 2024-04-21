////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/ApplicationLauncher.h>
#include <NsApp/Application.h>
#include <NsApp/LocalXamlProvider.h>
#include <NsApp/LocalTextureProvider.h>
#include <NsApp/LocalFontProvider.h>
#include <NsApp/EmbeddedXamlProvider.h>
#include <NsApp/EmbeddedFontProvider.h>
#include <NsApp/PreviewRenderer.h>
#include <NsApp/ThemeProviders.h>
#include <NsApp/Window.h>
#include <NsApp/LangServer.h>
#include <NsRender/RenderContext.h>
#include <NsGui/IntegrationAPI.h>
#include <NsCore/StringUtils.h>


using namespace Noesis;
using namespace NoesisApp;


#ifdef NS_PROFILE
// Dripicons V2 by Amit Jakhu
// https://github.com/amitjakhu/dripicons
#include "dripicons-v2.ttf.bin.h"
#include "ModeNine.ttf.bin.h"
#include "StatsOverlay.xaml.bin.h"
#endif

#include "MessageBox.xaml.bin.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
ApplicationLauncher::ApplicationLauncher()
{
    StrCopy(mAppFile, sizeof(mAppFile), "");

    EmbeddedXaml xamls[] =
    {
        #ifdef NS_PROFILE
         { "StatsOverlay.xaml", StatsOverlay_xaml },
        #endif

         { "MessageBox.xaml", MessageBox_xaml }
    };

    GUI::SetAssemblyXamlProvider("App", MakePtr<EmbeddedXamlProvider>(xamls));

  #ifdef NS_PROFILE
    EmbeddedFont fonts[] =
    {
        { "", dripicons_v2_ttf },
        { "", ModeNine_ttf }
    };

    GUI::SetAssemblyFontProvider("App", MakePtr<EmbeddedFontProvider>(fonts));
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ApplicationLauncher::~ApplicationLauncher()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ApplicationLauncher::SetApplicationFile(const char* filename)
{
    StrCopy(mAppFile, sizeof(mAppFile), filename);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ApplicationLauncher::DisableInspector()
{
    GUI::DisableInspector();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ApplicationLauncher::OnStartUp()
{
    const char* root = GetArguments().FindOption("root", nullptr);
    mLangServerEnabled = GetArguments().HasOption("lang_server");

    if (!StrIsNullOrEmpty(root))
    {
        NS_LOG_INFO("Root filesystem: '%s'", root);
        GUI::SetXamlProvider(MakePtr<LocalXamlProvider>(root));
        GUI::SetTextureProvider(MakePtr<LocalTextureProvider>(root));
        GUI::SetFontProvider(MakePtr<LocalFontProvider>(root));
    }
    else
    {
        GUI::SetXamlProvider(GetXamlProvider());
        GUI::SetTextureProvider(GetTextureProvider());
        GUI::SetFontProvider(GetFontProvider());
    }

    NoesisApp::SetThemeProviders();

    // Load application file
    if (StrIsNullOrEmpty(mAppFile))
    {
        NS_FATAL("Application file not defined");
    }

    mApplication = DynamicPtrCast<Application>(GUI::LoadXaml(mAppFile));
    if (mApplication == 0)
    {
        NS_FATAL("File '%s' does not define a valid application file.", mAppFile);
    }

    mApplication->Init(GetDisplay(), GetArguments());

  #ifdef NS_LANG_SERVER_ENABLED
    if (mLangServerEnabled)
    {
        LangServer::SetName(mApplication->GetClassType()->GetName());

        LangServer::SetXamlProvider(MakePtr<LocalXamlProvider>());
        LangServer::SetTextureProvider(MakePtr<LocalTextureProvider>());
        LangServer::SetFontProvider(MakePtr<LocalFontProvider>());

        LangServer::SetRenderCallback(0, [](void*, UIElement* content, int renderWidth,
            int renderHeight, double renderTime, const char* savePath)
        {
            RenderContext* context = RenderContext::Current();
            RenderPreview(context, content, renderWidth, renderHeight, renderTime, savePath);
        });

        LangServer::Init();
    }
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ApplicationLauncher::OnTick(double time)
{
  #ifdef NS_LANG_SERVER_ENABLED
    if (mLangServerEnabled)
    {
        LangServer::RunTick();
    }
  #endif

    mApplication->Tick(time);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ApplicationLauncher::OnExit()
{
  #ifdef NS_LANG_SERVER_ENABLED
    if (mLangServerEnabled)
    {
        LangServer::Shutdown();
    }
  #endif

    mApplication.Reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ApplicationLauncher::GetRunInBackgroundOverride() const
{
    return GetArguments().HasOption("lang_server") || GetArguments().HasOption("runInBackground");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<XamlProvider> ApplicationLauncher::GetXamlProvider() const
{
    return *new LocalXamlProvider("");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<TextureProvider> ApplicationLauncher::GetTextureProvider() const
{
    return *new LocalTextureProvider("");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<FontProvider> ApplicationLauncher::GetFontProvider() const
{
    return *new LocalFontProvider("");
}
