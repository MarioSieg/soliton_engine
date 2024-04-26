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
#include "../imgui/imgui.h"
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

    [[nodiscard]] static auto is_ui_active(GLFWwindow* const window) noexcept -> bool {
        return glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE
            && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
    }

    static auto on_mouse_event(GLFWwindow* const window, const double posX, const double posY) -> void {
        if (s_event_proxy && is_ui_active(window)) {
            s_event_proxy->MouseMove(static_cast<int>(posX), static_cast<int>(posY));
        }
    }
    static auto on_mouse_click_event(GLFWwindow* const window, const int button, const int action, [[maybe_unused]] const int mods) -> void {
        if (s_event_proxy && is_ui_active(window)) {
            double posX, posY;
            glfwGetCursorPos(window, &posX, &posY);
            Noesis::MouseButton mb;
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
    static auto on_key_event(GLFWwindow* const window, const int key, [[maybe_unused]] const int scancode, const int action, const int mods) -> void {
        if (s_event_proxy && is_ui_active(window)) {
            const Noesis::Key ns_key = [key]() noexcept -> Noesis::Key {
                using enum Noesis::Key;
                switch (key) {
                    case GLFW_KEY_SPACE:              return Key_Space;
                    case GLFW_KEY_A:                  return Key_A;
                    case GLFW_KEY_B:                  return Key_B;
                    case GLFW_KEY_C:                  return Key_C;
                    case GLFW_KEY_D:                  return Key_D;
                    case GLFW_KEY_E:                  return Key_E;
                    case GLFW_KEY_F:                  return Key_F;
                    case GLFW_KEY_G:                  return Key_G;
                    case GLFW_KEY_H:                  return Key_H;
                    case GLFW_KEY_I:                  return Key_I;
                    case GLFW_KEY_J:                  return Key_J;
                    case GLFW_KEY_K:                  return Key_K;
                    case GLFW_KEY_L:                  return Key_L;
                    case GLFW_KEY_M:                  return Key_M;
                    case GLFW_KEY_N:                  return Key_N;
                    case GLFW_KEY_O:                  return Key_O;
                    case GLFW_KEY_P:                  return Key_P;
                    case GLFW_KEY_Q:                  return Key_Q;
                    case GLFW_KEY_R:                  return Key_R;
                    case GLFW_KEY_S:                  return Key_S;
                    case GLFW_KEY_T:                  return Key_T;
                    case GLFW_KEY_U:                  return Key_U;
                    case GLFW_KEY_V:                  return Key_V;
                    case GLFW_KEY_W:                  return Key_W;
                    case GLFW_KEY_X:                  return Key_X;
                    case GLFW_KEY_Y:                  return Key_Y;
                    case GLFW_KEY_Z:                  return Key_Z;
                    case GLFW_KEY_0:                  return Key_D0;
                    case GLFW_KEY_1:                  return Key_D1;
                    case GLFW_KEY_2:                  return Key_D2;
                    case GLFW_KEY_3:                  return Key_D3;
                    case GLFW_KEY_4:                  return Key_D4;
                    case GLFW_KEY_5:                  return Key_D5;
                    case GLFW_KEY_6:                  return Key_D6;
                    case GLFW_KEY_7:                  return Key_D7;
                    case GLFW_KEY_8:                  return Key_D8;
                    case GLFW_KEY_9:                  return Key_D9;
                    case GLFW_KEY_ESCAPE:             return Key_Escape;
                    case GLFW_KEY_ENTER:              return Key_Enter;
                    case GLFW_KEY_TAB:                return Key_Tab;
                    case GLFW_KEY_BACKSPACE:          return Key_Back;
                    case GLFW_KEY_INSERT:             return Key_Insert;
                    case GLFW_KEY_DELETE:             return Key_Delete;
                    case GLFW_KEY_RIGHT:              return Key_Right;
                    case GLFW_KEY_LEFT:               return Key_Left;
                    case GLFW_KEY_DOWN:               return Key_Down;
                    case GLFW_KEY_UP:                 return Key_Up;
                    case GLFW_KEY_PAGE_UP:            return Key_PageUp;
                    case GLFW_KEY_PAGE_DOWN:          return Key_PageDown;
                    case GLFW_KEY_HOME:               return Key_Home;
                    case GLFW_KEY_END:                return Key_End;
                    case GLFW_KEY_CAPS_LOCK:          return Key_CapsLock;
                    case GLFW_KEY_SCROLL_LOCK:        return Key_Scroll;
                    case GLFW_KEY_NUM_LOCK:           return Key_NumLock;
                    case GLFW_KEY_PRINT_SCREEN:       return Key_PrintScreen;
                    case GLFW_KEY_PAUSE:              return Key_Pause;
                    case GLFW_KEY_F1:                 return Key_F1;
                    case GLFW_KEY_F2:                 return Key_F2;
                    case GLFW_KEY_F3:                 return Key_F3;
                    case GLFW_KEY_F4:                 return Key_F4;
                    case GLFW_KEY_F5:                 return Key_F5;
                    case GLFW_KEY_F6:                 return Key_F6;
                    case GLFW_KEY_F7:                 return Key_F7;
                    case GLFW_KEY_F8:                 return Key_F8;
                    case GLFW_KEY_F9:                 return Key_F9;
                    case GLFW_KEY_F10:                return Key_F10;
                    case GLFW_KEY_F11:                return Key_F11;
                    case GLFW_KEY_F12:                return Key_F12;
                    case GLFW_KEY_KP_0:               return Key_NumPad0;
                    case GLFW_KEY_KP_1:               return Key_NumPad1;
                    case GLFW_KEY_KP_2:               return Key_NumPad2;
                    case GLFW_KEY_KP_3:               return Key_NumPad3;
                    case GLFW_KEY_KP_4:               return Key_NumPad4;
                    case GLFW_KEY_KP_5:               return Key_NumPad5;
                    case GLFW_KEY_KP_6:               return Key_NumPad6;
                    case GLFW_KEY_KP_7:               return Key_NumPad7;
                    case GLFW_KEY_KP_8:               return Key_NumPad8;
                    case GLFW_KEY_KP_9:               return Key_NumPad9;
                    default:
                        log_warn("Unhandled key: {:#x}", key);
                        return Key_None;
                }
            }();
            switch (action) {
                case GLFW_PRESS:
                    s_event_proxy->KeyDown(ns_key);
                    break;
                case GLFW_RELEASE:
                    s_event_proxy->KeyUp(ns_key);
                    break;
            }
        }
    }

    context::context() {
        create_render_pass();

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
        platform_subsystem::s_key_callbacks.emplace_back(&on_key_event);
    }

    context::~context() {
        vkb::vkdvc().destroyRenderPass(m_render_pass, &vkb::s_allocator);
        NsShutdownPackages_NoesisApp();
        m_app.Reset();
        m_device.Reset();
    }

    auto context::render(const vk::CommandBuffer cmd) -> void {
        m_app->GetMainWindow()->GetView()->GetRenderer()->UpdateRenderTree();

        constexpr vk::CommandBufferBeginInfo begin_info {};
        cmd.begin(&begin_info);

        const NoesisApp::VKFactory::RecordingInfo recording_info {
            .commandBuffer = cmd,
            .frameNumber = vkb::ctx().get_image_index(),
            .safeFrameNumber = vkb::ctx().get_image_index()
        };
        NoesisApp::VKFactory::SetCommandBuffer(m_device, recording_info);
        m_app->GetMainWindow()->GetView()->GetRenderer()->RenderOffscreen();

        auto w = static_cast<float>(vkb::ctx().get_width());
        auto h = static_cast<float>(vkb::ctx().get_height());
        // Update dynamic viewport state
        vk::Viewport viewport {};
        viewport.width = w;
        viewport.height = -h;
        viewport.x = 0.0f;
        viewport.y = h;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        cmd.setViewport(0, 1, &viewport);

        // Update dynamic scissor state
        vk::Rect2D scissor {};
        scissor.extent.width = w;
        scissor.extent.height = h;
        scissor.offset.x = 0.0f;
        scissor.offset.y = 0.0f;
        cmd.setScissor(0, 1, &scissor);

        vkb::ctx().begin_render_pass(cmd, m_render_pass, vk::SubpassContents::eInline);
        NoesisApp::VKFactory::SetRenderPass(m_device, m_render_pass, static_cast<std::uint32_t>(vkb::k_msaa_sample_count));
        m_app->GetMainWindow()->GetView()->GetRenderer()->Render();
        vkb::ctx().end_render_pass(cmd);

        cmd.end();
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

    auto context::on_resize() -> void {
        m_app->Resize();
    }

    auto context::create_render_pass() -> void {
        std::array<vk::AttachmentDescription, 3> attachments {};

        // Multisampled attachment that we render to
        attachments[0].format = vkb::ctx().get_swapchain().get_format();
        attachments[0].samples = vkb::k_msaa_sample_count;
        attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
        attachments[0].storeOp = vk::AttachmentStoreOp::eDontCare;
        attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachments[0].initialLayout = vk::ImageLayout::eUndefined;
        attachments[0].finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

        // This is the frame buffer attachment to where the multisampled image
        // will be resolved to and which will be presented to the swapchain
        attachments[1].format = vkb::ctx().get_swapchain().get_format();
        attachments[1].samples = vk::SampleCountFlagBits::e1;
        attachments[1].loadOp = vk::AttachmentLoadOp::eDontCare;
        attachments[1].storeOp = vk::AttachmentStoreOp::eStore;
        attachments[1].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachments[1].initialLayout = vk::ImageLayout::eUndefined;
        attachments[1].finalLayout = vk::ImageLayout::ePresentSrcKHR;

        // Multisampled depth attachment we render to
        attachments[2].format = vkb::dvc().get_depth_format();
        attachments[2].samples = vkb::k_msaa_sample_count;
        attachments[2].loadOp = vk::AttachmentLoadOp::eClear;
        attachments[2].storeOp = vk::AttachmentStoreOp::eDontCare;
        attachments[2].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachments[2].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachments[2].initialLayout = vk::ImageLayout::eUndefined;
        attachments[2].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference color_reference {};
        color_reference.attachment = 0;
        color_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

        // Resolve attachment reference for the color attachment
        vk::AttachmentReference resolve_reference {};
        resolve_reference.attachment = 1;
        resolve_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference depth_reference {};
        depth_reference.attachment = 2;
        depth_reference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::SubpassDescription subpass_description {};
        subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &color_reference;
        subpass_description.pDepthStencilAttachment = &depth_reference;
        subpass_description.inputAttachmentCount = 0;
        subpass_description.pInputAttachments = nullptr;
        subpass_description.preserveAttachmentCount = 0;
        subpass_description.pPreserveAttachments = nullptr;
        subpass_description.pResolveAttachments = &resolve_reference;

        // Subpass dependencies for layout transitions
        std::array<vk::SubpassDependency, 2> dependencies {};

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
        dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
        dependencies[0].srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        dependencies[0].dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;

        dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].dstSubpass = 0;
        dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[1].dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;

        vk::RenderPassCreateInfo render_pass_ci {};
        render_pass_ci.attachmentCount = static_cast<std::uint32_t>(attachments.size());
        render_pass_ci.pAttachments = attachments.data();
        render_pass_ci.subpassCount = 1;
        render_pass_ci.pSubpasses = &subpass_description;
        render_pass_ci.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
        render_pass_ci.pDependencies = dependencies.data();

        vkcheck(vkb::vkdvc().createRenderPass(&render_pass_ci, &vkb::s_allocator, &m_render_pass));
    }
}
