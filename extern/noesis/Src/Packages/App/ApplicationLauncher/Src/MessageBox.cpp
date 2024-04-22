////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/MessageBox.h>
#include <NsApp/Application.h>
#include <NsApp/Window.h>
#include <NsApp/DelegateCommand.h>
#include <NsGui/UIElementData.h>
#include <NsGui/PropertyMetadata.h>
#include <NsGui/Keyboard.h>
#include <NsGui/ILayerManager.h>
#include <NsGui/LayerManager.h>
#include <NsGui/IntegrationAPI.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
static const DependencyProperty* MessageBoxTextProperty;
static const DependencyProperty* MessageBoxCaptionProperty;
static const DependencyProperty* MessageBoxButtonProperty;
static const DependencyProperty* MessageBoxDefaultButtonProperty;
static const DependencyProperty* MessageBoxIconProperty;
static const DependencyProperty* MessageBoxCloseProperty;

////////////////////////////////////////////////////////////////////////////////////////////////////
static MessageBoxResult GetDefaultButton(MessageBoxButton button, MessageBoxResult defaultResult)
{
    switch (button)
    {
        case MessageBoxButton::OK:
        {
            return MessageBoxResult::OK;
        }
        case MessageBoxButton::OKCancel:
        {
            if (defaultResult == MessageBoxResult::Cancel)
            {
                return MessageBoxResult::Cancel;
            }
            else
            {
                return MessageBoxResult::OK;
            }
        }
        case MessageBoxButton::YesNoCancel:
        {
            if (defaultResult == MessageBoxResult::Cancel)
            {
                return MessageBoxResult::Cancel;
            }
            else if (defaultResult == MessageBoxResult::No)
            {
                return MessageBoxResult::No;
            }
            else
            {
                return MessageBoxResult::Yes;
            }
        }
        case MessageBoxButton::YesNo:
        {
            if (defaultResult == MessageBoxResult::No)
            {
                return MessageBoxResult::No;
            }
            else
            {
                return MessageBoxResult::Yes;
            }
        }
        default: NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
MessageBox::MessageBox(UIElement* target, const char* text, const char* caption,
    MessageBoxButton button, MessageBoxImage icon, MessageBoxResult defaultResult,
    const MessageBoxResultCallback& callback): mCallback(callback)
{
    ForceCreateDependencyProperties();

    // store focused element
    Keyboard* keyboard = target != nullptr ? target->GetKeyboard() : nullptr;
    mFocused.Reset(keyboard != nullptr ? keyboard->GetFocused() : nullptr);

    MessageBoxResult defaultButton = GetDefaultButton(button, defaultResult);

    Ptr<DelegateCommand> close = *new DelegateCommand([this](BaseComponent* param)
    {
        Ptr<MessageBox> messageBox(this);

        ILayerManager* layers = LayerManager::FindRootLayer(messageBox);
        layers->RemoveLayer(messageBox);

        mCallback(Boxing::Unbox<MessageBoxResult>(param));

        // restore focus
        if (mFocused != nullptr)
        {
            mFocused->Focus();
        }
    });

    SetValue<String>(MessageBoxTextProperty, text);
    SetValue<String>(MessageBoxCaptionProperty, caption);
    SetValue<MessageBoxButton>(MessageBoxButtonProperty, button);
    SetValue<MessageBoxResult>(MessageBoxDefaultButtonProperty, defaultButton);
    SetValue<MessageBoxImage>(MessageBoxIconProperty, icon);
    SetValue<Ptr<DelegateCommand>>(MessageBoxCloseProperty, close);

    GUI::LoadComponent(this, Uri::Pack("App", "MessageBox.xaml"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MessageBox::Show(const char* text, const MessageBoxResultCallback& callback)
{
    Show(text, "", MessageBoxButton::OK, MessageBoxImage::None, MessageBoxResult::None, callback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MessageBox::Show(const char* text, const char* caption,
    const MessageBoxResultCallback& callback)
{
    Show(text, caption, MessageBoxButton::OK, MessageBoxImage::None, MessageBoxResult::None,
        callback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MessageBox::Show(const char* text, const char* caption, MessageBoxButton button,
    const MessageBoxResultCallback& callback)
{
    Show(text, caption, button, MessageBoxImage::None, MessageBoxResult::None, callback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MessageBox::Show(const char* text, const char* caption, MessageBoxButton button,
    MessageBoxImage icon, const MessageBoxResultCallback& callback)
{
    Show(text, caption, button, icon, MessageBoxResult::None, callback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MessageBox::Show(const char* text, const char* caption, MessageBoxButton button,
    MessageBoxImage icon, MessageBoxResult defaultResult, const MessageBoxResultCallback& callback)
{
    Window* window = Application::Current()->GetMainWindow();
    NS_ASSERT(window != nullptr);

    Ptr<MessageBox> messageBox = *new MessageBox(window, text, caption, button, icon, defaultResult,
        callback);

    ILayerManager* layers = LayerManager::FindRootLayer(window);
    layers->AddLayer(messageBox);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(MessageBox)
{
    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<String>(MessageBoxTextProperty, "Text",
        PropertyMetadata::Create(String()));
    data->RegisterProperty<String>(MessageBoxCaptionProperty, "Caption",
        PropertyMetadata::Create(String()));
    data->RegisterProperty<MessageBoxButton>(MessageBoxButtonProperty, "Button",
        PropertyMetadata::Create(MessageBoxButton::OK));
    data->RegisterProperty<MessageBoxResult>(MessageBoxDefaultButtonProperty, "DefaultButton",
        PropertyMetadata::Create(MessageBoxResult::None));
    data->RegisterProperty<MessageBoxImage>(MessageBoxIconProperty, "Icon",
        PropertyMetadata::Create(MessageBoxImage::None));
    data->RegisterProperty<Ptr<DelegateCommand>>(MessageBoxCloseProperty, "Close",
        PropertyMetadata::Create(Ptr<DelegateCommand>()));
};

NS_IMPLEMENT_REFLECTION_ENUM(MessageBoxResult)
{
    NsVal("None", MessageBoxResult::None);
    NsVal("OK", MessageBoxResult::OK);
    NsVal("Cancel", MessageBoxResult::Cancel);
    NsVal("Yes", MessageBoxResult::Yes);
    NsVal("No", MessageBoxResult::No);
}

NS_IMPLEMENT_REFLECTION_ENUM(MessageBoxButton)
{
    NsVal("OK", MessageBoxButton::OK);
    NsVal("OKCancel", MessageBoxButton::OKCancel);
    NsVal("YesNoCancel", MessageBoxButton::YesNoCancel);
    NsVal("YesNo", MessageBoxButton::YesNo);
}

NS_IMPLEMENT_REFLECTION_ENUM(MessageBoxImage)
{
    NsVal("None", MessageBoxImage::None);
    NsVal("Hand", MessageBoxImage::Hand);
    NsVal("Question", MessageBoxImage::Question);
    NsVal("Exclamation", MessageBoxImage::Exclamation);
    NsVal("Asterisk", MessageBoxImage::Asterisk);
    NsVal("Stop", MessageBoxImage::Stop);
    NsVal("Error", MessageBoxImage::Error);
    NsVal("Warning", MessageBoxImage::Warning);
    NsVal("Information", MessageBoxImage::Information);
}
