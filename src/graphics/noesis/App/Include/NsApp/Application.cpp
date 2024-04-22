// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "Application.h"
#include "../../../../core/core.hpp"
#include "../../../vulkancore/context.hpp"

#include <NsCore/Package.h>
#include <NsCore/EnumConverter.h>

NS_REGISTER_REFLECTION(App, ApplicationLauncher)
{
    NS_REGISTER_COMPONENT(NoesisApp::Application)
    NS_REGISTER_COMPONENT(NoesisApp::Window)
}

NS_INIT_PACKAGE(App, ApplicationLauncher) { }

NS_SHUTDOWN_PACKAGE(App, ApplicationLauncher) { }

namespace NoesisApp
{
    auto Application::Init
    (
        Noesis::RenderDevice* const device,
        const Noesis::Uri& startupUri,
        const std::uint16_t width,
        const std::uint16_t height
    ) -> void
    {
        passert(!Noesis::StrIsNullOrEmpty(this->m_boot_uri.Str()));

        Noesis::Ptr<BaseComponent> root = Noesis::GUI::LoadXaml(this->m_boot_uri.Str());
        this->m_main_window = DynamicPtrCast<Window>(root);

        // Non window roots are allowed
        if (!this->m_main_window) {
            this->m_main_window = *new Window {};
            this->m_main_window->DependencyObject::Init();
            this->m_main_window->SetContent(root);
        }

        this->m_main_window->Init(device, width, height);
    }

    auto Application::GetStartupUri() const noexcept -> const Noesis::Uri&
    {
        return this->m_boot_uri;
    }

    auto Application::SetStartupUri(const Noesis::Uri& startupUri) noexcept -> void
    {
        this->m_boot_uri = startupUri;
    }

    auto Application::GetResources() const noexcept -> Noesis::ResourceDictionary*
    {
        if (!this->m_resources)
        {
            this->m_resources = *new Noesis::ResourceDictionary();
            ConnectNode(this->m_resources, this);
        }
        return this->m_resources;
    }

    auto Application::SetResources(Noesis::ResourceDictionary* const resources) noexcept -> void
    {
        if (this->m_resources != resources)
        {
            DisconnectNode(this->m_resources, this);
            this->m_resources.Reset(resources);
            ConnectNode(this->m_resources, this);

            Noesis::GUI::SetApplicationResources(this->m_resources);
        }
    }

    auto Application::GetNodeParent() const -> IUITreeNode*
    {
        return this->m_owner;
    }

    auto Application::SetNodeParent(IUITreeNode* const parent) -> void
    {
        this->m_owner = parent;
    }

    auto Application::FindNodeResource(const char* const key, const bool fullSearch) const -> Noesis::BaseComponent*
    {
        Noesis::Ptr<BaseComponent> resource;
        if (this->m_resources != nullptr && this->m_resources->Find(key, resource))
        {
            return resource;
        }

        if (this->m_owner != nullptr)
        {
            return this->m_owner->FindNodeResource(key, fullSearch);
        }

        return Noesis::DependencyProperty::GetUnsetValue();
    }

    auto Application::FindNodeName(const char* const name) const -> Noesis::ObjectWithNameScope
    {
        return {}; // TODO
    }

    auto Application::GetMainWindow() const -> Window*
    {
        return this->m_main_window;
    }

    NS_IMPLEMENT_REFLECTION(Application)
    {
        NsImpl<IUITreeNode>();
        NsProp("Resources", &Application::GetResources, &Application::SetResources);
        NsProp("MainWindow", &Application::GetMainWindow);
        NsProp("StartupUri", &Application::m_boot_uri);
    }

    auto Application::Tick() -> void
    {
        this->m_main_window->Tick();
    }

    auto Application::Resize() -> void
    {
        this->m_main_window->Resize(vkb::ctx().get_width(), vkb::ctx().get_height());
    }

    Application::~Application()
    {
        this->m_resources.Reset();
        this->m_main_window.Reset();
    }
}
