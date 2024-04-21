////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include "MainMenu.xaml.h"
#include "ViewModel.h"

#include <NsGui/Uri.h>
#include <NsGui/IntegrationAPI.h>
#include <NsCore/ReflectionImplementEmpty.h>

using namespace Menu3D;
using namespace Noesis;

MainMenu::MainMenu()
{
    Loaded() += MakeDelegate(this, &MainMenu::OnLoaded);
    InitializeComponent();
}

void MainMenu::InitializeComponent()
{
    GUI::LoadComponent(this, "MainMenu.xaml");
}

NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION_(Menu3D::MainMenu, "Menu3D.MainMenu")

void MainMenu::OnLoaded(Noesis::BaseComponent *, const RoutedEventArgs &) {
    SetDataContext(MakePtr<ViewModel>());
}

NS_END_COLD_REGION
