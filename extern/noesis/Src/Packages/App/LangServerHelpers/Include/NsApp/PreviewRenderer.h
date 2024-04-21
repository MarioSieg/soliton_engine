////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_LANGSERVERPREVIEWRENDERER_H__
#define __APP_LANGSERVERPREVIEWRENDERER_H__


#include <NsCore/Noesis.h>
#include <NsApp/LangServerHelpersApi.h>


namespace Noesis { class UIElement; }

namespace NoesisApp
{

class RenderContext;

// Captures a snapshot of the specified Noesis UIElement at the given time and renders it into a
// image with the provided dimensions. The image is saved to a file specified by 'savePath'
NS_APP_LANGSERVERHELPERS_API void RenderPreview(RenderContext* context, Noesis::UIElement* content,
    int renderWidth, int renderHeight, double renderTime, const char* savePath);

}

#endif
