// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <NoesisPCH.h>
#include "Window.h"

namespace NoesisApp
{
    struct Application : Noesis::BaseComponent, Noesis::IUITreeNode
    {
        Application() = default;
        Application(const Application&) = delete;
        Application(Application&&) = delete;
        auto operator =(const Application&) -> Application& = delete;
        auto operator =(Application&&) -> Application& = delete;
        virtual ~Application() override;

        auto Init
        (
            Noesis::RenderDevice* device,
            const Noesis::Uri& startupUri,
            std::uint16_t width,
            std::uint16_t height,
            bool wireframe
        ) -> void;
        auto Tick() -> void;
        auto Resize() -> void;

        auto GetStartupUri() const noexcept -> const Noesis::Uri&;
        auto SetStartupUri(const Noesis::Uri& startupUri) noexcept -> void;

        auto GetResources() const noexcept -> Noesis::ResourceDictionary*;
        auto SetResources(Noesis::ResourceDictionary* resources) noexcept -> void;

        auto GetMainWindow() const -> Window*;

        auto GetNodeParent() const -> IUITreeNode* override;
        auto SetNodeParent(IUITreeNode* parent) -> void override;
        auto FindNodeResource(const char* key, bool fullSearch) const -> Noesis::BaseComponent* override;
        auto FindNodeName(const char* name) const -> Noesis::ObjectWithNameScope  override;

        NS_IMPLEMENT_INTERFACE_FIXUP

    private:
        IUITreeNode* m_owner { };
        Noesis::Uri m_boot_uri { };
        Noesis::Ptr<Window> m_main_window { };
        mutable Noesis::Ptr<Noesis::ResourceDictionary> m_resources { };

        NS_DECLARE_REFLECTION(Application, BaseComponent)
    };
}
