////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_ANDROIDDISPLAY_H__
#define __APP_ANDROIDDISPLAY_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsApp/Display.h>


struct android_app;
struct AInputEvent;


namespace NoesisApp
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Implementation of Display using Android NDK.
////////////////////////////////////////////////////////////////////////////////////////////////////
class AndroidDisplay: public Display
{
public:
    AndroidDisplay();
    ~AndroidDisplay();

    /// From Display
    //@{
    void Show() override;
    void EnterMessageLoop(bool runInBackground) override;
    void Close() override;
    void OpenSoftwareKeyboard(Noesis::UIElement* focused) override;
    void CloseSoftwareKeyboard() override;
    void OpenUrl(const char* url) override;
    void* GetNativeHandle() const override;
    uint32_t GetClientWidth() const override;
    uint32_t GetClientHeight() const override;
    float GetScale() const override;
    //@}

private:
    bool IsReadyToRender() const;
    int LooperTimeout() const;
    bool ProcessMessages();

    static void ChoreographerCallback(long frameTimeNanos, void* data);
    void ChoreographerEnqueue();
    void ChoreographerStart();

    static void OnAppCmd(android_app* app, int cmd);
    void DispatchAppEvent(int eventId);

    static int OnInputEvent(android_app* app, AInputEvent* event);
    int DispatchInputEvent(AInputEvent* event);

    void FillKeyTable();

private:
    android_app* mApp;

    int32_t mWidth, mHeight;

    bool mHasFocus;
    bool mIsVisible;
    bool mHasWindow;
    bool mChoreographerEnabled;
    bool mChoreographerStarted;

    struct AChoreographer;
    typedef void (*AChoreographer_frameCallback)(long frameTimeNanos, void* data);
    typedef AChoreographer* (*AChoreographer_getInstanceT)();
    typedef void (*AChoreographer_postFrameCallbackT)(AChoreographer*, AChoreographer_frameCallback,
        void*);

    AChoreographer_getInstanceT AChoreographer_getInstance;
    AChoreographer_postFrameCallbackT AChoreographer_postFrameCallback;

    uint8_t mKeyTable[256];

    NS_DECLARE_REFLECTION(AndroidDisplay, Display)
};

}


#endif
