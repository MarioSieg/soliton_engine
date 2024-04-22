////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_MESSAGEBOX_H__
#define __APP_MESSAGEBOX_H__


#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/Delegate.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsGui/UserControl.h>
#include <NsApp/ApplicationLauncherApi.h>


namespace NoesisApp
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Specifies which message box button a user clicked.
////////////////////////////////////////////////////////////////////////////////////////////////////
enum class MessageBoxResult: int32_t
{
    /// The message box returns no result.
    None = 0,
    /// The result value of the message box is OK.
    OK = 1,
    /// The result value of the message box is Cancel.
    Cancel = 2,
    /// The result value of the message box is Yes.
    Yes = 6,
    /// The result value of the message box is No.
    No = 7
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Specifies the buttons that are displayed on a message box.
////////////////////////////////////////////////////////////////////////////////////////////////////
enum class MessageBoxButton: int32_t
{
    /// The message box displays an OK button.
    OK = 0,
    /// The message box displays OK and Cancel buttons.
    OKCancel = 1,
    /// The message box displays Yes, No, and Cancel buttons.
    YesNoCancel = 3,
    /// The message box displays Yes and No buttons.
    YesNo = 4
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Specifies the icon that is displayed by a message box.
////////////////////////////////////////////////////////////////////////////////////////////////////
enum class MessageBoxImage: int32_t
{
    /// No icon is displayed.
    None = 0,
    /// The message box contains a symbol consisting of a white X inside a red circle.
    Hand = 0x10,
    /// The message box contains a symbol consisting of a question mark in a blue circle.
    Question = 0x20,
    /// The message box contains a symbol consisting of an exclamation inside a yellow triangle.
    Exclamation = 0x30,
    /// The message box contains a symbol consisting of a lowercase letter i in a blue circle.
    Asterisk = 0x40,

    Stop = Hand,
    Error = Hand,
    Warning = Exclamation,
    Information = Asterisk
};

////////////////////////////////////////////////////////////////////////////////////////////////////
typedef Noesis::Delegate<void (MessageBoxResult result)> MessageBoxResultCallback;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Displays a message box by calling the static *Show* method. The callback gets called when the
/// dialog is closed indicating which button was pressed as result.
///
/// .. code-block:: c++
///    :caption: C++
///
///    MessageBox::Show("Save changes to the modified file?", "Close", MessageBoxButton::YesNo,
///        [](MessageBoxResult result)
///        {
///            if (result == MessageBoxResult::Yes) { /* save file */ }
///        });
///
/// .. code-block:: c#
///    :caption: C#
///
///    MessageBox.Show("Save changes to the modified file?", "Close", MessageBoxButton.YesNo,
///        (result) =>
///        {
///            if (result == MessageBoxResult.Yes) { /* save file */ }
///        });
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_APPLICATIONLAUNCHER_API MessageBox final: public Noesis::UserControl
{
public:
    /// Displays a message box that has a message and uses a callback to notify the result.
    static void Show(const char* text, const MessageBoxResultCallback& callback);

    /// Displays a message box that has a message and title bar caption; it uses a callback to
    /// notify the result.
    static void Show(const char* text, const char* caption,
        const MessageBoxResultCallback& callback);

    /// Displays a message box that has a message, title bar caption, and button; it uses a
    /// callback to notify the result.
    static void Show(const char* text, const char* caption, MessageBoxButton button,
        const MessageBoxResultCallback& callback);

    /// Displays a message box that has a message, title bar caption, button, and icon; it uses a
    /// callback to notify the result.
    static void Show(const char* text, const char* caption, MessageBoxButton button,
        MessageBoxImage icon, const MessageBoxResultCallback& callback);

    /// Displays a message box that has a message, title bar caption, button, icon, and accept a
    /// default message box result; it uses a callback to notify the result.
    static void Show(const char* text, const char* caption, MessageBoxButton button,
        MessageBoxImage icon, MessageBoxResult defaultResult,
        const MessageBoxResultCallback& callback);

private:
    MessageBox(Noesis::UIElement* target, const char* text, const char* caption,
        MessageBoxButton button, MessageBoxImage icon, MessageBoxResult defaultResult,
        const MessageBoxResultCallback& callback);

private:
    MessageBoxResultCallback mCallback;
    Noesis::Ptr<Noesis::UIElement> mFocused;

    NS_DECLARE_REFLECTION(MessageBox, Noesis::UserControl)
};

NS_WARNING_POP

}

NS_DECLARE_REFLECTION_ENUM_EXPORT(NS_APP_APPLICATIONLAUNCHER_API, NoesisApp::MessageBoxResult)
NS_DECLARE_REFLECTION_ENUM_EXPORT(NS_APP_APPLICATIONLAUNCHER_API, NoesisApp::MessageBoxButton)
NS_DECLARE_REFLECTION_ENUM_EXPORT(NS_APP_APPLICATIONLAUNCHER_API, NoesisApp::MessageBoxImage)


#endif
