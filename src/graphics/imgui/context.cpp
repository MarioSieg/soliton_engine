// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "context.hpp"

#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"
#include "font_awesome.ttf.inl"
#include "jetbrains_mono.ttf.inl"
#include "font_awesome_pro_5.hpp"
#include "text_editor.hpp"
#include "implot.h"
#include "../pipeline.hpp"
#include "../../platform/platform_subsystem.hpp"
#include "../../scripting/scripting_subsystem.hpp"

#include <vk_mem_alloc.h>

namespace imgui {
    context::context() {
        ImGui::SetAllocatorFunctions(
            +[](size_t size, [[maybe_unused]] void* usr) -> void* {
                return mi_malloc(size);
            },
            +[](void* ptr, [[maybe_unused]] void* usr) -> void {
                mi_free(ptr);
            }
        );
        ImGui::CreateContext();
        ImPlot::CreateContext();
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = 800.0f;
        io.DisplaySize.y = 600.0f;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.IniFilename = nullptr;


        GLFWwindow* window = platform::platform_subsystem::get_glfw_window();
        ImGui_ImplGlfw_InitForVulkan(window, true);

        constexpr std::size_t k_num = 1024;
        constexpr std::array<vk::DescriptorPoolSize, 11> k_pool_sizes {
            vk::DescriptorPoolSize { vk::DescriptorType::eSampler, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eSampledImage, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eStorageImage, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eUniformTexelBuffer, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eStorageTexelBuffer, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eStorageBuffer, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eUniformBufferDynamic, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eStorageBufferDynamic, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eInputAttachment, k_num }
        };

        auto& device = vkb::dvc();
        vk::Device vdevice = vkb::vkdvc();

        vk::DescriptorPoolCreateInfo pool_info = {};
        pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        pool_info.maxSets = k_num;
        pool_info.poolSizeCount = k_pool_sizes.size();
        pool_info.pPoolSizes = k_pool_sizes.data();

        vkcheck(vdevice.createDescriptorPool(&pool_info, &vkb::s_allocator, &m_imgui_descriptor_pool));

        ImGui_ImplVulkan_InitInfo init_info {};
        init_info.Instance = device.get_instance();
        init_info.PhysicalDevice = device.get_physical_device();
        init_info.Device = device.get_logical_device();
        init_info.QueueFamily = vkb::ctx().get_swapchain().get_queue_node_index();
        init_info.Queue = device.get_graphics_queue();
        init_info.PipelineCache = graphics::pipeline_registry::get().get_cache();
        init_info.DescriptorPool = m_imgui_descriptor_pool;
        init_info.ImageCount = vkb::context::k_max_concurrent_frames;
        init_info.MinImageCount = vkb::context::k_max_concurrent_frames;
        init_info.MSAASamples = static_cast<VkSampleCountFlagBits>(vkb::k_msaa_sample_count);
        init_info.Allocator = reinterpret_cast<const VkAllocationCallbacks*>(&vkb::s_allocator);
        init_info.CheckVkResultFn = [](const VkResult result) {
            vkccheck(result);
        };
        passert(ImGui_ImplVulkan_Init(&init_info, vkb::ctx().get_render_pass()));

        const float font_size = scripting::scripting_subsystem::get_config_table()["Renderer"]["uiFontSize"].cast<float>().valueOr(18.0f);

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
        static constexpr font_range range = { k_font_awesome_ttf, { ICON_MIN_FA, ICON_MAX_FA, 0 } };
        ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
            const_cast<void*>(static_cast<const void*>(range.data.data())),
            static_cast<int>(range.data.size()),
            font_size-5.0f,
            &config,
            reinterpret_cast<const ImWchar*>(range.ranges.data())
        );
        static_assert(sizeof(ImWchar) == sizeof(char16_t));
        passert(ImGui_ImplVulkan_CreateFontsTexture());

        // Apply DPI scaling
        float scale = 1.0f;
        float xscale;
        float yscale;
        glfwGetWindowContentScale(window, &xscale, &yscale);
        scale = (xscale + yscale) * 0.5f;
        if constexpr (PLATFORM_OSX) {
            io.FontGlobalScale = 1.0f / scale;
        } else {
            ImGui::GetStyle().ScaleAllSizes(scale);
        }
    }

    context::~context() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        vkb::vkdvc().destroyDescriptorPool(m_imgui_descriptor_pool, &vkb::s_allocator);
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