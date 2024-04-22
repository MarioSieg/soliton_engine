// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "Window.h"
#include <GLFW/glfw3.h>

namespace NoesisApp
{
    NS_IMPLEMENT_REFLECTION(Window) { }

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

    auto Window::Init(Noesis::RenderDevice* const device, const std::uint16_t width, const std::uint16_t height) -> void
    {
        this->m_view = Noesis::GUI::CreateView(this);
        this->m_view->SetSize(
            static_cast<std::uint32_t>(width),
            static_cast<std::uint32_t>(height)
        );
        this->m_view->SetFlags(Noesis::RenderFlags_LCD|Noesis::RenderFlags_FlipY|Noesis::RenderFlags_PPAA);
        this->m_view->GetRenderer()->Init(device);
        this->m_view->SetTessellationMaxPixelError(Noesis::TessellationMaxPixelError::HighQuality());
        this->m_view->SetScale(1.F);
    }

    Window::~Window()
    {
        this->m_view.Reset();
    }
}
