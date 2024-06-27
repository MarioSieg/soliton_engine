// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <NsApp/AttachableObject.h>
#include <NsRender/RenderDevice.h>
#include <NsGui/IView.h>
#include <NsRender/RenderDevice.h>
#include <NsCore/Ptr.h>

#include "../vulkancore/prelude.hpp"
#include <NsApp/Application.h>

namespace noesis {
    class context final {
    public:
        context();
        context(const context&) = delete;
        context(context&&) = delete;
        auto operator =(const context&) -> context& = delete;
        auto operator =(context&&) -> context& = delete;
        ~context();

        auto load_ui_from_xaml(std::string&& path) -> void;
        auto reload_ui(bool render_wireframe = false) -> void;
        auto render_offscreen(vk::CommandBuffer cmd) -> void;
        auto render_onscreen(vk::RenderPass pass) -> void;
        auto tick() -> void;
        auto on_resize() -> void;

    private:
        std::string m_xaml_path {};
        Noesis::Ptr<NoesisApp::Application> m_app {};
        Noesis::Ptr<Noesis::RenderDevice> m_device {};
    };
}
