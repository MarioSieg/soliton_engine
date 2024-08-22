// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <cstdint>

#include <NoesisPCH.h>

namespace NoesisApp
{
    struct Window : Noesis::ContentControl
    {
        Window() = default;
        Window(const Window&) = delete;
        Window(Window&&) = delete;
        auto operator =(const Window&) -> Window& = delete;
        auto operator =(Window&&) -> Window& = delete;
        virtual ~Window() override;

        auto Init(Noesis::RenderDevice* device, std::uint16_t width, std::uint16_t height, bool wireframe) -> void;
        auto Tick() -> void;
        auto Resize(std::uint16_t width, std::uint16_t height) -> void;
        [[nodiscard]] auto GetView() const noexcept -> Noesis::IView* {
            return this->m_view.GetPtr();
        }

    private:
        Noesis::Ptr<Noesis::IView> m_view { };
        NS_DECLARE_REFLECTION(Window, ContentControl)
    };
}
