// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "Window.h"
#include <GLFW/glfw3.h>

#include "../../../../../platform/platform_subsystem.hpp"
#include "../../../../../core/system_variable.hpp"

namespace soliton::scripting {
    class scripting_subsystem;
}

namespace NoesisApp
{
    using namespace soliton;

    static const system_variable<std::int64_t> ui_render_flags {
        "ui.render_flags",
        {Noesis::RenderFlags_LCD | Noesis::RenderFlags_FlipY | Noesis::RenderFlags_PPAA}
    };
    static const system_variable<float> ui_tesselation_pixel_error {
        "ui.tesselation_error",
        {Noesis::TessellationMaxPixelError::HighQuality().error}
    };

    NS_IMPLEMENT_REFLECTION(Window) { }

    static_assert(Noesis::RenderFlags::RenderFlags_Wireframe == 1, "Update scripts/config/engine.lua -> UI_RENDER_FLAGS");
    static_assert(Noesis::RenderFlags::RenderFlags_ColorBatches == 2, "Update scripts/config/engine.lua -> UI_RENDER_FLAGS");
    static_assert(Noesis::RenderFlags::RenderFlags_Overdraw == 4, "Update scripts/config/engine.lua -> UI_RENDER_FLAGS");
    static_assert(Noesis::RenderFlags::RenderFlags_FlipY == 8, "Update scripts/config/engine.lua -> UI_RENDER_FLAGS");
    static_assert(Noesis::RenderFlags::RenderFlags_PPAA == 16, "Update scripts/config/engine.lua -> UI_RENDER_FLAGS");
    static_assert(Noesis::RenderFlags::RenderFlags_LCD == 32, "Update scripts/config/engine.lua -> UI_RENDER_FLAGS");
    static_assert(Noesis::RenderFlags::RenderFlags_ShowGlyphs == 64, "Update scripts/config/engine.lua -> UI_RENDER_FLAGS");
    static_assert(Noesis::RenderFlags::RenderFlags_ShowRamps == 128, "Update scripts/config/engine.lua -> UI_RENDER_FLAGS");
    static_assert(Noesis::RenderFlags::RenderFlags_DepthTesting == 256, "Update scripts/config/engine.lua -> UI_RENDER_FLAGS");

    auto Window::Tick() -> void
    {
        this->m_view->Update(glfwGetTime());
    }

    auto Window::Resize(const std::uint16_t width, const std::uint16_t height) -> void
    {
        this->m_view->SetSize(
            static_cast<std::uint32_t>(width),
            static_cast<std::uint32_t>(height)
        );
    }

    auto Window::Init(Noesis::RenderDevice* const device, const std::uint16_t width, const std::uint16_t height, const bool wireframe) -> void
    {
        this->m_view = Noesis::GUI::CreateView(this);
        this->m_view->SetSize(
            static_cast<std::uint32_t>(width),
            static_cast<std::uint32_t>(height)
        );
        std::uint32_t flags = ui_render_flags();
        if (wireframe) {
            flags |= Noesis::RenderFlags::RenderFlags_Wireframe;
        }
        this->m_view->SetFlags(flags);
        this->m_view->GetRenderer()->Init(device);
        this->m_view->SetTessellationMaxPixelError(ui_tesselation_pixel_error());
        const XMFLOAT2 scale = platform::platform_subsystem::get_main_window().get_content_scale();
        this->m_view->SetScale((scale.x+scale.y) * 0.5f);
    }

    Window::~Window()
    {
        this->m_view.Reset();
    }
}
