////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/Noesis.h>
#include <NsCore/RegisterComponent.h>
#include <NsApp/ApplicationLauncher.h>
#include <NsApp/EntryPoint.h>
#include <NsApp/EmbeddedXamlProvider.h>
#include <NsApp/LocalXamlProvider.h>

#include "App.xaml.h"
#include "App.xaml.bin.h"
#include "MainWindow.xaml.h"
#include "MainWindow.xaml.bin.h"


using namespace Noesis;
using namespace NoesisApp;


static const char Content_xaml[] =
R"(
    <Grid
        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">

        <Grid.Resources>
            <SolidColorBrush x:Key="Brush.Help.Text" Color="#FF8F9295"/>
            <SolidColorBrush x:Key="Brush.Help.Shortcut" Color="#FFB5B9BC"/>
        </Grid.Resources>
)"
#if defined(NS_PLATFORM_WINDOWS_DESKTOP) || defined(NS_PLATFORM_OSX) || defined(NS_PLATFORM_LINUX)
R"(
        <Grid>
            <Grid.RowDefinitions>
                <RowDefinition Height="40*"/>
                <RowDefinition Height="20*" MinHeight="170"/>
                <RowDefinition Height="40*"/>
            </Grid.RowDefinitions>
            <Viewbox Grid.Row="1" HorizontalAlignment="Center">
                <StackPanel>
                    <TextBlock
                        Text="Drag &amp; Drop XAML files here&#xA;"
                        HorizontalAlignment="Center"
                        VerticalAlignment="Center"
                        TextAlignment="Center"
                        Foreground="{StaticResource Brush.Help.Text}"/>
                    <StackPanel Orientation="Horizontal" HorizontalAlignment="Center">
                        <TextBlock Foreground="{StaticResource Brush.Help.Shortcut}" Width="39">
                            <Run Text="F1"/><LineBreak/>
                            <Run Text="F5"/><LineBreak/>
                            <Run Text="CTRL"/><LineBreak/>
                            <Run Text="CTRL"/><LineBreak/>
                            <Run Text="CTRL"/><LineBreak/>
                            <Run Text="CTRL"/><LineBreak/>
                            <Run Text="ALT"/>
                        </TextBlock>
                        <TextBlock Foreground="{StaticResource Brush.Help.Shortcut}" Width="125">
                            <Run/><LineBreak/>
                            <Run/><LineBreak/>
                            <Run Text="+ T"/><LineBreak/>
                            <Run Text="+ R"/><LineBreak/>
                            <Run Text="+ Mouse Wheel"/><LineBreak/>
                            <Run Text="+ Right Mouse"/><LineBreak/>
                            <Run Text="+ Right Mouse"/>
                        </TextBlock>
                        <TextBlock Foreground="{StaticResource Brush.Help.Text}">
                            <Run Text="Help"/><LineBreak/>
                            <Run Text="Reload"/><LineBreak/>
                            <Run Text="Settings"/><LineBreak/>
                            <Run Text="Reset View"/><LineBreak/>
                            <Run Text="Zoom View"/><LineBreak/>
                            <Run Text="Pan View"/><LineBreak/>
                            <Run Text="Rotate View"/>
                        </TextBlock>
                    </StackPanel>
                </StackPanel>
            </Viewbox>
        </Grid>
)"
#elif defined(NS_PLATFORM_SCE)
R"(
        <Grid>
            <Grid.RowDefinitions>
                <RowDefinition Height="46*"/>
                <RowDefinition Height="7*" MinHeight="57"/>
                <RowDefinition Height="47*"/>
            </Grid.RowDefinitions>
            <Viewbox Grid.Row="1" HorizontalAlignment="Center">
                <StackPanel>
                    <TextBlock
                        Text="Connect Inspector and edit 'Content.xaml'&#xA;"
                        HorizontalAlignment="Center" VerticalAlignment="Center"
                        TextAlignment="Center"
                        Foreground="{StaticResource Brush.Help.Text}"/>
                    <TextBlock
                        HorizontalAlignment="Center"
                        VerticalAlignment="Center"
                        TextAlignment="Center"
                        Foreground="{StaticResource Brush.Help.Text}">
                        <Run Text="Press"/>
                        <Run Text="Options" Foreground="{StaticResource Brush.Help.Shortcut}"/>
                        <Run Text="button for Settings"/>
                    </TextBlock>
                </StackPanel>
            </Viewbox>
        </Grid>
)"
#elif defined(NS_PLATFORM_XBOX_ONE)
R"(
        <Grid>
            <Grid.RowDefinitions>
                <RowDefinition Height="46*"/>
                <RowDefinition Height="7*" MinHeight="57"/>
                <RowDefinition Height="47*"/>
            </Grid.RowDefinitions>
            <Viewbox Grid.Row="1" HorizontalAlignment="Center">
                <StackPanel>
                    <TextBlock
                        Text="Connect Inspector and edit 'Content.xaml'&#xA;"
                        HorizontalAlignment="Center" VerticalAlignment="Center"
                        TextAlignment="Center"
                        Foreground="{StaticResource Brush.Help.Text}"/>
                    <TextBlock
                        HorizontalAlignment="Center"
                        VerticalAlignment="Center"
                        TextAlignment="Center"
                        Foreground="{StaticResource Brush.Help.Text}">
                        <Run Text="Press"/>
                        <Run Text="Menu" Foreground="{StaticResource Brush.Help.Shortcut}"/>
                        <Run Text="button for Settings"/>
                    </TextBlock>
                </StackPanel>
            </Viewbox>
        </Grid>
)"
#else
R"(
        <Grid>
            <Grid.RowDefinitions>
                <RowDefinition Height="46*"/>
                <RowDefinition Height="7*" MinHeight="57"/>
                <RowDefinition Height="47*"/>
            </Grid.RowDefinitions>
            <Viewbox Grid.Row="1" HorizontalAlignment="Center">
                <TextBlock
                    Text="Connect Inspector and edit 'Content.xaml'&#xA;&#xA;Swipe from left border for Settings"
                    HorizontalAlignment="Center"
                    VerticalAlignment="Center"
                    TextAlignment="Center"
                    Foreground="{StaticResource Brush.Help.Text}"/>
            </Viewbox>
        </Grid>
)"
#endif
R"(
    </Grid>
)";

////////////////////////////////////////////////////////////////////////////////////////////////////
class AppLauncher final: public ApplicationLauncher
{
public:
    void RegisterComponents() const override
    {
        RegisterComponent<XamlPlayer::App>();
        RegisterComponent<XamlPlayer::MainWindow>();
    }

    Noesis::Ptr<XamlProvider> GetXamlProvider() const override
    {
        EmbeddedXaml xamls[] = 
        {
            { "App.xaml", App_xaml },
            { "MainWindow.xaml", MainWindow_xaml },
            { "Content.xaml", ArrayRef<uint8_t>((uint8_t*)Content_xaml, sizeof(Content_xaml) - 1) }
        };

        Noesis::Ptr<XamlProvider> fallback = *new LocalXamlProvider("");
        fallback->XamlChanged() += [](const Noesis::Uri&)
        {
            auto window = (XamlPlayer::MainWindow*)Application::Current()->GetMainWindow();
            window->ClearErrors();
        };

        return *new EmbeddedXamlProvider(xamls, fallback);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
int NsMain(int argc, char **argv)
{
    AppLauncher launcher;
    launcher.SetArguments(argc, argv);
    launcher.SetApplicationFile("App.xaml");
    return launcher.Run();
}
