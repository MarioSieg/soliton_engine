////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/PlatformSettings.h>

#ifdef NS_PLATFORM_OSX
#include <AppKit/NSApplication.h>

void RunLoopWakeUp()
{
    NSEvent* event = [NSEvent otherEventWithType: NSEventTypeApplicationDefined location: NSMakePoint(0,0)
        modifierFlags: 0 timestamp: 0.0 windowNumber: 0 context: nil subtype: 0 data1: 0 data2: 0];
    [NSApp postEvent: event atStart: YES];
}

#endif

