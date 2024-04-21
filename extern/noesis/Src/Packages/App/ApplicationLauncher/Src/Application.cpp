////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/Application.h>
#include <NsApp/Window.h>
#include <NsApp/Display.h>
#include <NsApp/DisplayLauncher.h>
#include <NsGui/DependencyProperty.h>
#include <NsGui/ResourceDictionary.h>
#include <NsGui/IntegrationAPI.h>
#include <NsGui/IView.h>
#include <NsCore/ReflectionImplement.h>
#include <NsRender/RenderContext.h>
#include <NsRender/RenderDevice.h>

#ifdef NS_PLATFORM_WINDOWS_DESKTOP
#include "renderdoc_app.h"
#include <windows.h>
#define RENDER_DOC_ENABLED
#endif

#if defined(NS_CREATE_CRASH_DUMPS) && defined(NS_PLATFORM_WINDOWS_DESKTOP)
#include <windows.h>

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4091)
#include <Dbghelp.h>
NS_WARNING_POP

#include <Shlobj.h>
#pragma comment(lib, "Dbghelp.lib")
#endif


using namespace Noesis;
using namespace NoesisApp;


static Application* gInstance;


////////////////////////////////////////////////////////////////////////////////////////////////////
Application::Application()
{
    // More than one instantation is possible, for example when dropping App.xaml into XamlPlayer
    if (gInstance == 0)
    {
        gInstance = this;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Application::~Application()
{
    mExit(this, EventArgs::Empty);

    if (mMainWindow != 0)
    {
        // Make sure the View is destroyed after the Window element tree
        Ptr<IView> view(mMainWindow->GetView());

        mMainWindow->Shutdown();
        mMainWindow.Reset();
    }

    if (mRenderContext != 0)
    {
        mRenderContext->Shutdown();
        mRenderContext.Reset();
    }

    if (mResources != 0)
    {
        mResources->RemoveDictionaryChangedListeners();
        mResources.Reset();
    }

    if (gInstance == this)
    {
        gInstance = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Application* Application::Current()
{
    NS_ASSERT(gInstance != 0);
    return gInstance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const Uri& Application::GetStartupUri() const
{
    return mStartupUri;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::SetStartupUri(const Uri& startupUri)
{
    mStartupUri = startupUri;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ResourceDictionary* Application::GetResources() const
{
    EnsureResources();
    return mResources;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::SetResources(ResourceDictionary* resources)
{
    if (mResources != resources)
    {
        mResources.Reset(resources);
        GUI::SetApplicationResources(mResources);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Window* Application::GetMainWindow() const
{
    return mMainWindow;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const CommandLine& Application::GetArguments() const
{
    return mArguments;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::Init(Display* display, const CommandLine& arguments)
{
    mArguments = arguments;

  #ifdef RENDER_DOC_ENABLED
    display->KeyDown() += [&](Display* display, Key key)
    {
        if (key == Key_F10)
        {
            static RENDERDOC_API_1_1_2* RenderDoc;

            if (RenderDoc == 0)
            {
                HMODULE handle = LoadLibraryA("renderdoc.dll");
                if (!handle) handle = LoadLibraryA("C:/Program Files/RenderDoc/renderdoc.dll");

                if (handle != 0)
                {
                    // Destroy view and context
                    mMainWindow->ResetView();
                    mRenderContext->Shutdown();

                    // Load renderdoc API
                    auto RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(handle, "RENDERDOC_GetAPI");
                    if (RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&RenderDoc) == 1)
                    {
                        RenderDoc->SetFocusToggleKeys(NULL, 0);
                        RenderDoc->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
                    }

                    // Recreate context and view
                    CreateRenderContext(display);
                    mMainWindow->RecreateView(mRenderContext, GetEmulateTouchOverride());
                }
            }

            if (RenderDoc != 0)
            {
                RenderDoc->TriggerCapture();

                if (RenderDoc->IsTargetControlConnected())
                {
                    RenderDoc->ShowReplayUI();
                }
                else
                {
                    RenderDoc->LaunchReplayUI(1, NULL);
                }
            }
        }
    };
  #endif

    // Render context creation
    CreateRenderContext(display);

    // Application resources
    GUI::SetApplicationResources(GetResources());

    // Redirect integration callbacks to display
    GUI::SetSoftwareKeyboardCallback(display, [](void* user, UIElement* focused, bool open)
    {
        if (open)
        {
            ((Display*)user)->OpenSoftwareKeyboard(focused);
        }
        else
        {
            ((Display*)user)->CloseSoftwareKeyboard();
        }
    });

    GUI::SetCursorCallback(display, [](void* user, IView*, Cursor* cursor)
    {
        ((Display*)user)->SetCursor(cursor);
    });

    GUI::SetOpenUrlCallback(display, [](void* user, const char* url)
    {
        ((Display*)user)->OpenUrl(url);
    });

    GUI::SetPlayAudioCallback(display, [](void* user, const Uri& uri, float volume)
    {
        ((Display*)user)->PlayAudio(uri, volume);
    });

    display->Activated() += [this](Display*)
    {
        mActivated(this, EventArgs::Empty);
    };

    display->Deactivated() += [this](Display*)
    {
        mDeactivated(this, EventArgs::Empty);
    };

    display->NativeHandleChanged() += [this](Display*, void* window)
    {
        mRenderContext->SetWindow(window);
    };

    display->SaveState() += [this](Display*)
    {
        mRenderContext->SaveState();
    };

    // Load startup URI as main window
    if (!StrIsNullOrEmpty(mStartupUri.Str()))
    {
        Ptr<BaseComponent> root = GUI::LoadXaml(mStartupUri.Str());
        mMainWindow = DynamicPtrCast<Window>(root);

        // Non window roots are allowed
        if (mMainWindow == 0)
        {
            mMainWindow = *new Window();
            mMainWindow->DependencyObject::Init();
            mMainWindow->SetContent(root);
            mMainWindow->SetTitle(GetTitleOverride(DynamicPtrCast<UIElement>(root)));
        }
    }
    else
    {
        NS_FATAL("Startup window not defined");
    }

    // Initialize main window
    uint32_t samples = GetSamplesOverride();
    bool ppaa = GetPPAAOverride();
    bool lcd = GetLCDOverride();
    bool emulateTouch = GetEmulateTouchOverride();

    mMainWindow->Init(display, mRenderContext, Max(samples, 1U), ppaa, lcd, emulateTouch);

  #if defined(NS_CREATE_CRASH_DUMPS) && defined(NS_PLATFORM_WINDOWS_DESKTOP)
    auto handler = [](PEXCEPTION_POINTERS info) -> LONG
    {
        SYSTEMTIME t;
        GetLocalTime(&t);

        WCHAR dump[MAX_PATH];
        wsprintfW(dump, L"%4d%02d%02d_%02d%02d%02d.dmp",
            t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

        WCHAR filename[MAX_PATH];
        if (FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, filename)))
        {
            return EXCEPTION_EXECUTE_HANDLER;
        }

        wcscat_s(filename, L"\\Noesis");
        CreateDirectoryW(filename, nullptr);

        wcscat_s(filename, L"\\Dumps");
        CreateDirectoryW(filename, nullptr);

        wcscat_s(filename, L"\\");
        wcscat_s(filename, dump);

        HANDLE file = CreateFileW(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        if (file == 0)
        {
            return EXCEPTION_EXECUTE_HANDLER;
        }

        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = info;
        mdei.ClientPointers = true;

        if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal,
            &mdei, nullptr, nullptr))
        {
            WCHAR msg[2048] = L"";
            wcscat_s(msg, L"A crash dump has been created at\n\n");
            wcscat_s(msg, L"%LOCALAPPDATA%\\Noesis\\Dumps\\");
            wcscat_s(msg, dump);
            wcscat_s(msg, L"\n\nPlease report this issue.");

            MessageBoxW(0, msg, L"Noesis Crash!", MB_ICONERROR);
        }

        return EXCEPTION_EXECUTE_HANDLER;
    };

    // Call this as late as possible because LoadLibrary override this
    SetUnhandledExceptionFilter(handler);
  #endif

    // Raise StartUp event
    mStartUp(this, EventArgs::Empty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::Tick(double time)
{
    mMainWindow->Render(time);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::Shutdown(int exitCode)
{
    ((DisplayLauncher*)Launcher::Current())->Quit(exitCode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
EventHandler& Application::Activated()
{
    return mActivated;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
EventHandler& Application::Deactivated()
{
    return mDeactivated;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
EventHandler& Application::StartUp()
{
    return mStartUp;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
EventHandler& Application::Exit()
{
    return mExit;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IUITreeNode* Application::GetNodeParent() const
{
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::SetNodeParent(IUITreeNode*)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
BaseComponent* Application::FindNodeResource(const char* key, bool) const
{
    if (mResources != 0)
    {
        BaseComponent* resource = mResources->Get(key);

        if (resource != 0)
        {
            return resource;
        }
    }

    return DependencyProperty::GetUnsetValue();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ObjectWithNameScope Application::FindNodeName(const char*) const
{
    return ObjectWithNameScope { };
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* Application::GetTitleOverride(UIElement* root) const
{
    Window* window = DynamicCast<Window*>(root);
    return window ? window->GetTitle() : "";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderContext> Application::GetRenderContextOverride() const
{
    if (mArguments.HasOption("render"))
    {
        return RenderContext::Create(mArguments.FindOption("render", ""));
    }
    else
    {
        return RenderContext::Create();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Application::GetPPAAOverride() const
{
    return GetSamplesOverride() == 1 && atoi(mArguments.FindOption("ppaa", "1")) != 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Application::GetLCDOverride() const
{
    return atoi(mArguments.FindOption("lcd", "1")) != 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t Application::GetSamplesOverride() const
{
    return atoi(mArguments.FindOption("samples", "1"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t Application::GetOffscreenSamplesOverride() const
{
    return GetSamplesOverride();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Application::GetVSyncOverride() const
{
    return atoi(mArguments.FindOption("vsync", "1")) != 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Application::GetsRGBOverride() const
{
    return mArguments.HasOption("linear");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Application::GetEmulateTouchOverride() const
{
    return mArguments.HasOption("emulate_touch");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::EnsureResources() const
{
    if (mResources == 0)
    {
        mResources = *new ResourceDictionary();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::CreateRenderContext(Display* display)
{
    mRenderContext = GetRenderContextOverride();

    uint32_t samples = GetSamplesOverride();
    uint32_t offscreenSamples = Min(GetOffscreenSamplesOverride(), samples);
    bool vSync = GetVSyncOverride();
    bool sRGB = GetsRGBOverride();

    mRenderContext->Init(display->GetNativeHandle(), samples, vSync, sRGB);
    mRenderContext->GetDevice()->SetOffscreenSampleCount(offscreenSamples);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(Application)
{
    NsImpl<IUITreeNode>();

    NsProp("Resources", &Application::GetResources, &Application::SetResources);
    NsProp("MainWindow", &Application::GetMainWindow);
    NsProp("StartupUri", &Application::mStartupUri);
    NsEvent("Activated", &Application::mActivated);
    NsEvent("Deactivated", &Application::mDeactivated);
    NsEvent("StartUp", &Application::mStartUp);
    NsEvent("Exit", &Application::mExit);
}

NS_END_COLD_REGION
