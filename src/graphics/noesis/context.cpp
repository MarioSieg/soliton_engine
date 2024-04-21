// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "context.hpp"

#include <NsGui/IntegrationAPI.h>
#include <NsGui/Uri.h>
#include <NsGui/IView.h>
#include <NsGui/IRenderer.h>
#include <NsGui/FrameworkElement.h>
#include <NsRender/RenderDevice.h>
#include <NsCore/RegisterComponent.h>
#include <NsCore/EnumConverter.h>
#include <NsCore/CompilerSettings.h>

#include "../noesis/Providers/Include/NsApp/LocalXamlProvider.h"
#include "../noesis/Providers/Include/NsApp/LocalFontProvider.h"
#include "../noesis/Providers/Include/NsApp/LocalTextureProvider.h"
#include "../noesis/Theme/Include/NsApp/ThemeProviders.h"
#include "../noesis/VKRenderDevice/Include/NsRender/VKFactory.h"
#include "../noesis/UI/MainMenu.xaml.h"
#include "../noesis/UI/MenuDescription.xaml.h"
#include "../noesis/UI/MultiplierConverter.h"
#include "../noesis/UI/OptionSelector.xaml.h"
#include "../noesis/UI/SettingsMenu.xaml.h"
#include "../noesis/UI/StartMenu.xaml.h"
#include "../noesis/UI/ViewModel.h"
#include "../vulkancore/context.hpp"
#include "../../platform/platform_subsystem.hpp"

using platform::platform_subsystem;

#include "UI/App.xaml.h"
#include "UI/MainWindow.xaml.h"

#define PACKAGE_REGISTER(MODULE, PACKAGE) \
    void NsRegisterReflection##MODULE##PACKAGE(); \
    NsRegisterReflection##MODULE##PACKAGE()

#define PACKAGE_INIT(MODULE, PACKAGE) \
    void NsInitPackage##MODULE##PACKAGE(); \
    NsInitPackage##MODULE##PACKAGE()

#define PACKAGE_SHUTDOWN(MODULE, PACKAGE) \
    void NsShutdownPackage##MODULE##PACKAGE(); \
    NsShutdownPackage##MODULE##PACKAGE()


extern "C" void NsRegisterReflection_NoesisApp() {
    PACKAGE_REGISTER(App, Providers);
    PACKAGE_REGISTER(App, Theme);
    PACKAGE_REGISTER(App, MediaElement);
    PACKAGE_REGISTER(App, Interactivity);
    PACKAGE_REGISTER(Render, VKRenderDevice);
    PACKAGE_REGISTER(App, ApplicationLauncher);
}

extern "C" void NsInitPackages_NoesisApp() {
    PACKAGE_INIT(App, Providers);
    PACKAGE_INIT(App, Theme);
    PACKAGE_INIT(App, MediaElement);
    PACKAGE_INIT(App, Interactivity);
    PACKAGE_INIT(Render, VKRenderDevice);
    PACKAGE_INIT(App, ApplicationLauncher);
}

extern "C" void NsShutdownPackages_NoesisApp() {
    PACKAGE_INIT(Render, VKRenderDevice);
    PACKAGE_SHUTDOWN(App, Interactivity);
    PACKAGE_SHUTDOWN(App, MediaElement);
    PACKAGE_SHUTDOWN(App, Theme);
    PACKAGE_SHUTDOWN(App, Providers);
    PACKAGE_SHUTDOWN(App, ApplicationLauncher);
}

namespace noesis {
    static constinit Noesis::IView* s_event_proxy;

    static auto on_mouse_event([[maybe_unused]] GLFWwindow* const window, const double posX, const double posY) -> void {
        if (s_event_proxy) [[likely]] {
            s_event_proxy->MouseMove(static_cast<int>(posX), static_cast<int>(posY));
        }
    }
    static auto on_mouse_click_event([[maybe_unused]] GLFWwindow* const window, const int button, const int action, [[maybe_unused]] const int mods) -> void {
        if (s_event_proxy) [[likely]] {
            double posX, posY;
            glfwGetCursorPos(window, &posX, &posY);
            Noesis::MouseButton mb {};
            switch (button) {
                case GLFW_MOUSE_BUTTON_LEFT:
                    mb = Noesis::MouseButton_Left;
                    break;
                case GLFW_MOUSE_BUTTON_RIGHT:
                    mb = Noesis::MouseButton_Right;
                    break;
                case GLFW_MOUSE_BUTTON_MIDDLE:
                    mb = Noesis::MouseButton_Middle;
                    break;
                default:
                    return;
            }
            switch (action) {
                case GLFW_PRESS:
                    s_event_proxy->MouseButtonDown(static_cast<int>(posX), static_cast<int>(posY), mb);
                    break;
                case GLFW_RELEASE:
                    s_event_proxy->MouseButtonUp(static_cast<int>(posX), static_cast<int>(posY), mb);
                    break;
            }
        }
    }

