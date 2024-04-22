////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __RENDER_RENDERCONTEXT_H__
#define __RENDER_RENDERCONTEXT_H__


#include <NsCore/Noesis.h>
#include <NsCore/BaseComponent.h>
#include <NsCore/ArrayRef.h>
#include <NsRender/RenderContextApi.h>


namespace Noesis
{
class RenderDevice;
class RenderTarget;
template<class T> class Ptr;
}

namespace NoesisApp
{

class Image;

#if defined(NS_PLATFORM_WINDOWS) || defined(NS_PLATFORM_SCE) || defined(NS_PLATFORM_NX) || \
    defined(NS_PLATFORM_ANDROID) || defined(NS_PLATFORM_EMSCRIPTEN)
#define HAVE_CUSTOM_SHADERS
#endif

struct BrushShaders
{
    bool created = false;
    void* shaders[5] = {};
};

struct EffectShaders
{
    bool created = false;
    void* shaders[1] = {};
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// RenderContext implementation is in charge of the initialization of a rendering device
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_RENDER_RENDERCONTEXT_API RenderContext: public Noesis::BaseComponent
{
public:
    /// Returns the best available context
    static Noesis::Ptr<RenderContext> Create();

    /// Returns the context registered with the given name
    static Noesis::Ptr<RenderContext> Create(const char* name);

    /// Gets the current render context
    static RenderContext* Current();

    /// Creates all Brush Shader Permutations
    static void EnsureShaders(BrushShaders& shaders, const char* label,
        Noesis::ArrayRef<uint8_t> blob);

    /// Creates all Effect Shader Permutations
    static void EnsureShaders(EffectShaders& shaders, const char* label,
        Noesis::ArrayRef<uint8_t> blob);

    /// Brief description provided by the implementation
    virtual const char* Description() const = 0;

    /// Returns the shader language used by this implementation
    virtual const char* ShaderLang() const;

    /// When looking for the best render context, those with higher score are tried first
    virtual uint32_t Score() const;

    /// Returns whether the implementation is valid
    virtual bool Validate() const;

    /// Initializes the rendering context with the given window and multisampling samples
    virtual void Init(void* window, uint32_t& samples, bool vsync, bool sRGB);

    /// Cleans up an initialized rendering context
    virtual void Shutdown();

    /// Changes the window associated to this context
    virtual void SetWindow(void* window);

    /// Invoked to save cached state like pipeline objects
    virtual void SaveState();

    /// Returns the rendering device maintained by this context
    virtual Noesis::RenderDevice* GetDevice() const = 0;

    /// Called prior to rendering
    virtual void BeginRender();

    /// Called after the rendering
    virtual void EndRender();

    /// Should be called when the window is resized
    virtual void Resize();

    /// Returns the milliseconds taken by the last frame executed in the GPU
    virtual float GetGPUTimeMs() const;

    /// Sets the clear color (in sRGB space)
    virtual void SetClearColor(float r, float g, float b, float a);

    /// Binds the render targets associated with the window swap chain and clears it
    virtual void SetDefaultRenderTarget(uint32_t width, uint32_t height, bool doClearColor);

    /// Grabs an image with the content of current render target
    virtual Noesis::Ptr<Image> CaptureRenderTarget(Noesis::RenderTarget* surface) const;

    /// Copies the content of the back-buffer to the front buffer
    virtual void Swap();

    /// Creates a pixel shader
    virtual void* CreatePixelShader(const char* label, uint8_t shader,
        Noesis::ArrayRef<uint8_t> code);

    NS_DECLARE_REFLECTION(RenderContext, BaseComponent)
};

}

#endif
