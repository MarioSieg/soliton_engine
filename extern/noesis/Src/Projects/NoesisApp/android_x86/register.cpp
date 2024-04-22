////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/CompilerSettings.h>


#define PACKAGE_REGISTER(MODULE, PACKAGE) \
    void NsRegisterReflection##MODULE##PACKAGE(); \
    NsRegisterReflection##MODULE##PACKAGE()

#define PACKAGE_INIT(MODULE, PACKAGE) \
    void NsInitPackage##MODULE##PACKAGE(); \
    NsInitPackage##MODULE##PACKAGE()

#define PACKAGE_SHUTDOWN(MODULE, PACKAGE) \
    void NsShutdownPackage##MODULE##PACKAGE(); \
    NsShutdownPackage##MODULE##PACKAGE()


////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" void NsRegisterReflection_NoesisApp()
{
    PACKAGE_REGISTER(App, Providers);
    PACKAGE_REGISTER(Render, GLRenderDevice);
    PACKAGE_REGISTER(Render, RenderContext);
    PACKAGE_REGISTER(Render, GLRenderContext);
    PACKAGE_REGISTER(Render, VKRenderDevice);
    PACKAGE_REGISTER(Render, VKRenderContext);
    PACKAGE_REGISTER(App, Display);
    PACKAGE_REGISTER(App, AndroidDisplay);
    PACKAGE_REGISTER(App, Launcher);
    PACKAGE_REGISTER(App, Shaders);
    PACKAGE_REGISTER(App, DisplayLauncher);
    PACKAGE_REGISTER(App, Theme);
    PACKAGE_REGISTER(App, LangServer);
    PACKAGE_REGISTER(App, LangServerHelpers);
    PACKAGE_REGISTER(App, ApplicationLauncher);
    PACKAGE_REGISTER(App, MediaElement);
    PACKAGE_REGISTER(App, Interactivity);
    PACKAGE_REGISTER(App, RiveBase);
    PACKAGE_REGISTER(App, Rive);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" void NsInitPackages_NoesisApp()
{
    PACKAGE_INIT(App, Providers);
    PACKAGE_INIT(Render, GLRenderDevice);
    PACKAGE_INIT(Render, RenderContext);
    PACKAGE_INIT(Render, GLRenderContext);
    PACKAGE_INIT(Render, VKRenderDevice);
    PACKAGE_INIT(Render, VKRenderContext);
    PACKAGE_INIT(App, Display);
    PACKAGE_INIT(App, AndroidDisplay);
    PACKAGE_INIT(App, Launcher);
    PACKAGE_INIT(App, Shaders);
    PACKAGE_INIT(App, DisplayLauncher);
    PACKAGE_INIT(App, Theme);
    PACKAGE_INIT(App, LangServer);
    PACKAGE_INIT(App, LangServerHelpers);
    PACKAGE_INIT(App, ApplicationLauncher);
    PACKAGE_INIT(App, MediaElement);
    PACKAGE_INIT(App, Interactivity);
    PACKAGE_INIT(App, RiveBase);
    PACKAGE_INIT(App, Rive);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" void NsShutdownPackages_NoesisApp()
{
    PACKAGE_SHUTDOWN(App, Rive);
    PACKAGE_SHUTDOWN(App, RiveBase);
    PACKAGE_SHUTDOWN(App, Interactivity);
    PACKAGE_SHUTDOWN(App, MediaElement);
    PACKAGE_SHUTDOWN(App, ApplicationLauncher);
    PACKAGE_SHUTDOWN(App, LangServerHelpers);
    PACKAGE_SHUTDOWN(App, LangServer);
    PACKAGE_SHUTDOWN(App, Theme);
    PACKAGE_SHUTDOWN(App, DisplayLauncher);
    PACKAGE_SHUTDOWN(App, Shaders);
    PACKAGE_SHUTDOWN(App, Launcher);
    PACKAGE_SHUTDOWN(App, AndroidDisplay);
    PACKAGE_SHUTDOWN(App, Display);
    PACKAGE_SHUTDOWN(Render, VKRenderContext);
    PACKAGE_SHUTDOWN(Render, VKRenderDevice);
    PACKAGE_SHUTDOWN(Render, GLRenderContext);
    PACKAGE_SHUTDOWN(Render, RenderContext);
    PACKAGE_SHUTDOWN(Render, GLRenderDevice);
    PACKAGE_SHUTDOWN(App, Providers);
}