    context::context() {
        static constexpr Noesis::MemoryCallbacks k_allocator {
            .user = nullptr,
            .alloc = +[]([[maybe_unused]] void*, const Noesis::SizeT size) noexcept -> void* { return mi_malloc(size); },
            .realloc = +[]([[maybe_unused]] void*, void* const block, const Noesis::SizeT size) noexcept -> void* { return mi_realloc(block, size); },
            .dealloc = +[]([[maybe_unused]] void*, void* const block) noexcept -> void { mi_free(block); },
            .allocSize = +[]([[maybe_unused]] void*, void* const block) noexcept -> Noesis::SizeT { return mi_malloc_size(block); }
        };
        Noesis::GUI::SetMemoryCallbacks(k_allocator);
        Noesis::SetLogHandler([](const char*, uint32_t, uint32_t level, const char*, const char* msg) {
            constexpr static const char* prefixes[] = { "T", "D", "I", "W", "E" };
            log_info("[GUI]: [{}] {}", prefixes[level], msg);
        });
        Noesis::GUI::SetLicense("neo", "Hayg7oXhoO5LHKuI/YPxXOK/Ghu5Mosoic3bbRrVZQmc/ovw");
        Noesis::GUI::Init();

        NsRegisterReflection_NoesisApp();
        NsInitPackages_NoesisApp();

        Noesis::RegisterComponent<Menu3D::App>();
        Noesis::RegisterComponent<Menu3D::MainWindow>();
        Noesis::RegisterComponent<Menu3D::MenuDescription>();
        Noesis::RegisterComponent<Menu3D::MainMenu>();
        Noesis::RegisterComponent<Menu3D::StartMenu>();
        Noesis::RegisterComponent<Menu3D::SettingsMenu>();
        Noesis::RegisterComponent<Menu3D::OptionSelector>();
        Noesis::RegisterComponent<Noesis::EnumConverter<Menu3D::State>>();
        Noesis::RegisterComponent<Menu3D::MultiplierConverter>();

        Noesis::GUI::SetXamlProvider(Noesis::MakePtr<NoesisApp::LocalXamlProvider>("assets/ui/"));
        Noesis::GUI::SetFontProvider(Noesis::MakePtr<NoesisApp::LocalFontProvider>("assets/ui/"));
        Noesis::GUI::SetTextureProvider(Noesis::MakePtr<NoesisApp::LocalTextureProvider>("assets/ui/"));
        const char* fonts[] = { "Fonts/#PT Root UI", "Arial", "Segoe UI Emoji" };
        Noesis::GUI::SetFontFallbacks(fonts, 3);
        Noesis::GUI::SetFontDefaultProperties(15.0f, Noesis::FontWeight_Normal, Noesis::FontStretch_Normal, Noesis::FontStyle_Normal);
        NoesisApp::SetThemeProviders();
        Noesis::GUI::LoadApplicationResources(NoesisApp::Theme::DarkBlue());

        const NoesisApp::VKFactory::InstanceInfo instance_info {
            .instance = vkb::ctx().get_device().get_instance(),
            .physicalDevice = vkb::ctx().get_device().get_physical_device(),
            .device = vkb::ctx().get_device().get_logical_device(),
            .pipelineCache = vkb::ctx().get_pipeline_cache(),
            .queueFamilyIndex = vkb::ctx().get_device().get_graphics_queue_idx(),
            .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
            .stereoSupport = false,
            .QueueSubmit = +[](const VkCommandBuffer cmd) noexcept -> void {
                vkb::ctx().flush_command_buffer<vk::QueueFlagBits::eGraphics, false>(cmd);
            }
        };
        m_device = NoesisApp::VKFactory::CreateDevice(false, instance_info);

        platform_subsystem::s_cursor_pos_callbacks.emplace_back(&on_mouse_event);
        platform_subsystem::s_mouse_button_callbacks.emplace_back(&on_mouse_click_event);
    }

    context::~context() {
        NsShutdownPackages_NoesisApp();
        m_app.Reset();
        m_device.Reset();
    }

    auto context::render(const vk::CommandBuffer cmd) -> void {
        m_app->GetMainWindow()->GetView()->GetRenderer()->UpdateRenderTree();
        const NoesisApp::VKFactory::RecordingInfo recording_info {
            .commandBuffer = cmd,
            .frameNumber = vkb::ctx().get_image_index(),
            .safeFrameNumber = vkb::ctx().get_image_index()
        };
        NoesisApp::VKFactory::SetCommandBuffer(m_device, recording_info);
        NoesisApp::VKFactory::SetRenderPass(m_device, vkb::ctx().get_render_pass(), static_cast<std::uint32_t>(vkb::k_msaa_sample_count));
        m_app->GetMainWindow()->GetView()->GetRenderer()->RenderOffscreen();
        m_app->GetMainWindow()->GetView()->GetRenderer()->Render();
    }

    auto context::load_ui_from_xaml(const std::string& path) -> void {
        m_app.Reset();
        m_app = Noesis::DynamicPtrCast<NoesisApp::Application>(Noesis::GUI::LoadXaml(path.c_str()));
        passert(m_app);
        m_app->Init(m_device, path.c_str(), vkb::ctx().get_width(), vkb::ctx().get_height());
        s_event_proxy = m_app->GetMainWindow()->GetView();
    }

    auto context::tick() -> void {
        m_app->Tick();
    }
}
