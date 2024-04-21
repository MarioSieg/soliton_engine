////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/PreviewRenderer.h>
#include <NsRender/RenderContext.h>
#include <NsRender/RenderDevice.h>
#include <NsRender/RenderTarget.h>
#include <NsRender/Image.h>
#include <NsGui/Border.h>
#include <NsGui/IView.h>
#include <NsGui/IRenderer.h>
#include <NsGui/IntegrationAPI.h>


using namespace Noesis;
using namespace NoesisApp;


NS_WARNING_PUSH

#ifdef _PREFAST_
#include <codeanalysis/warnings.h>
NS_MSVC_WARNING_DISABLE(ALL_CODE_ANALYSIS_WARNINGS)
#endif

NS_MSVC_WARNING_DISABLE(4242 4244 4100 4996)
NS_GCC_CLANG_WARNING_DISABLE("-Wconversion")
NS_GCC_CLANG_WARNING_DISABLE("-Wunused-function")
NS_GCC_CLANG_WARNING_DISABLE("-Wdeprecated-declarations")
NS_CLANG_WARNING_DISABLE("-Wunknown-warning-option")
NS_CLANG_WARNING_DISABLE("-Wcomma")

#define STBIW_MALLOC(sz) Noesis::Alloc(sz)
#define STBIW_REALLOC(p,sz) Noesis::Realloc(p, sz)
#define STBIW_FREE(p) Noesis::Dealloc(p)
#define STBIW_ASSERT(x) NS_ASSERT(x)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

NS_WARNING_POP


////////////////////////////////////////////////////////////////////////////////////////////////////
static void Save(NoesisApp:: Image* image, const char* path)
{
    uint8_t* data = image->Data();
    int x = image->Width();
    int y = image->Height();

    for (int i = 0; i < x * y; i++)
    {
        // Convert to BGRA
        uint8_t r = data[4 * i];
        uint8_t g = data[4 * i + 1];
        uint8_t b = data[4 * i + 2];
        uint8_t a = data[4 * i + 3];

        // Unremultiply the alpha
        if (a != 255)
        {
            if (a == 0)
            {
                r = 0;
                g = 0;
                b = 0;
            }
            else
            {
                r = 255 * r / a;
                g = 255 * g / a;
                b = 255 * b / a;
            }
        }

        data[4 * i] = b;
        data[4 * i + 1] = g;
        data[4 * i + 2] = r; 
        data[4 * i + 3] = a;
    }

    int err  = stbi_write_png(path, x, y, 4, data, image->Stride());
    NS_ASSERT(err == 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::RenderPreview(RenderContext* context, UIElement* content, int width, int height,
    double time, const char* savePath)
{
    NS_ASSERT(width > 0);
    NS_ASSERT(height > 0);

    const Ptr<Border> root = *new Border();
    root->SetChild(content);

    const Ptr<IView> view = GUI::CreateView(root);
    view->SetSize(width, height);
    view->SetFlags(RenderFlags_PPAA);
    view->SetTessellationMaxPixelError(TessellationMaxPixelError::HighQuality());
    view->Update(0.0);

    RenderDevice* device = context->GetDevice();
    Ptr<RenderTarget> texture = device->CreateRenderTarget("Preview", width, height, 1, true);

    //Render
    context->BeginRender();

    IRenderer* renderer = view->GetRenderer();
    renderer->Init(device);

    // Generate a new frame for the time specified in the preview. Must be done after previous
    // frame is processed by the renderer inside Init, so the update buffer write pointer can
    // be advanced, and this new frame can be consumed by the following UpdateRenderTree
    view->Update(time);

    renderer->UpdateRenderTree();
    renderer->RenderOffscreen();

    const Tile tile = { 0, 0, (uint32_t)width, (uint32_t)height };

    device->SetRenderTarget(texture);
    device->BeginTile(texture, tile);
    renderer->Render(false, true);
    device->EndTile(texture);

    device->ResolveRenderTarget(texture, &tile, 1);

    Ptr<Image> image = context->CaptureRenderTarget(texture);
    context->EndRender();
    renderer->Shutdown();

    Save(image, savePath);
}
