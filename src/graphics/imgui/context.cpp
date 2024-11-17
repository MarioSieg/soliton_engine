// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "context.hpp"

#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"
#include "font_awesome.ttf.inl"
#include "jetbrains_mono.ttf.inl"
#include "font_awesome_pro_5.hpp"
#include "text_editor.hpp"
#include "implot.h"
#include "../pipeline_cache.hpp"
#include "../vulkancore/context.hpp"
#include "../../platform/platform_subsystem.hpp"
#include "../../scripting/system_variable.hpp"

#include <vk_mem_alloc.h>

namespace lu::imgui {
    static const system_variable<float> sv_font_size {"editor.font_size", {18.0f}};

    context::context() {
#if USE_MIMALLOC
        ImGui::SetAllocatorFunctions(
            +[](size_t size, [[maybe_unused]] void* usr) -> void* {
                return mi_malloc(size);
            },
            +[](void* ptr, [[maybe_unused]] void* usr) -> void {
                mi_free(ptr);
            }
        );
#endif
        ImGui::CreateContext();
        ImPlot::CreateContext();
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = 800.0f;
        io.DisplaySize.y = 600.0f;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.IniFilename = nullptr;


        GLFWwindow* const window = *platform::platform_subsystem::get_main_window();
        ImGui_ImplGlfw_InitForVulkan(window, true);

        auto& device = vkb::dvc();
        vk::Device vdevice = vkb::vkdvc();

        m_imgui_descriptor_pool = vkb::ctx().get_descriptor_allocator().request_pool();

        ImGui_ImplVulkan_InitInfo init_info {};
        init_info.Instance = device.get_instance();
        init_info.PhysicalDevice = device.get_physical_device();
        init_info.Device = device.get_logical_device();
        init_info.QueueFamily = vkb::ctx().get_swapchain().get_queue_node_index();
        init_info.Queue = device.get_graphics_queue();
        init_info.PipelineCache = graphics::pipeline_cache::get().get_cache();
        init_info.DescriptorPool = m_imgui_descriptor_pool;
        init_info.ImageCount = vkb::ctx().get_concurrent_frame_count();
        init_info.MinImageCount = vkb::ctx().get_concurrent_frame_count();
        init_info.MSAASamples = static_cast<VkSampleCountFlagBits>(vkb::ctx().get_msaa_samples());
        init_info.Allocator = reinterpret_cast<const VkAllocationCallbacks*>(vkb::get_alloc());
        init_info.CheckVkResultFn = [](const VkResult result) {
            vkccheck(result);
        };
        panic_assert(ImGui_ImplVulkan_Init(&init_info, vkb::ctx().get_scene_render_pass()));

        const float font_size = sv_font_size();

        // add primary text font:
        ImFontConfig config { };
        config.FontDataOwnedByAtlas = false;
        config.MergeMode = false;
        ImFont* primaryFont = io.Fonts->AddFontFromMemoryTTF(
            const_cast<void*>(static_cast<const void*>(k_ttf_jet_brains_mono)),
            sizeof(k_ttf_jet_brains_mono) / sizeof(*k_ttf_jet_brains_mono),
            font_size, // font size
            &config,
            io.Fonts->GetGlyphRangesDefault()
        );

        // now add font awesome icons:
        config.MergeMode = true;
        config.DstFont = primaryFont;
        struct font_range final {
            std::span<const std::uint8_t> data {};
            std::array<char16_t, 3> ranges {};
        };

        // Compute DPI scaling
        float scale = 1.0f;
        float xscale;
        float yscale;
        glfwGetWindowContentScale(window, &xscale, &yscale);
        scale = (xscale+yscale)*0.5f;
        log_info("DPI scaling: {}", scale);

        static constexpr font_range range = { k_font_awesome_ttf, { ICON_MIN_FA, ICON_MAX_FA, 0 } };
        ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
            const_cast<void*>(static_cast<const void*>(range.data.data())),
            static_cast<int>(range.data.size()),
            font_size-(scale>1.0f?8.0f:5.0f),
            &config,
            reinterpret_cast<const ImWchar*>(range.ranges.data())
        );
        static_assert(sizeof(ImWchar) == sizeof(char16_t));
        panic_assert(ImGui_ImplVulkan_CreateFontsTexture());

        // Apply DPI scaling
        if constexpr (PLATFORM_OSX) {
            io.FontGlobalScale = 1.0f / scale;
        } else {
            ImGui::GetStyle().ScaleAllSizes(scale);
        }
    }

    context::~context() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        vkb::vkdvc().destroyDescriptorPool(m_imgui_descriptor_pool, vkb::get_alloc());
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

    auto context::submit_imgui(vk::CommandBuffer cmd_buf) -> void {
        if (auto* dd = ImGui::GetDrawData()) [[likely]] {
            ImGui_ImplVulkan_RenderDrawData(dd, cmd_buf);
        }
    }

    auto context::begin_frame() -> void {
        const auto w = static_cast<float>(vkb::ctx().get_width());
        const auto h = static_cast<float>(vkb::ctx().get_height());
        ImGui::NewFrame();
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = w;
        io.DisplaySize.y = h;
        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplVulkan_NewFrame();
    }

    auto context::end_frame() -> void {
        ImGui::Render();
    }
}
