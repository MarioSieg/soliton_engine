////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/Window.h>
#include <NsGui/UIElementData.h>
#include <NsGui/Canvas.h>
#include <NsGui/Brushes.h>
#include <NsGui/Path.h>
#include <NsGui/TextBlock.h>
#include <NsGui/MeshGeometry.h>
#include <NsGui/IView.h>
#include <NsGui/IRenderer.h>
#include <NsGui/Brushes.h>
#include <NsGui/SolidColorBrush.h>
#include <NsGui/FrameworkPropertyMetadata.h>
#include <NsGui/ResourceDictionary.h>
#include <NsGui/Keyboard.h>
#include <NsGui/KeyboardNavigation.h>
#include <NsGui/SizeChangedInfo.h>
#include <NsGui/Popup.h>
#include <NsGui/ToggleButton.h>
#include <NsGui/IntegrationAPI.h>
#include <NsGui/VisualTreeHelper.h>
#include <NsGui/Storyboard.h>
#include <NsGui/FocusManager.h>
#include <NsGui/ControlTemplate.h>
#include <NsGui/ContentPresenter.h>
#include <NsGui/DrawingContext.h>
#include <NsCore/Nullable.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>
#include <NsCore/CpuProfiler.h>
#include <NsRender/RenderContext.h>
#include <NsDrawing/Color.h>


using namespace Noesis;
using namespace NoesisApp;


enum WindowFlags
{
    WindowFlags_DontUpdateDisplay = 1 << 0,
    WindowFlags_DontUpdateWindow = 1 << 1
};

#define CHECK_FLAG(f) ((mWindowFlags & f) != 0)
#define SET_FLAG(f) (mWindowFlags |= f)
#define CLEAR_FLAG(f) (mWindowFlags &= ~f);

