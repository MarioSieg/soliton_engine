////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_ENTRYPOINT_H__
#define __APP_ENTRYPOINT_H__


#include <NsCore/Noesis.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
// Common main entry point for all Noesis applications
////////////////////////////////////////////////////////////////////////////////////////////////////
int NsMain(int argc, char** argv);


////////////////////////////////////////////////////////////////////////////////////////////////////
/// Platform dependent application entry point
////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(NS_PLATFORM_WINDOWS_DESKTOP) || defined(NS_PLATFORM_GAME_CORE)
    #ifdef _CONSOLE
        int main(int argc, char** argv)
        {
            return NsMain(argc, argv);
        }
    #else
        #include <stdlib.h>
        struct HINSTANCE__;
        typedef HINSTANCE__* HINSTANCE;

        extern "C" int __stdcall WinMain(HINSTANCE, HINSTANCE, char*, int)
        {
            return NsMain(__argc, __argv);
        }
    #endif

#elif defined(NS_PLATFORM_IPHONE)
    #import <UIKit/UIKit.h>
    #import <CoreFoundation/CFURL.h>

    static int __argc;
    static char** __argv;

    @interface AppDelegate: NSObject<UIApplicationDelegate>
    @end

    @implementation AppDelegate

    - (void)applicationDidFinishLaunching:(UIApplication *)application
    {
        [self performSelector:@selector(performInit:) withObject:nil afterDelay:0.2f];
    }

    - (void)performInit:(id)object
    {
        NsMain(__argc, __argv);
    }

    @end

    int main(int argc, char** argv)
    {
        __argc = argc;
        __argv = argv;

        // Set working directory to main bundle
        CFBundleRef bundle = CFBundleGetMainBundle();
        if (bundle)
        {
            CFURLRef url = CFBundleCopyBundleURL(bundle);
            if (url)
            {
                char uri[PATH_MAX];
                if (CFURLGetFileSystemRepresentation(url, true, (UInt8*)uri, PATH_MAX))
                {
                    chdir(uri);
                }

                CFRelease(url);
            }
        }

        return UIApplicationMain(argc, argv, nil, @"AppDelegate");
    }

#elif defined(NS_PLATFORM_OSX)
    int main(int argc, char** argv)
    {
        return NsMain(argc, argv);
    }

#elif defined(NS_PLATFORM_ANDROID)
    #include <NsApp/Display.h>
    #include <android_native_app_glue.h>
    #include <android/log.h>
    #include <unistd.h>

    static bool initialized = false;

    void android_main(android_app* app)
    {
        // When Android reuses the same process for another execution, our static variables are
        // already initialized. This is not supported, kill the process and get a fresh one
        if (initialized)
        {
            __android_log_print(ANDROID_LOG_INFO, "Noesis", "=> Killing process(%d) to avoid "
                "reentrancy", getpid());
            exit(0);
        }

        initialized = true;

        // Store current app environment
        NoesisApp::Display::SetPrivateData(app);

        const char* argv[] = { "/system/bin/app_process" };
        NsMain(1, (char**)argv);
    }

#elif defined(NS_PLATFORM_LINUX) || defined(NS_PLATFORM_EMSCRIPTEN)
    int main(int argc, char** argv)
    {
        return NsMain(argc, argv);
    }

#elif defined(NS_PLATFORM_XBOX_ONE)
    #include <NsCore/UTF8.h>
    #include <string.h>

    using namespace Windows::Foundation;
    using namespace Windows::ApplicationModel::Activation;
    using namespace Windows::ApplicationModel::Core;
    using namespace Windows::UI::Core;

    ref class ViewProvider sealed: public IFrameworkView
    {
    public:
        virtual void Initialize(CoreApplicationView^ applicationView)
        {
            applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^,
                IActivatedEventArgs^>(this, &ViewProvider::OnActivated);
        }

        virtual void Uninitialize() {}
        virtual void SetWindow(CoreWindow^ window) {}
        virtual void Load(Platform::String^ entryPoint) {}

        virtual void Run()
        {
            char args[_MAX_PATH];
            Noesis::UTF8::UTF16To8((const uint16_t*)args_->Data(), args, sizeof(args));

            int argc = 1;
            char* argv[16] = { "App.exe" };
            char* context;

            argv[argc++] = strtok_s(args, " ", &context);

            while (argv[argc - 1] != 0 && argc < _countof(argv))
            {
                argv[argc++] = strtok_s(nullptr, " ", &context);
            }

            NsMain(argc - 1, argv);
        }

    private:
        void OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
        {
            if (args->Kind == ActivationKind::Launch)
            {
                LaunchActivatedEventArgs ^launchArgs = (LaunchActivatedEventArgs ^)args;
                args_ = launchArgs->Arguments;
            }

            CoreWindow::GetForCurrentThread()->Activate();
        }

        Platform::String ^args_;
    };

    ref class ViewProviderFactory: IFrameworkViewSource
    {
    public:
        virtual IFrameworkView^ CreateView()
        {
            return ref new ViewProvider();
        }
    };

    [Platform::MTAThread]
    int main(Platform::Array<Platform::String^>^ argv)
    {
        CoreApplication::DisableKinectGpuReservation = true;
        auto viewProviderFactory = ref new ViewProviderFactory();
        CoreApplication::Run(viewProviderFactory);
        return 0;
    }

#elif defined(NS_PLATFORM_SCE)
    #include <stdlib.h>

    // Switch to dynamic allocation
    unsigned int sceLibcHeapExtendedAlloc = 1;

    // No upper limit for heap area
    size_t sceLibcHeapSize = SCE_LIBC_HEAP_SIZE_EXTENDED_ALLOC_NO_LIMIT;

    int main(int argc, char** argv)
    {
        return NsMain(argc, argv);
    }

#elif defined(NS_PLATFORM_NX)
    #include <nn/os/os_Argument.h>

    extern "C" void nnMain()
    {
        NsMain(nn::os::GetHostArgc(), nn::os::GetHostArgv());
    }

#elif defined(NS_PLATFORM_WINRT)
    using namespace Windows::Foundation;
    using namespace Windows::ApplicationModel::Activation;
    using namespace Windows::ApplicationModel::Core;
    using namespace Windows::UI::Core;

    ref class ViewProvider sealed: public IFrameworkView
    {
    public:
        virtual void Initialize(CoreApplicationView^ applicationView)
        {
            applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^,
                IActivatedEventArgs^>(this, &ViewProvider::OnActivated);
        }

        virtual void Uninitialize() {}
        virtual void SetWindow(CoreWindow^ window) {}
        virtual void Load(Platform::String^ entryPoint) {}

        virtual void Run()
        {
            char* argv[] = { "App.exe" };
            NsMain(1, argv);
        }

    private:
        void OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
        {
            CoreWindow::GetForCurrentThread()->Activate();
        }
    };

    ref class ViewProviderFactory: IFrameworkViewSource
    {
    public:
        virtual IFrameworkView^ CreateView()
        {
            return ref new ViewProvider();
        }
    };

    [Platform::MTAThread]
    int main(Platform::Array<Platform::String^>^ argv)
    {
        IFrameworkViewSource^ viewProviderFactory = ref new ViewProviderFactory();
        CoreApplication::Run(viewProviderFactory);
        return 0;
    }

#else
    #error Unknown platform
#endif


#endif
