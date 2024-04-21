////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __XAMLPLAYER_MAINWINDOW_H__
#define __XAMLPLAYER_MAINWINDOW_H__


#include <NsCore/Noesis.h>
#include <NsApp/Window.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/Error.h>
#include <NsGui/Uri.h>


namespace Noesis
{

class Border;
class ScaleTransform;
class TranslateTransform;
class CompositeTransform3D;
template<class T> class ObservableCollection;

}

namespace XamlPlayer
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// XAML Player Main Window code behind
////////////////////////////////////////////////////////////////////////////////////////////////////
class MainWindow final: public NoesisApp::Window
{
public:
    MainWindow();
    ~MainWindow();

    void LoadXAML(const Noesis::Uri& uri);
    void ClearErrors();

private:
    void SetContent(const Noesis::Uri& uri, BaseComponent* content);

    /// From UIElement
    //@{
    void OnPreviewMouseRightButtonDown(const Noesis::MouseButtonEventArgs& e) override;
    void OnPreviewMouseRightButtonUp(const Noesis::MouseButtonEventArgs& e) override;
    void OnPreviewMouseMove(const Noesis::MouseEventArgs& e) override;
    void OnPreviewMouseWheel(const Noesis::MouseWheelEventArgs& e) override;
    void OnPreviewKeyDown(const Noesis::KeyEventArgs& e) override;
    //@}

    /// From Window
    //@{
    void OnFileDropped(const char* filename) override;
    //@}

    static void ErrorHandler(const char* file, uint32_t line , const char* desc, bool fatal);
    void InitializeComponent();
    bool ConnectField(BaseComponent* object, const char* name) override;
    void UpdateTitle(const char* filename);
    bool IsFileLoaded() const;
    void Reset();

    void OnInitialized(BaseComponent* object, const Noesis::EventArgs& e);
    void OnLoaded(BaseComponent* object, const Noesis::RoutedEventArgs& e);

private:
    Noesis::Uri mActiveUri;

    Noesis::FrameworkElement* mRoot;
    Noesis::ItemsControl* mErrors;
    Noesis::Border* mContainer;
    Noesis::ScaleTransform* mContainerScale;
    Noesis::TranslateTransform* mContainerTranslate;
    Noesis::CompositeTransform3D* mTransform3D;

    Noesis::ErrorHandler mDefaultErrorHandler;
    Noesis::Ptr<Noesis::ObservableCollection<BaseComponent>> mErrorList;

    float mZoom;
    bool mDragging;
    bool mRotating;
    Noesis::Point mDraggingLastPosition;

    NS_DECLARE_REFLECTION(MainWindow, Window)
};

}

#endif