////////////////////////////////////////////////////////////////////////////////////////////////////
Window::Window(): mWindowFlags(0), mViewFlags(0), mWidth(0), mHeight(0), mBackgroundIsSolid(true),
    mActiveView(0), mRenderContext(0), mIsCtrlDown(false), mIsShiftDown(false),
    mStatsMode(StatsMode_Disabled), mToolbar(0), mWireframeBtn(0), mBatchesBtn(0), mOverdrawBtn(0),
    mPPAABtn(0), mStatsBtn(0), mWaitToHideToolbar(0), mCachedTitle(""), mStats(0)
{
    // Window does not use layout properties to measure or arrange
    IgnoreLayout(true);

#ifndef NS_PROFILE
    NS_UNUSED(mCachedTitle);
    NS_UNUSED(mStats);
    NS_UNUSED(mTextTitle);
    NS_UNUSED(mTextFps);
    NS_UNUSED(mTextMs);
    NS_UNUSED(mTextUpdate);
    NS_UNUSED(mTextRender);
    NS_UNUSED(mTextGPU);
    NS_UNUSED(mTextPrimitives);
    NS_UNUSED(mTextTriangles);
    NS_UNUSED(mTextNodes);
    NS_UNUSED(mTextUploads);
    NS_UNUSED(mTextMemory);
    NS_UNUSED(mGeoHistCPU);
    NS_UNUSED(mGeoHistGPU);
    NS_UNUSED(mHistSize);
    NS_UNUSED(mHistogramCPU);
    NS_UNUSED(mHistogramGPU);
    NS_UNUSED(mHistPos);
    NS_UNUSED(mLastRenderTime);
    NS_UNUSED(mNumFrames);
    NS_UNUSED(mUpdateTime);
    NS_UNUSED(mRenderTime);
    NS_UNUSED(mFrameTime);
    NS_UNUSED(mGPUTime);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Window::~Window()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::Init(Display* display, RenderContext* context, uint32_t samples, bool ppaa, bool lcd,
    bool emulateTouch)
{
    mDisplay.Reset(display);
    mSamples = samples;
    mRenderContext = context;
    mViewFlags = (ppaa ? RenderFlags_PPAA : 0) | (lcd ? RenderFlags_LCD : 0);

    // Set display properties
    mDisplay->SetTitle(GetTitle());
    mDisplay->SetWindowStyle(GetWindowStyle());
    mDisplay->SetWindowState(GetWindowState());
    mDisplay->SetWindowStartupLocation(GetWindowStartupLocation());
    mDisplay->SetResizeMode(GetResizeMode());
    mDisplay->SetShowInTaskbar(GetShowInTaskbar());
    mDisplay->SetTopmost(GetTopmost());
    mDisplay->SetAllowFileDrop(GetAllowDrop());

    // Location
    float left = GetLeft();
    float top = GetTop();
    if (!IsNaN(left) && !IsNaN(top))
    {
        mDisplay->SetLocation((int)left, (int)top);
    }

    // Size
    float width = GetWidth();
    float height = GetHeight();

    SizeToContent sizeToContent = GetSizeToContent();
    if (sizeToContent != SizeToContent_Manual)
    {
        Size contentSize = MeasureContent();
        if (sizeToContent != SizeToContent_Height)
        {
            width = contentSize.width;
        }
        if (sizeToContent != SizeToContent_Width)
        {
            height = contentSize.height;
        }
    }

    if (!IsNaN(width) && !IsNaN(height))
    {
        mDisplay->SetSize((uint32_t)width, (uint32_t)height);
    }

    // Create view
    mView = GUI::CreateView(this);
    mView->SetScale(mDisplay->GetScale());
    mView->SetTessellationMaxPixelError(TessellationMaxPixelError::HighQuality());
    mView->SetFlags(mViewFlags);
    mView->SetEmulateTouch(emulateTouch);
    mView->GetRenderer()->Init(context->GetDevice());
    mActiveView = mView;

    // Let's call SetClearColor before first resize because on some renderers like D3D12 the
    // clear color is part of the buffers and may require destroying and recreating
    mRenderContext->SetClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    mContextClearColor = mClearColor;

    // Override the template for doing optimized clear background
    Ptr<ControlTemplate> template_ = MakePtr<ControlTemplate>();
    template_->SetTargetType(TypeOf<Window>());
    template_->SetVisualTree(MakePtr<ContentPresenter>());
    SetTemplate(template_);

#ifdef NS_PROFILE
    // Enable manipulations (to swipe toolbar)
    SetIsManipulationEnabled(true);
#endif

    // Hook to display events
    mDisplay->LocationChanged() += MakeDelegate(this, &Window::OnDisplayLocationChanged);
    mDisplay->SizeChanged() += MakeDelegate(this, &Window::OnDisplaySizeChanged);
    mDisplay->ScaleChanged() += MakeDelegate(this, &Window::OnDisplayScaleChanged);
    mDisplay->FileDropped() += MakeDelegate(this, &Window::OnDisplayFileDropped);
    mDisplay->Activated() += MakeDelegate(this, &Window::OnDisplayActivated);
    mDisplay->Deactivated() += MakeDelegate(this, &Window::OnDisplayDeactivated);
    mDisplay->MouseMove() += MakeDelegate(this, &Window::OnDisplayMouseMove);
    mDisplay->MouseButtonDown() += MakeDelegate(this, &Window::OnDisplayMouseButtonDown);
    mDisplay->MouseButtonUp() += MakeDelegate(this, &Window::OnDisplayMouseButtonUp);
    mDisplay->MouseDoubleClick() += MakeDelegate(this, &Window::OnDisplayMouseDoubleClick);
    mDisplay->MouseWheel() += MakeDelegate(this, &Window::OnDisplayMouseWheel);
    mDisplay->MouseHWheel() += MakeDelegate(this, &Window::OnDisplayMouseHWheel);
    mDisplay->Scroll() += MakeDelegate(this, &Window::OnDisplayScroll);
    mDisplay->HScroll() += MakeDelegate(this, &Window::OnDisplayHScroll);
    mDisplay->TouchDown() += MakeDelegate(this, &Window::OnDisplayTouchDown);
    mDisplay->TouchMove() += MakeDelegate(this, &Window::OnDisplayTouchMove);
    mDisplay->TouchUp() += MakeDelegate(this, &Window::OnDisplayTouchUp);
    mDisplay->KeyDown() += MakeDelegate(this, &Window::OnDisplayKeyDown);
    mDisplay->KeyUp() += MakeDelegate(this, &Window::OnDisplayKeyUp);
    mDisplay->Char() += MakeDelegate(this, &Window::OnDisplayChar);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::ResetView()
{
    mView->GetRenderer()->Shutdown();
    mView.Reset();

    if (mViewStats != 0)
    {
        mViewStats->GetRenderer()->Shutdown();
        mViewStats.Reset();
    }

    mStatsMode = StatsMode_Disabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::RecreateView(RenderContext* context, bool emulateTouch)
{
    NS_ASSERT(mView == 0);

    context->SetClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    context->Resize();

    mView = GUI::CreateView(this);
    mView->SetScale(mDisplay->GetScale());
    mView->SetTessellationMaxPixelError(TessellationMaxPixelError::HighQuality());
    mView->SetEmulateTouch(emulateTouch);
    mView->SetFlags(mViewFlags);
    mView->SetSize(mDisplay->GetClientWidth(), mDisplay->GetClientHeight());
    mView->Update(0.0f);

    mView->GetRenderer()->Init(context->GetDevice());

    mActiveView = mView;
    mRenderContext = context;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Display* Window::GetDisplay() const
{
    return mDisplay;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::Render(double time)
{
    NS_PROFILE_CPU("App/RenderWindow");

    // Make sure display invoked OnDisplaySizeChanged() callback
    NS_ASSERT(!IsNaN(GetWidth()));
    NS_ASSERT(!IsNaN(GetHeight()));

    // Sync view flags (TCP changes)
    uint32_t viewFlags = mView->GetFlags();
    if (NS_UNLIKELY(mViewFlags != viewFlags))
    {
        mViewFlags = viewFlags;
        SyncViewFlags();
    }

    // Update view
  #ifdef NS_PROFILE
    UpdateStats(time);
  #endif

    mView->Update(time);

    // Render view
    {
        NS_PROFILE_CPU("App/BeginRender");
        mRenderContext->BeginRender();
    }

    IRenderer* renderer = mView->GetRenderer();
    renderer->UpdateRenderTree();
    renderer->RenderOffscreen();

    {
        NS_PROFILE_CPU("App/SetDefaultRenderTarget");

        bool hasDebugFlags = (viewFlags & (RenderFlags_Overdraw | RenderFlags_Wireframe |
            RenderFlags_ColorBatches)) > 0;
        bool doClearColor = mBackgroundIsSolid || hasDebugFlags;

        if (doClearColor)
        {
            SetClearColor(hasDebugFlags ? Color(0.0f, 0.0f, 0.0f, 1.0f) : mClearColor);
        }

        // Bind main render target
        uint32_t width = mDisplay->GetClientWidth();
        uint32_t height = mDisplay->GetClientHeight();
        mRenderContext->SetDefaultRenderTarget(width, height, doClearColor);
    }

    renderer->Render();

  #ifdef NS_PROFILE
    RenderStats(time);
  #endif

    {
        NS_PROFILE_CPU("App/EndRender");
        mRenderContext->EndRender();
    }

    // Swap buffers
    {
        NS_PROFILE_CPU("App/Swap");
        mRenderContext->Swap();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::Shutdown()
{
    mView->GetRenderer()->Shutdown();
    mView.Reset();

    if (mViewStats != 0)
    {
        mViewStats->GetRenderer()->Shutdown();
        mViewStats.Reset();
    }

    mClosed(this, EventArgs::Empty);

    if (mDisplay != 0)
    {
        mDisplay->LocationChanged() -= MakeDelegate(this, &Window::OnDisplayLocationChanged);
        mDisplay->SizeChanged() -= MakeDelegate(this, &Window::OnDisplaySizeChanged);
        mDisplay->ScaleChanged() -= MakeDelegate(this, &Window::OnDisplayScaleChanged);
        mDisplay->FileDropped() -= MakeDelegate(this, &Window::OnDisplayFileDropped);
        mDisplay->Activated() -= MakeDelegate(this, &Window::OnDisplayActivated);
        mDisplay->Deactivated() -= MakeDelegate(this, &Window::OnDisplayDeactivated);
        mDisplay->MouseMove() -= MakeDelegate(this, &Window::OnDisplayMouseMove);
        mDisplay->MouseButtonDown() -= MakeDelegate(this, &Window::OnDisplayMouseButtonDown);
        mDisplay->MouseButtonUp() -= MakeDelegate(this, &Window::OnDisplayMouseButtonUp);
        mDisplay->MouseDoubleClick() -= MakeDelegate(this, &Window::OnDisplayMouseDoubleClick);
        mDisplay->MouseWheel() -= MakeDelegate(this, &Window::OnDisplayMouseWheel);
        mDisplay->MouseHWheel() -= MakeDelegate(this, &Window::OnDisplayMouseHWheel);
        mDisplay->Scroll() -= MakeDelegate(this, &Window::OnDisplayScroll);
        mDisplay->HScroll() -= MakeDelegate(this, &Window::OnDisplayHScroll);
        mDisplay->TouchDown() -= MakeDelegate(this, &Window::OnDisplayTouchDown);
        mDisplay->TouchMove() -= MakeDelegate(this, &Window::OnDisplayTouchMove);
        mDisplay->TouchUp() -= MakeDelegate(this, &Window::OnDisplayTouchUp);
        mDisplay->KeyDown() -= MakeDelegate(this, &Window::OnDisplayKeyDown);
        mDisplay->KeyUp() -= MakeDelegate(this, &Window::OnDisplayKeyUp);
        mDisplay->Char() -= MakeDelegate(this, &Window::OnDisplayChar);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::Close()
{
    if (mDisplay != 0)
    {
        mDisplay->Close();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Window::GetAllowsTransparency() const
{
    return GetValue<bool>(AllowsTransparencyProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetAllowsTransparency(bool allowsTransparency)
{
    SetValue<bool>(AllowsTransparencyProperty, allowsTransparency);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Window::GetFullscreen() const
{
    return GetValue<bool>(FullscreenProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetFullscreen(bool fullscreen)
{
    SetValue<bool>(FullscreenProperty, fullscreen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Window::GetIsActive() const
{
    return GetValue<bool>(IsActiveProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetIsActive(bool value)
{
    SetValue<bool>(IsActiveProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float Window::GetLeft() const
{
    return GetValue<float>(LeftProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetLeft(float value)
{
    SetValue<float>(LeftProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ResizeMode Window::GetResizeMode() const
{
    return GetValue<ResizeMode>(ResizeModeProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetResizeMode(ResizeMode value)
{
    SetValue<ResizeMode>(ResizeModeProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Window::GetShowInTaskbar() const
{
    return GetValue<bool>(ShowInTaskbarProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetShowInTaskbar(bool value)
{
    SetValue<bool>(ShowInTaskbarProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
SizeToContent Window::GetSizeToContent() const
{
    return GetValue<SizeToContent>(SizeToContentProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetSizeToContent(SizeToContent value)
{
    SetValue<SizeToContent>(SizeToContentProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* Window::GetTitle() const
{
    return GetValue<String>(TitleProperty).Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetTitle(const char* value)
{
    SetValue<String>(TitleProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float Window::GetTop() const
{
    return GetValue<float>(TopProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetTop(float value)
{
    SetValue<float>(TopProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Window::GetTopmost() const
{
    return GetValue<bool>(TopmostProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetTopmost(bool value)
{
    SetValue<bool>(TopmostProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
WindowState Window::GetWindowState() const
{
    return GetValue<WindowState>(WindowStateProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetWindowState(WindowState value)
{
    SetValue<WindowState>(WindowStateProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
WindowStyle Window::GetWindowStyle() const
{
    return GetValue<WindowStyle>(WindowStyleProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetWindowStyle(WindowStyle value)
{
    SetValue<WindowStyle>(WindowStyleProperty, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
WindowStartupLocation Window::GetWindowStartupLocation() const
{
    return GetValue<WindowStartupLocation>(WindowStartupLocationProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetWindowStartupLocation(WindowStartupLocation location)
{
    SetValue<WindowStartupLocation>(WindowStartupLocationProperty, location);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
EventHandler& Window::Activated()
{
    return mActivated;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
EventHandler& Window::Deactivated()
{
    return mDeactivated;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
EventHandler& Window::Closed()
{
    return mClosed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
EventHandler& Window::LocationChanged()
{
    return mLocationChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
EventHandler& Window::StateChanged()
{
    return mStateChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnFullscreenChanged(bool)
{
    CoerceValue<WindowStyle>(WindowStyleProperty);
    CoerceValue<ResizeMode>(ResizeModeProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnTitleChanged(const char* title)
{
    if (!CHECK_FLAG(WindowFlags_DontUpdateDisplay))
    {
        SET_FLAG(WindowFlags_DontUpdateWindow);
        if (mDisplay != 0)
        {
          #ifdef NS_PROFILE
            StrCopy(mCachedTitle, sizeof(mCachedTitle), title);
          #endif
            mDisplay->SetTitle(title);
        }
        CLEAR_FLAG(WindowFlags_DontUpdateWindow);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnLeftChanged(float left)
{
    if (!CHECK_FLAG(WindowFlags_DontUpdateDisplay))
    {
        SET_FLAG(WindowFlags_DontUpdateWindow);
        if (mDisplay != 0)
        {
            mDisplay->SetLocation((int)left, (int)GetTop());
        }
        CLEAR_FLAG(WindowFlags_DontUpdateWindow);
    }

    mLocationChanged(this, EventArgs::Empty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnTopChanged(float top)
{
    if (!CHECK_FLAG(WindowFlags_DontUpdateDisplay))
    {
        SET_FLAG(WindowFlags_DontUpdateWindow);
        if (mDisplay != 0)
        {
            mDisplay->SetLocation((int)GetLeft(), (int)top);
        }
        CLEAR_FLAG(WindowFlags_DontUpdateWindow);
    }

    mLocationChanged(this, EventArgs::Empty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnWindowStyleChanged(WindowStyle windowStyle)
{
    if (!CHECK_FLAG(WindowFlags_DontUpdateDisplay))
    {
        SET_FLAG(WindowFlags_DontUpdateWindow);
        if (mDisplay != 0)
        {
            mDisplay->SetWindowStyle(windowStyle);
        }
        CLEAR_FLAG(WindowFlags_DontUpdateWindow);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnStateChanged(WindowState windowState)
{
    if (!CHECK_FLAG(WindowFlags_DontUpdateDisplay))
    {
        SET_FLAG(WindowFlags_DontUpdateWindow);
        if (mDisplay != 0)
        {
            mDisplay->SetWindowState(windowState);
        }
        CLEAR_FLAG(WindowFlags_DontUpdateWindow);
    }

    mStateChanged(this, EventArgs::Empty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnResizeModeChanged(ResizeMode resizeMode)
{
    if (!CHECK_FLAG(WindowFlags_DontUpdateDisplay))
    {
        SET_FLAG(WindowFlags_DontUpdateWindow);
        if (mDisplay != 0)
        {
            mDisplay->SetResizeMode(resizeMode);
        }
        CLEAR_FLAG(WindowFlags_DontUpdateWindow);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnShowInTaskbarChanged(bool showInTaskbar)
{
    if (!CHECK_FLAG(WindowFlags_DontUpdateDisplay))
    {
        SET_FLAG(WindowFlags_DontUpdateWindow);
        if (mDisplay != 0)
        {
            mDisplay->SetShowInTaskbar(showInTaskbar);
        }
        CLEAR_FLAG(WindowFlags_DontUpdateWindow);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnTopmostChanged(bool topmost)
{
    if (!CHECK_FLAG(WindowFlags_DontUpdateDisplay))
    {
        SET_FLAG(WindowFlags_DontUpdateWindow);
        if (mDisplay != 0)
        {
            mDisplay->SetTopmost(topmost);
        }
        CLEAR_FLAG(WindowFlags_DontUpdateWindow);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnFileDropped(const char*)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::PreviewToolbar()
{
#ifdef NS_PROFILE
    // Shows Toolbar and enables it
    ShowToolbar(true);

    // Waits 2 seconds to allow user to interact with the Toolbar
    mWaitToHideToolbar->Begin(mToolbar);

    // If no interaction occurred, we hide the Toolbar
    mWaitToHideToolbar->Completed() += MakeDelegate(this, &Window::HideToolbar);

    // Detect if user interacted with the Toolbar
    mToolbar->MouseEnter() += MakeDelegate(this, &Window::OnToolbarMouseInteraction);
    mToolbar->TouchEnter() += MakeDelegate(this, &Window::OnToolbarTouchInteraction);
    mToolbar->PreviewKeyDown() += MakeDelegate(this, &Window::OnToolbarKeyInteraction);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Size Window::MeasureContent()
{
    // Make sure window is already initialized and template is applied
    InitComponent(this, true);
    ApplyTemplate();

    // Measure window content
    UIElement* child = DynamicCast<UIElement*>(GetSingleVisualChild());
    if (child != 0)
    {
        child->Measure(Size(FLT_INF, FLT_INF));

        Size desiredSize = child->GetDesiredSize();

        uint32_t width = (uint32_t)Ceil(desiredSize.width);
        uint32_t height = (uint32_t)Ceil(desiredSize.height);
        mDisplay->AdjustWindowSize(width, height);

        return Size((float)width, (float)height);
    }

    return Size::Zero();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::HideToolbar(BaseComponent*, const TimelineEventArgs&)
{
    mWaitToHideToolbar->Completed() -= MakeDelegate(this, &Window::HideToolbar);
    ShowToolbar(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnToolbarMouseInteraction(BaseComponent*, const MouseEventArgs&)
{
    mToolbar->MouseEnter() -= MakeDelegate(this, &Window::OnToolbarMouseInteraction);
    mWaitToHideToolbar->Completed() -= MakeDelegate(this, &Window::HideToolbar);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnToolbarTouchInteraction(BaseComponent*, const TouchEventArgs&)
{
    mToolbar->TouchEnter() -= MakeDelegate(this, &Window::OnToolbarTouchInteraction);
    mWaitToHideToolbar->Completed() -= MakeDelegate(this, &Window::HideToolbar);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnToolbarKeyInteraction(BaseComponent*, const KeyEventArgs&)
{
    mToolbar->PreviewKeyDown() -= MakeDelegate(this, &Window::OnToolbarKeyInteraction);
    mWaitToHideToolbar->Completed() -= MakeDelegate(this, &Window::HideToolbar);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Window::OnPropertyChanged(const DependencyPropertyChangedEventArgs& args)
{
    bool handled = ParentClass::OnPropertyChanged(args);

    if (!handled)
    {
        if (args.prop == FullscreenProperty)
        {
            OnFullscreenChanged(args.NewValue<bool>());
            return true;
        }
        if (args.prop == TitleProperty)
        {
            OnTitleChanged(args.NewValue<String>().Str());
            return true;
        }
        if (args.prop == LeftProperty)
        {
            OnLeftChanged(args.NewValue<float>());
            return true;
        }
        if (args.prop == TopProperty)
        {
            OnTopChanged(args.NewValue<float>());
            return true;
        }
        if (args.prop == WindowStyleProperty)
        {
            OnWindowStyleChanged(args.NewValue<WindowStyle>());
            return true;
        }
        if (args.prop == WindowStateProperty)
        {
            OnStateChanged(args.NewValue<WindowState>());
            return true;
        }
        if (args.prop == ResizeModeProperty)
        {
            OnResizeModeChanged(args.NewValue<ResizeMode>());
            return true;
        }
        if (args.prop == ShowInTaskbarProperty)
        {
            OnShowInTaskbarChanged(args.NewValue<bool>());
            return true;
        }
        if (args.prop == TopmostProperty)
        {
            OnTopmostChanged(args.NewValue<bool>());
            return true;
        }
    }

    return handled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnWidthChanged(float width)
{
    ParentClass::OnWidthChanged(width);

    if (!CHECK_FLAG(WindowFlags_DontUpdateDisplay))
    {
        SET_FLAG(WindowFlags_DontUpdateWindow);
        if (mDisplay != 0)
        {
            mDisplay->SetSize((uint32_t)width, (uint32_t)GetHeight());
        }
        CLEAR_FLAG(WindowFlags_DontUpdateWindow);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnHeightChanged(float height)
{
    ParentClass::OnHeightChanged(height);

    if (!CHECK_FLAG(WindowFlags_DontUpdateDisplay))
    {
        SET_FLAG(WindowFlags_DontUpdateWindow);
        if (mDisplay != 0)
        {
            mDisplay->SetSize((uint32_t)GetWidth(), (uint32_t)height);
        }
        CLEAR_FLAG(WindowFlags_DontUpdateWindow);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Size Window::MeasureOverride(const Size& availableSize)
{
    if (mDisplay != 0)
    {
        float scale = mDisplay->GetScale();
        float clientW = (float)mDisplay->GetClientWidth() / scale;
        float clientH = (float)mDisplay->GetClientHeight() / scale;

        SizeToContent sizeToContent = GetSizeToContent();
        if (sizeToContent == SizeToContent_Manual)
        {
            ParentClass::MeasureOverride(Size(clientW, clientH));
            return Sizei((uint32_t)GetWidth(), (uint32_t)GetHeight());
        }
        else
        {
            // calculate content desired size
            Size constraint(
                sizeToContent == SizeToContent_Height ? clientW : FLT_INF,
                sizeToContent == SizeToContent_Width ? clientH : FLT_INF);

            Size desiredSize = ParentClass::MeasureOverride(constraint);

            // adjust content size to window size (including borders)
            uint32_t desiredWidth = (uint32_t)Ceil(desiredSize.width);
            uint32_t desiredHeight = (uint32_t)Ceil(desiredSize.height);
            mDisplay->AdjustWindowSize(desiredWidth, desiredHeight);

            // update window to the desired size
            if (sizeToContent != SizeToContent_Height)
            {
                SetWidth((float)desiredWidth);
            }
            else
            {
                desiredWidth = (uint32_t)GetWidth();
            }

            if (sizeToContent != SizeToContent_Width)
            {
                SetHeight((float)desiredHeight);
            }
            else
            {
                desiredHeight = (uint32_t)GetHeight();
            }

            return Sizei(desiredWidth, desiredHeight);
        }
    }

    return ParentClass::MeasureOverride(availableSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Size Window::ArrangeOverride(const Size& finalSize)
{
    if (mDisplay != 0)
    {
        float scale = mDisplay->GetScale();
        float clientW = (float)mDisplay->GetClientWidth() / scale;
        float clientH = (float)mDisplay->GetClientHeight() / scale;

        ParentClass::ArrangeOverride(Size(clientW, clientH));
        return Sizei((uint32_t)GetWidth(), (uint32_t)GetHeight());
    }

    return ParentClass::ArrangeOverride(finalSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnManipulationStarted(const ManipulationStartedEventArgs& e)
{
    if (e.source == this)
    {
        if (mDisplay == 0 || e.manipulationOrigin.x > mDisplay->GetClientWidth() * 0.05f)
        {
            // Finger was touching outside our swipe area (10% of left screen)
            e.cancel = true;
        }

        e.handled = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnManipulationInertiaStarting(const ManipulationInertiaStartingEventArgs& e)
{
    if (e.source == this)
    {
        if (mDisplay != 0 && (mToolbar == 0 || mToolbar->GetIsEnabled() == false))
        {
            // Detect if user moved his finger upwards
            if (e.initialVelocities.linearVelocity.x > 0.5f)
            {
                ShowToolbar(true);
            }
        }

        e.handled = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnRender(DrawingContext* context)
{
    // If the background is a solid brush it is rendered using the render context clear color
    if (mRenderContext && !mBackgroundIsSolid)
    {
        context->DrawRectangle(GetBackground(), 0, Rect(mRenderSize));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::HitTestToolbar(int x, int y)
{
    if (mToolbar != 0 && mToolbar->GetIsEnabled() == true)
    {
        Pointi p(x ,y);
        if (VisualTreeHelper::HitTest(mToolbar, mToolbar->PointFromScreen(p)).visualHit == 0)
        {
            ShowToolbar(false);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::ShowToolbar(bool visible)
{
    NS_UNUSED(visible);

#ifdef NS_PROFILE
    LoadOverlayXAML();
    NS_ASSERT(mToolbar != 0);

    if (visible)
    {
        mToolbar->SetIsEnabled(true);
        mWireframeBtn->Focus();
        mActiveView = mViewStats;
        SyncViewFlags();
    }
    else
    {
        mToolbar->SetIsEnabled(false);
        mActiveView = mView;
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::EnablePPAA(BaseComponent*, const RoutedEventArgs&)
{
    mViewFlags |= RenderFlags_PPAA;
    SyncViewFlags();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::DisablePPAA(BaseComponent*, const RoutedEventArgs&)
{
    mViewFlags &= ~RenderFlags_PPAA;
    SyncViewFlags();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::EnableWireframe(BaseComponent*, const RoutedEventArgs&)
{
    mViewFlags |= RenderFlags_Wireframe;
    SyncViewFlags();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::DisableWireframe(BaseComponent*, const RoutedEventArgs&)
{
    mViewFlags &= ~RenderFlags_Wireframe;
    SyncViewFlags();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Window::EnableBatches(BaseComponent*, const RoutedEventArgs&)
{
    mViewFlags |= RenderFlags_ColorBatches;
    SyncViewFlags();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Window::DisableBatches(BaseComponent*, const RoutedEventArgs&)
{
    mViewFlags &= ~RenderFlags_ColorBatches;
    SyncViewFlags();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Window::EnableOverdraw(BaseComponent*, const RoutedEventArgs&)
{
    mViewFlags |= RenderFlags_Overdraw;
    SyncViewFlags();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Window::DisableOverdraw(BaseComponent*, const RoutedEventArgs&)
{
    mViewFlags &= ~RenderFlags_Overdraw;
    SyncViewFlags();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Window::EnableTitleStats(BaseComponent*, const RoutedEventArgs&)
{
#ifdef NS_PROFILE
    mStatsMode = StatsMode_TitleBar;

    if (mStats != 0)
    {
        mStats->SetVisibility(Visibility_Hidden);
    }

    if (mStatsBtn != 0)
    {
        mStatsBtn->SetIsChecked(false);
    }

    mLastRenderTime = 0.0;
    mNumFrames = 0;
    mUpdateTime = 0.0f; 
    mRenderTime = 0.0f;
    mFrameTime = 0.0f;
    mGPUTime = 0.0f;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Window::EnableWidgetStats(BaseComponent*, const RoutedEventArgs&)
{
#ifdef NS_PROFILE
    mStatsMode = StatsMode_Widget;

    LoadOverlayXAML();
    mStats->SetVisibility(Visibility_Visible);
    mStatsBtn->SetIsChecked(true);

    mHistPos = 0;
    memset(mHistogramCPU, 0, sizeof(mHistogramCPU));
    memset(mHistogramGPU, 0, sizeof(mHistogramGPU));

    mLastRenderTime = 0.0;
    mNumFrames = 0;
    mUpdateTime = 0.0f; 
    mRenderTime = 0.0f;
    mFrameTime = 0.0f;
    mGPUTime = 0.0f;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Window::DisableStats(BaseComponent*, const RoutedEventArgs&)
{
#ifdef NS_PROFILE
    mStatsMode = StatsMode_Disabled;

    if (mDisplay != 0)
    {
        mDisplay->SetTitle(mCachedTitle);
    }

    if (mStats != 0)
    {
        mStats->SetVisibility(Visibility_Hidden);
    }

    if (mStatsBtn != 0)
    {
        mStatsBtn->SetIsChecked(false);
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SyncViewFlags()
{
    mView->SetFlags(mViewFlags);

    if (mWireframeBtn != 0)
    {
        mWireframeBtn->SetIsChecked((mViewFlags & RenderFlags_Wireframe) > 0);
    }

    if (mBatchesBtn != 0)
    {
        mBatchesBtn->SetIsChecked((mViewFlags & RenderFlags_ColorBatches) > 0);
    }

    if (mOverdrawBtn != 0)
    {
        mOverdrawBtn->SetIsChecked((mViewFlags & RenderFlags_Overdraw) > 0);
    }

    if (mPPAABtn != 0)
    {
        mPPAABtn->SetIsChecked((mViewFlags & RenderFlags_PPAA) > 0);
    }

    if (mStatsBtn != 0)
    {
        mStatsBtn->SetIsChecked(mStatsMode != StatsMode_Disabled);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Window::LoadOverlayXAML()
{
#ifdef NS_PROFILE
    if (mViewStats == 0)
    {
        Uri uri = Uri::Pack("App", "StatsOverlay.xaml");
        Ptr<FrameworkElement> xaml = GUI::LoadXaml<FrameworkElement>(uri);

        mViewStats = GUI::CreateView(xaml);
        mViewStats->SetScale(mDisplay->GetScale());
        mViewStats->SetSize(mDisplay->GetClientWidth(), mDisplay->GetClientHeight());
        mViewStats->GetRenderer()->Init(mRenderContext->GetDevice());
        mViewStats->SetFlags((mViewFlags & RenderFlags_LCD) > 0 ? RenderFlags_LCD : 0);

        mStats = xaml->FindName<FrameworkElement>("stats");
        NS_ASSERT(mStats != 0);

        mToolbar = xaml->FindName<FrameworkElement>("toolbar");
        NS_ASSERT(mToolbar != 0);
        mToolbar->SetIsEnabled(false);

        mWireframeBtn = xaml->FindName<ToggleButton>("wireframeBtn");
        NS_ASSERT(mWireframeBtn != 0);
        mWireframeBtn->Checked() += MakeDelegate(this, &Window::EnableWireframe);
        mWireframeBtn->Unchecked() += MakeDelegate(this, &Window::DisableWireframe);

        mBatchesBtn = xaml->FindName<ToggleButton>("batchesBtn");
        NS_ASSERT(mBatchesBtn != 0);
        mBatchesBtn->Checked() += MakeDelegate(this, &Window::EnableBatches);
        mBatchesBtn->Unchecked() += MakeDelegate(this, &Window::DisableBatches);

        mOverdrawBtn = xaml->FindName<ToggleButton>("overdrawBtn");
        NS_ASSERT(mOverdrawBtn != 0);
        mOverdrawBtn->Checked() += MakeDelegate(this, &Window::EnableOverdraw);
        mOverdrawBtn->Unchecked() += MakeDelegate(this, &Window::DisableOverdraw);

        mPPAABtn = xaml->FindName<ToggleButton>("ppaaBtn");
        NS_ASSERT(mPPAABtn != 0);
        mPPAABtn->Checked() += MakeDelegate(this, &Window::EnablePPAA);
        mPPAABtn->Unchecked() += MakeDelegate(this, &Window::DisablePPAA);

        mStatsBtn = xaml->FindName<ToggleButton>("statsBtn");
        NS_ASSERT(mStatsBtn != 0);
        mStatsBtn->Checked() += MakeDelegate(this, &Window::EnableWidgetStats);
        mStatsBtn->Indeterminate() += MakeDelegate(this, &Window::EnableTitleStats);
        mStatsBtn->Unchecked() += MakeDelegate(this, &Window::DisableStats);

#if defined(NS_PLATFORM_WINDOWS_DESKTOP) || defined(NS_PLATFORM_OSX) || defined(NS_PLATFORM_LINUX)
        mWireframeBtn->SetTag("WIREFRAME\n\nCTRL + W");
        mBatchesBtn->SetTag("BATCHES\n\nCTRL + B");
        mOverdrawBtn->SetTag("OVERDRAW\n\nCTRL + O");
        mPPAABtn->SetTag("PPAA\n\nCTRL + P");
        mStatsBtn->SetTag("STATS\n\nCTRL + F");
#endif

        mWaitToHideToolbar = (Storyboard*)xaml->GetResources()->Get("WaitToHideToolbar");
        NS_ASSERT(mWaitToHideToolbar != 0);

        mTextTitle = xaml->FindName<TextBlock>("title");
        mTextFps = xaml->FindName<TextBlock>("fps");
        mTextMs = xaml->FindName<TextBlock>("ms");
        mTextUpdate = xaml->FindName<TextBlock>("update");
        mTextRender = xaml->FindName<TextBlock>("render");
        mTextGPU = xaml->FindName<TextBlock>("gpu");
        mTextPrimitives = xaml->FindName<TextBlock>("primitives");
        mTextTriangles = xaml->FindName<TextBlock>("triangles");
        mTextNodes = xaml->FindName<TextBlock>("nodes");
        mTextUploads = xaml->FindName<TextBlock>("uploads");
        mTextMemory = xaml->FindName<TextBlock>("memory");

        NS_ASSERT(mTextTitle != 0);
        NS_ASSERT(mTextFps != 0);
        NS_ASSERT(mTextMs != 0);
        NS_ASSERT(mTextUpdate != 0);
        NS_ASSERT(mTextRender != 0);
        NS_ASSERT(mTextGPU != 0);
        NS_ASSERT(mTextPrimitives != 0);
        NS_ASSERT(mTextTriangles != 0);
        NS_ASSERT(mTextNodes != 0);
        NS_ASSERT(mTextUploads != 0);
        NS_ASSERT(mTextMemory != 0);

        Path* pathHistCPU = xaml->FindName<Path>("histCPU");
        NS_ASSERT(pathHistCPU != 0);
        Ptr<MeshGeometry> geoHistCPU = *new MeshGeometry();
        pathHistCPU->SetData(geoHistCPU);
        mGeoHistCPU = geoHistCPU;

        Path* pathHistGPU = xaml->FindName<Path>("histGPU");
        NS_ASSERT(pathHistGPU != 0);
        Ptr<MeshGeometry> geoHistGPU = *new MeshGeometry();
        pathHistGPU->SetData(geoHistGPU);
        mGeoHistGPU = geoHistGPU;

        FrameworkElement* histBox = xaml->FindName<FrameworkElement>("histBox");
        NS_ASSERT(histBox != 0);
        mHistSize.width = histBox->GetWidth();
        NS_ASSERT(!IsNaN(mHistSize.width));
        mHistSize.height = histBox->GetHeight();
        NS_ASSERT(!IsNaN(mHistSize.height));
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Window::UpdateHistogram(uint16_t* data, float maxF, MeshGeometry* geometry) const
{
    NS_UNUSED(data, maxF, geometry);

#ifdef NS_PROFILE
    geometry->SetBounds(Rect(mHistSize));

    float stepX = mHistSize.width / (NS_COUNTOF(mHistogramCPU) - 1);
    float baseH = mHistSize.height;
    float minH = baseH - 1.0f;

    geometry->SetNumVertices(2 * NS_COUNTOF(mHistogramCPU));
    Point* vertices = geometry->GetVertices();

    for (uint32_t i = 0; i < NS_COUNTOF(mHistogramCPU); i++)
    {
        float x = i * stepX;

        uint16_t f = data[(mHistPos + i) % NS_COUNTOF(mHistogramCPU)];
        float h = minH - (f * minH / maxF);

        vertices[2 * i] = Point(x, baseH);
        vertices[2 * i + 1] = Point(x, h);
    }

    geometry->SetNumIndices(6 * (NS_COUNTOF(mHistogramCPU) - 1));
    uint16_t* indices = geometry->GetIndices();

    for (uint16_t i = 0; i < NS_COUNTOF(mHistogramCPU) - 1; i++)
    {
        indices[6 * i] = 2 * i;
        indices[6 * i + 1] = 2 * i + 1;
        indices[6 * i + 2] = 2 * i + 2;

        indices[6 * i + 3] = 2 * i + 2;
        indices[6 * i + 4] = 2 * i + 1;
        indices[6 * i + 5] = 2 * i + 3;
    }

    geometry->Update();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::UpdateStats(double time)
{
    NS_UNUSED(time);

#ifdef NS_PROFILE
    if (mStatsMode != StatsMode_Disabled)
    {
        ViewStats stats = mView->GetStats();
        mUpdateTime += stats.updateTime;
        mRenderTime += stats.renderTime;
        mFrameTime += stats.frameTime;
        mGPUTime += mRenderContext->GetGPUTimeMs();
        mNumFrames++;

        float dt = (float)(time - mLastRenderTime);

        if (dt > 0.5f)
        {
            float update = mUpdateTime / mNumFrames;
            float render = mRenderTime / mNumFrames;
            float frame = mFrameTime / mNumFrames;
            float gpu = mGPUTime / mNumFrames;
            float fps = IsZero(frame) ? 0.0f : 1000.0f / frame;

            mLastRenderTime = time;
            mNumFrames = 0;
            mUpdateTime = 0.0f;
            mRenderTime = 0.0f;
            mFrameTime = 0.0f;
            mGPUTime = 0.0f;

            if (mStatsMode == StatsMode_TitleBar)
            {
                char title[512];
                snprintf(title, sizeof(title), "%s [%.0f fps %.2f ms (%.2f %.2f %.2f)]",
                    mCachedTitle, fps, frame, update, render, gpu);
                mDisplay->SetTitle(title);
            }
            else
            {
                char buffer[64];
                snprintf(buffer, 64, "%s - %ux%u - %dx", mRenderContext->Description(),
                    mDisplay->GetClientWidth(), mDisplay->GetClientHeight(), mSamples);
                StrAppend(buffer, 64, (mViewFlags & RenderFlags_PPAA) > 0 ? " - PPAA" : "");
                mTextTitle->SetText(buffer);

                snprintf(buffer, 64, "%.0f fps", fps);
                mTextFps->SetText(buffer);

                snprintf(buffer, 64, "%.2f ms", frame);
                mTextMs->SetText(buffer);

                snprintf(buffer, 64, "%5.2f ms", update);
                mTextUpdate->SetText(buffer);

                snprintf(buffer, 64, "%5.2f ms", render);
                mTextRender->SetText(buffer);

                snprintf(buffer, 64, "%5.2f ms", gpu);
                mTextGPU->SetText(buffer);

                snprintf(buffer, 64, "Tris: %6d  Draws: %6d  Batches: %5d", stats.triangles,
                    stats.draws, stats.batches);
                mTextTriangles->SetText(buffer);

                snprintf(buffer, 64, "Tess: %6d  Flushes: %4d  Size: %5d KB", stats.tessellations,
                    stats.flushes, stats.geometrySize / 1024);
                mTextPrimitives->SetText(buffer);

                snprintf(buffer, 64, "Masks: %5d  Tiles: %6d  SetRTs: %6d", stats.masks,
                    stats.opacities, stats.renderTargetSwitches);
                mTextNodes->SetText(buffer);

                snprintf(buffer, 64, "Ramps^ %5d  Glyphs^ %5d  Discards: %4d", stats.uploadedRamps,
                    stats.rasterizedGlyphs, stats.discardedGlyphTiles);
                mTextUploads->SetText(buffer);

                float mem = GetAllocatedMemory() / (1024.0f * 1024.0f);
                uint32_t allocsAccum = GetAllocationsCount();

                snprintf(buffer, 64, "Memory %.2f MB (%d allocs)", mem, allocsAccum);
                mTextMemory->SetText(buffer);

                static_assert(NS_COUNTOF(mHistogramCPU) == NS_COUNTOF(mHistogramGPU), "array size");
                mHistogramCPU[mHistPos] = (uint16_t)(frame * 100.0f);
                mHistogramGPU[mHistPos] = (uint16_t)(gpu * 100.0f);
                mHistPos = (mHistPos + 1) % NS_COUNTOF(mHistogramCPU);

                float maxF = 0;
                for (uint32_t i = 0; i < NS_COUNTOF(mHistogramCPU); i++)
                {
                    float cpuF = mHistogramCPU[i];
                    float gpuF = mHistogramGPU[i];
                    maxF = Max(Max(maxF, cpuF), gpuF);
                }

                maxF = maxF > 0.0f ? (maxF * 110.0f) / 100.0f : 1.0f;

                UpdateHistogram(mHistogramCPU, maxF, mGeoHistCPU);
                UpdateHistogram(mHistogramGPU, maxF, mGeoHistGPU);
            }
        }
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::RenderStats(double time)
{
    NS_UNUSED(time);

#ifdef NS_PROFILE
    if (mViewStats != 0)
    {
        mViewStats->Update(time);

        IRenderer* renderer = mViewStats->GetRenderer();
        renderer->UpdateRenderTree();
        bool needsOffscreen = renderer->RenderOffscreen();
        NS_ASSERT(!needsOffscreen);
        renderer->Render();
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayLocationChanged(Display*, int x, int y)
{
    if (!CHECK_FLAG(WindowFlags_DontUpdateWindow))
    {
        SET_FLAG(WindowFlags_DontUpdateDisplay);
        SetLeft((float)x);
        SetTop((float)y);
        CLEAR_FLAG(WindowFlags_DontUpdateDisplay);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplaySizeChanged(Display*, uint32_t width, uint32_t height)
{
    if (width != mWidth || height != mHeight)
    {
        mWidth = width;
        mHeight = height;

        if (!CHECK_FLAG(WindowFlags_DontUpdateWindow))
        {
            // Manually changing window size will override SizeToContent initial value
            SizeToContent sizeToContent = GetSizeToContent();
            if (sizeToContent != SizeToContent_Manual && !NeverMeasured() && !NeverArranged())
            {
                if (sizeToContent != SizeToContent_Height && width != GetWidth())
                {
                    SetSizeToContent(sizeToContent == SizeToContent_WidthAndHeight ?
                        SizeToContent_Height : SizeToContent_Manual);
                }
                else if (sizeToContent != SizeToContent_Width && height != GetHeight())
                {
                    SetSizeToContent(sizeToContent == SizeToContent_WidthAndHeight ?
                        SizeToContent_Width : SizeToContent_Manual);
                }
            }

            SET_FLAG(WindowFlags_DontUpdateDisplay);
            SetWidth((float)width);
            SetHeight((float)height);
            CLEAR_FLAG(WindowFlags_DontUpdateDisplay);
        }

        if (mView != 0)
        {
            NS_ASSERT(mRenderContext != 0);
            mRenderContext->Resize();

            NS_ASSERT(mDisplay != 0);
            uint32_t cw = mDisplay->GetClientWidth();
            uint32_t ch = mDisplay->GetClientHeight();

            mView->SetSize(cw, ch);

            if (mViewStats != 0)
            {
                mViewStats->SetSize(cw, ch);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayScaleChanged(Display*, float scale)
{
    if (mView != 0)
    {
        mView->SetScale(scale);

        if (mViewStats != 0)
        {
            mViewStats->SetScale(scale);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayFileDropped(Display*, const char* filename)
{
    OnFileDropped(filename);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayActivated(Display*)
{
    mView->Activate();
    SetReadOnlyProperty<bool>(IsActiveProperty, true);
    mActivated(this, EventArgs::Empty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayDeactivated(Display*)
{
    mView->Deactivate();
    mIsCtrlDown = false;
    mIsShiftDown = false;
    SetReadOnlyProperty<bool>(IsActiveProperty, false);
    mDeactivated(this, EventArgs::Empty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayMouseMove(Display*, int x, int y)
{
    mActiveView->MouseMove(x, y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayMouseButtonDown(Display*, int x, int y, MouseButton button)
{
    HitTestToolbar(x, y);
    mActiveView->MouseButtonDown(x, y, button);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayMouseButtonUp(Display*, int x, int y, MouseButton button)
{
    mActiveView->MouseButtonUp(x, y, button);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayMouseDoubleClick(Display*, int x, int y, MouseButton button)
{
    mActiveView->MouseDoubleClick(x, y, button);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayMouseWheel(Display*, int x, int y, int delta)
{
    mActiveView->MouseWheel(x, y, delta);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayMouseHWheel(Display*, int x, int y, int delta)
{
    mActiveView->MouseHWheel(x, y, delta);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayScroll(Display*, float value)
{
    mActiveView->Scroll(value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayHScroll(Display*, float value)
{
    mActiveView->HScroll(value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayTouchDown(Display*, int x, int y, uint64_t id)
{
    HitTestToolbar(x ,y);
    mActiveView->TouchDown(x, y, id);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayTouchMove(Display*, int x, int y, uint64_t id)
{
    mActiveView->TouchMove(x, y, id);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayTouchUp(Display*, int x, int y, uint64_t id)
{
    mActiveView->TouchUp(x, y, id);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayKeyDown(Display*, Key key)
{
    mActiveView->KeyDown(key);

    mIsCtrlDown = key == Key_LeftCtrl ? true : mIsCtrlDown;
    mIsShiftDown = key == Key_LeftShift ? true : mIsShiftDown;

    if ((mIsCtrlDown && key == Key_T) || key == Key_GamepadMenu)
    {
        ShowToolbar(mToolbar ? !mToolbar->GetIsEnabled() : true);
    }
    else if (mIsCtrlDown && key == Key_F)
    {
        if (mStatsMode == StatsMode_Disabled)
        {
            if (mIsShiftDown)
            {
                EnableTitleStats(0, RoutedEventArgs(0, 0));
            }
            else
            {
                EnableWidgetStats(0, RoutedEventArgs(0, 0));
            }
        }
        else
        {
            if (mStatsMode == StatsMode_Widget)
            {
                DisableStats(0, RoutedEventArgs(0, 0));

                if (mIsShiftDown)
                {
                    EnableTitleStats(0, RoutedEventArgs(0, 0));
                }
            }
            else
            {
                DisableStats(0, RoutedEventArgs(0, 0));

                if (!mIsShiftDown)
                {
                    EnableWidgetStats(0, RoutedEventArgs(0, 0));
                }
            }
        }
    }
    else if (mIsCtrlDown && key == Key_P)
    {
        if ((mViewFlags & RenderFlags_PPAA) > 0)
        {
            DisablePPAA(0, RoutedEventArgs(0, 0));
        }
        else
        {
            EnablePPAA(0, RoutedEventArgs(0, 0));
        }
    }
  #ifdef NS_PROFILE
    else if (mIsCtrlDown && key == Key_W)
    {
        if ((mViewFlags & RenderFlags_Wireframe) > 0)
        {
            DisableWireframe(0, RoutedEventArgs(0, 0));
        }
        else
        {
            EnableWireframe(0, RoutedEventArgs(0, 0));
        }
    }
    else if (mIsCtrlDown && key == Key_B)
    {
        if ((mViewFlags & RenderFlags_ColorBatches) > 0)
        {
            DisableBatches(0, RoutedEventArgs(0, 0));
        }
        else
        {
            EnableBatches(0, RoutedEventArgs(0, 0));
        }
    }
    else if (mIsCtrlDown && key == Key_O)
    {
        if ((mViewFlags & RenderFlags_Overdraw) > 0)
        {
            DisableOverdraw(0, RoutedEventArgs(0, 0));
        }
        else
        {
            EnableOverdraw(0, RoutedEventArgs(0, 0));
        }
    }
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayKeyUp(Display*, Key key)
{
    mIsCtrlDown = key == Key_LeftCtrl ? false : mIsCtrlDown;
    mIsShiftDown = key == Key_LeftShift ? false : mIsShiftDown;

    mActiveView->KeyUp(key);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::OnDisplayChar(Display*, uint32_t c)
{
    mActiveView->Char(c);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Window::CoerceWindowStyle(const DependencyObject* object, const void* in, void* out)
{
    const Window* window = static_cast<const Window*>(object);
    WindowStyle newValue = *static_cast<const WindowStyle*>(in);
    WindowStyle& coerced = *static_cast<WindowStyle*>(out);

    coerced = window->GetFullscreen() ? WindowStyle_None : newValue;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Window::CoerceResizeMode(const DependencyObject* object, const void* in, void* out)
{
    const Window* window = static_cast<const Window*>(object);
    ResizeMode newValue = *static_cast<const ResizeMode*>(in);
    ResizeMode& coerced = *static_cast<ResizeMode*>(out);

    coerced = window->GetFullscreen() ? ResizeMode_NoResize : newValue;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Window::SetClearColor(const Color& color)
{
    if (NS_UNLIKELY(color != mContextClearColor))
    {
        mRenderContext->SetClearColor(color.r, color.g, color.b, color.a);
        mContextClearColor = color;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(Window)
{
    auto OnBackgroundChanged = [](DependencyObject* d, const DependencyPropertyChangedEventArgs& e)
    {
        Window* window = (Window*)d;
        Brush* brush = e.NewValue<Ptr<Brush>>();

        if (brush != 0)
        {
            SolidColorBrush* solid = DynamicCast<SolidColorBrush*>(brush);

            window->mBackgroundIsSolid = solid != 0;
            window->mClearColor = solid ? solid->GetColor() : Color();
        }
        else
        {
            window->mBackgroundIsSolid = true;
            window->mClearColor = Color();
        }
    };

    UIElementData* data = NsMeta<UIElementData>(TypeOf<Window>());

    data->AddOwner<bool>(AllowsTransparencyProperty, "AllowsTransparency",
        Popup::AllowsTransparencyProperty);
    data->RegisterProperty<bool>(FullscreenProperty, "Fullscreen",
        FrameworkPropertyMetadata::Create(false, FrameworkPropertyMetadataOptions_AffectsMeasure |
        FrameworkPropertyMetadataOptions_AffectsRender));
    data->RegisterProperty<bool>(IsActiveProperty, "IsActive",
        FrameworkPropertyMetadata::Create(false));
    data->AddOwner<float>(LeftProperty, "Left", Canvas::LeftProperty);
    data->RegisterProperty<ResizeMode>(ResizeModeProperty, "ResizeMode",
        FrameworkPropertyMetadata::Create(ResizeMode_CanResize,
            CoerceValueCallback(CoerceResizeMode)));
    data->RegisterProperty<bool>(ShowInTaskbarProperty, "ShowInTaskbar",
        FrameworkPropertyMetadata::Create(true));
    data->RegisterProperty<SizeToContent>(SizeToContentProperty, "SizeToContent",
        FrameworkPropertyMetadata::Create(SizeToContent_Manual));
    data->RegisterProperty<String>(TitleProperty, "Title",
        FrameworkPropertyMetadata::Create(String()));
    data->AddOwner<float>(TopProperty, "Top", Canvas::TopProperty);
    data->RegisterProperty<bool>(TopmostProperty, "Topmost",
        FrameworkPropertyMetadata::Create(false));
    data->RegisterProperty<WindowState>(WindowStateProperty, "WindowState",
        FrameworkPropertyMetadata::Create(WindowState_Normal));
    data->RegisterProperty<WindowStyle>(WindowStyleProperty, "WindowStyle",
        FrameworkPropertyMetadata::Create(WindowStyle_SingleBorderWindow,
            CoerceValueCallback(CoerceWindowStyle)));
    data->RegisterProperty<WindowStartupLocation>(WindowStartupLocationProperty,
        "WindowStartupLocation", FrameworkPropertyMetadata::Create(WindowStartupLocation_Manual));

    data->OverrideMetadata<Ptr<Brush>>(Control::BackgroundProperty, "Background",
        FrameworkPropertyMetadata::Create(PropertyChangedCallback(OnBackgroundChanged)));

    data->OverrideMetadata<bool>(Control::IsTabStopProperty, "IsTabStop",
        FrameworkPropertyMetadata::Create(false));
    data->OverrideMetadata<KeyboardNavigationMode>(KeyboardNavigation::DirectionalNavigationProperty,
        "DirectionalNavigation", FrameworkPropertyMetadata::Create(KeyboardNavigationMode_Cycle));
    data->OverrideMetadata<KeyboardNavigationMode>(KeyboardNavigation::TabNavigationProperty,
        "TabNavigation", FrameworkPropertyMetadata::Create(KeyboardNavigationMode_Cycle));
    data->OverrideMetadata<KeyboardNavigationMode>(KeyboardNavigation::ControlTabNavigationProperty,
        "ControlTabNavigation", FrameworkPropertyMetadata::Create(KeyboardNavigationMode_Cycle));
    data->OverrideMetadata<bool>(FocusManager::IsFocusScopeProperty, "IsFocusScope",
        FrameworkPropertyMetadata::Create(true));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* Window::AllowsTransparencyProperty;
const DependencyProperty* Window::FullscreenProperty;
const DependencyProperty* Window::IsActiveProperty;
const DependencyProperty* Window::LeftProperty;
const DependencyProperty* Window::ResizeModeProperty;
const DependencyProperty* Window::ShowInTaskbarProperty;
const DependencyProperty* Window::SizeToContentProperty;
const DependencyProperty* Window::TitleProperty;
const DependencyProperty* Window::TopmostProperty;
const DependencyProperty* Window::TopProperty;
const DependencyProperty* Window::WindowStateProperty;
const DependencyProperty* Window::WindowStyleProperty;
const DependencyProperty* Window::WindowStartupLocationProperty;

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_IMPLEMENT_REFLECTION_ENUM(SizeToContent)
{
    NsVal("Manual", SizeToContent_Manual);
    NsVal("Width", SizeToContent_Width);
    NsVal("Height", SizeToContent_Height);
    NsVal("WidthAndHeight", SizeToContent_WidthAndHeight);
};

NS_END_COLD_REGION
