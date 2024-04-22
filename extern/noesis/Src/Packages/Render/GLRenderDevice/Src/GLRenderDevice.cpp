////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


//#undef NS_LOG_TRACE
//#define NS_LOG_TRACE(...) NS_LOG_(NS_LOG_LEVEL_TRACE, __VA_ARGS__)


#include "GLRenderDevice.h"

#include <NsRender/Texture.h>
#include <NsRender/RenderTarget.h>
#include <NsCore/Ptr.h>
#include <NsCore/Log.h>
#include <NsCore/Pair.h>
#include <NsCore/HighResTimer.h>
#include <NsCore/StringUtils.h>
#include <NsCore/String.h>

#include <stdio.h>
#include <stdarg.h>

#if NS_RENDERER_USE_NSGL
    #include <OpenGL/CGLCurrent.h>
    #include <dlfcn.h>
#endif


using namespace Noesis;
using namespace NoesisApp;


#define NS_GL_VER(major, minor) (major * 10 + minor)
#define INVALIDATED_RENDER_STATE ((uint32_t)-1)
#define PRECREATE_VR_SHADERS 0

#ifdef NS_DEBUG
    #define V(exp) \
        NS_MACRO_BEGIN \
            while (glGetError() != GL_NO_ERROR); \
            exp; \
            GLenum err = glGetError(); \
            if (err != GL_NO_ERROR) \
            { \
                switch (err) \
                { \
                    case GL_INVALID_ENUM: NS_FATAL("%s [GL_INVALID_ENUM]", #exp); \
                    case GL_INVALID_VALUE: NS_FATAL("%s [GL_INVALID_VALUE]", #exp); \
                    case GL_INVALID_OPERATION: NS_FATAL("%s [GL_INVALID_OPERATION]", #exp); \
                    case GL_INVALID_FRAMEBUFFER_OPERATION: NS_FATAL("%s [GL_INVALID_FRAMEBUFFER_OPERATION]", #exp); \
                    case GL_OUT_OF_MEMORY: NS_FATAL("%s [GL_OUT_OF_MEMORY]", #exp); \
                    default: NS_FATAL("%s [0x%08x]", #exp, err); \
                } \
            } \
        NS_MACRO_END
#else
    #define V(exp) exp
#endif

#ifdef NS_PROFILE
    #define GL_PUSH_DEBUG_GROUP(n) PushDebugGroup(n)
    #define GL_POP_DEBUG_GROUP() PopDebugGroup()
    #define GL_OBJECT_LABEL(s, id, ...) ObjectLabel(s, id, __VA_ARGS__)
#else
    #define GL_PUSH_DEBUG_GROUP(n) NS_NOOP
    #define GL_POP_DEBUG_GROUP() NS_NOOP
    #define GL_OBJECT_LABEL(...) NS_UNUSED(__VA_ARGS__)
#endif

#ifdef NS_COMPILER_MSVC
#define sscanf sscanf_s
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
class GLTexture final: public Texture
{
public:
    GLTexture(GLRenderDevice* device_, uint32_t width_, uint32_t height_, uint32_t levels_,
        GLenum format_, bool isInverted_, bool hasAlpha_): device(device_), width(width_),
        height(height_), levels(levels_), format(format_), isInverted(isInverted_),
        hasAlpha(hasAlpha_), object(0) {}

    GLTexture(GLuint object_, uint32_t width_, uint32_t height_, uint32_t levels_, GLenum format_,
        bool isInverted_, bool hasAlpha_): device(0), width(width_), height(height_),
        levels(levels_), format(format_), isInverted(isInverted_), hasAlpha(hasAlpha_),
        object(object_) {}

    ~GLTexture()
    {
        // We don't have ownership of textures created by WrapTexture()
        if (device != 0)
        {
            device->DeleteTexture(this);
        }
    }

    uint32_t GetWidth() const override { return width; }
    uint32_t GetHeight() const override { return height; }
    bool HasMipMaps() const override { return levels > 1; }
    bool IsInverted() const override { return isInverted; }
    bool HasAlpha() const override { return hasAlpha; }

    GLRenderDevice* const device;

    const uint32_t width;
    const uint32_t height;
    const uint32_t levels;
    const GLenum format;
    const bool isInverted;
    const bool hasAlpha;

    GLuint object;
    SamplerState state;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class GLRenderTarget final: public RenderTarget
{
public:
    GLRenderTarget(GLRenderDevice* device_, uint32_t width_, uint32_t height_, GLsizei samples_):
        device(device_), width(width_), height(height_), samples(samples_), fbo(0), fboResolved(0),
        stencil(0), colorAA(0)
    {
        texture = *new GLTexture(device, width, height, 1, GL_RGBA, true, true);
    }

    ~GLRenderTarget()
    {
        texture.Reset();
        NS_ASSERT(device != 0);
        device->DeleteRenderTarget(this);
    }

    Texture* GetTexture() override { return texture; }

    GLRenderDevice* const device;

    const uint32_t width;
    const uint32_t height;
    const GLsizei samples;

    GLuint fbo;
    GLuint fboResolved;

    GLuint stencil;
    GLuint colorAA;

    Ptr<GLTexture> texture;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
GLRenderDevice::GLRenderDevice(bool sRGB): mGLVersion(0), mDefaultVAO(0)
{
    BindAPI();
    FindExtensions();
    FillCaps(sRGB);

    CreateSamplers();
    CreatePrograms();
    CreateVertexFormats();

    InvalidateStateCache();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLRenderDevice::~GLRenderDevice()
{
    if (mDefaultVAO != 0)
    {
        DeleteVertexArrays(1, &mDefaultVAO);
    }

    for (uint32_t i = 0; i < NS_COUNTOF(mVertexFormats); i++)
    {
        if (mVertexFormats[i].vao != 0)
        {
            DeleteVertexArrays(1, &mVertexFormats[i].vao);
        }
    }

    DestroyBuffer(mDynamicVB);
    DestroyBuffer(mDynamicIB);

  #if GL_VERSION_3_1 || GL_ES_VERSION_3_0 || GL_ARB_uniform_buffer_object
    if (IsSupported(Extension::ARB_uniform_buffer_object))
    {
        DestroyBuffer(mUBO);
    }
  #endif
    

    for (uint32_t i = 0; i < NS_COUNTOF(mSamplers); i++)
    {
        if (mSamplers[i] != 0)
        {
            DeleteSampler(mSamplers[i]);
        }
    }

    for (uint32_t i = 0; i < NS_COUNTOF(mVsShaders); i++)
    {
        if (mVsShaders[i] != 0)
        {
            DeleteShader(mVsShaders[i]);
        }

        if (mVsShadersVR[i] != 0)
        {
            DeleteShader(mVsShadersVR[i]);
        }
    }

    for (uint32_t i = 0; i < NS_COUNTOF(mPrograms); i++)
    {
        if (mPrograms[i].object != 0)
        {
            DeleteProgram(mPrograms[i].object);
        }

        if (mProgramsVR[i].object != 0)
        {
            DeleteProgram(mProgramsVR[i].object);
        }
    }

    for (const ProgramInfo& program : mCustomShaders)
    {
        DeleteProgram(program.object);
    }

    for (const ProgramInfo& program : mCustomShadersVR)
    {
        DeleteProgram(program.object);
    }

#if NS_RENDERER_USE_WGL
    FreeLibrary(mOpenGL32);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Texture> GLRenderDevice::WrapTexture(GLuint object, uint32_t width, uint32_t height,
    uint32_t levels, bool isInverted, bool hasAlpha)
{
    return *new GLTexture(object, width, height, levels, GL_RGBA, isInverted, hasAlpha);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* GLRenderDevice::CreatePixelShader(const char* label, uint8_t shader, const char* glsl)
{
    NS_ASSERT(shader < NS_COUNTOF(VertexForShader));
    uint32_t vsIndex = VertexForShader[shader];

    GLuint pso = CompileShader(GL_FRAGMENT_SHADER, &glsl, 1);
    GL_OBJECT_LABEL(GL_SHADER, pso, "Noesis_%s_Shader", label);

    {
        EnsureVsShader(vsIndex, false);
        GLuint vso = mVsShaders[vsIndex];

        ProgramInfo& program = mCustomShaders.EmplaceBack();
        program.vertexFormat = FormatForVertex[vsIndex];
        LinkProgram(vso, pso, program, label);
    }

  #if NS_RENDERER_OPENGL
    if (IsSupported(Extension::AMD_vertex_shader_layer) ||
        IsSupported(Extension::ARB_shader_viewport_layer_array))
  #else
    if (IsSupported(Extension::OVR_multiview))
  #endif
    {
        EnsureVsShader(vsIndex, true);
        GLuint vso = mVsShadersVR[vsIndex];

        ProgramInfo& program = mCustomShadersVR.EmplaceBack();
        program.vertexFormat = FormatForVertex[vsIndex];
        LinkProgram(vso, pso, program, label);
    }

    DeleteShader(pso);

    return (void*)(uintptr_t)mCustomShaders.Size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::ClearPixelShaders()
{
    for (const ProgramInfo& program : mCustomShaders)
    {
        DeleteProgram(program.object);
    }

    mCustomShaders.Clear();

    for (const ProgramInfo& program : mCustomShadersVR)
    {
        DeleteProgram(program.object);
    }

    mCustomShadersVR.Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DeviceCaps& GLRenderDevice::GetCaps() const
{
    return mCaps;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderTarget> GLRenderDevice::CreateRenderTarget(const char* label, uint32_t width,
    uint32_t height, uint32_t sampleCount, bool needsStencil)
{
    NS_ASSERT(sampleCount > 0);
    NS_LOG_TRACE("CreateRenderTarget '%s'", label);

    // samples must be 0 to disable multisampling in glRenderbufferStorageMultisample
    sampleCount = Min(sampleCount, MaxSamples());
    GLsizei samples = sampleCount == 1 ? 0 : sampleCount;

    Ptr<GLRenderTarget> surface = *new GLRenderTarget(this, width, height, samples);

    // Texture creation
    GLenum format;
    if (mCaps.linearRendering)
    {
        NS_ASSERT(HaveTexStorage());
        format = GL_SRGB8_ALPHA8;
    }
    else
    {
        format = HaveTexStorage() ? GL_RGBA8 : GL_RGBA;
    }

    InitTexture(label, surface->texture, format, GL_RGBA);

    if (samples != 0 && !HaveMultisampledRenderToTexture())
    {
        surface->colorAA = CreateColor(label, width, height, samples);
    }

    if (needsStencil)
    {
        surface->stencil = CreateStencil(label, width, height, samples);
    }

    CreateFBO(label, samples, surface->texture->object, surface->stencil, surface->colorAA,
        surface->fbo, surface->fboResolved);

    return surface;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderTarget> GLRenderDevice::CloneRenderTarget(const char* label, RenderTarget* shared_)
{
    NS_LOG_TRACE("CreateRenderTarget '%s'", label);

    GLRenderTarget* shared = (GLRenderTarget*)shared_;
    GLsizei samples = shared->samples;
    uint32_t width = shared->width;
    uint32_t height = shared->height;

    Ptr<GLRenderTarget> surface = *new GLRenderTarget(this, width, height, samples);

    // Texture creation
    GLenum format;
    if (mCaps.linearRendering)
    {
        NS_ASSERT(HaveTexStorage());
        format = GL_SRGB8_ALPHA8;
    }
    else
    {
        format = HaveTexStorage() ? GL_RGBA8 : GL_RGBA;
    }

    InitTexture(label, surface->texture, format, GL_RGBA);

    CreateFBO(label, samples, surface->texture->object, shared->stencil, shared->colorAA,
        surface->fbo, surface->fboResolved);

    return surface;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Texture> GLRenderDevice::CreateTexture(const char* label, uint32_t width, uint32_t height,
    uint32_t numLevels, TextureFormat::Enum format_, const void** data)
{
    NS_LOG_TRACE("CreateTexture '%s'", label);

    GLenum internalFormat, format;
    GetGLFormat(format_, internalFormat, format);

    bool hasAlpha = format_ == TextureFormat::RGBA8;
    Ptr<GLTexture> texture = *new GLTexture(this, width, height, numLevels, format, false, hasAlpha);

    InitTexture(label, texture, internalFormat, format);

    if (data != 0)
    {
        for (uint32_t i = 0; i < numLevels; i++)
        {
            UpdateTexture(texture, i, 0, 0, width >> i, height >> i, data[i]);
        }
    }

    return texture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::UpdateTexture(Texture* texture_, uint32_t level, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height, const void* data)
{
    ActiveTexture(0);

    GLTexture* texture = (GLTexture*)(texture_);
    BindTexture2D(texture->object);

    TexSubImage2D(level, x, y, width, height, texture->format, data);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BeginOffscreenRender()
{
    GL_PUSH_DEBUG_GROUP("Noesis.Offscreen");
    InvalidateStateCache();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::EndOffscreenRender()
{
    UnbindObjects();
    GL_POP_DEBUG_GROUP();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BeginOnscreenRender()
{
    GL_PUSH_DEBUG_GROUP("Noesis");
    InvalidateStateCache();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::EndOnscreenRender()
{
    UnbindObjects();
    GL_POP_DEBUG_GROUP();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::SetRenderTarget(RenderTarget* surface)
{
    GL_PUSH_DEBUG_GROUP("SetRenderTarget");
    BindRenderTarget((GLRenderTarget*)surface);

    GLenum attachments[] = { GL_COLOR_ATTACHMENT0, GL_STENCIL_ATTACHMENT };
    DiscardFramebuffer(GL_FRAMEBUFFER, 2, attachments);

    V(glEnable(GL_SCISSOR_TEST));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BeginTile(RenderTarget*, const Tile& tile)
{
  #if GL_QCOM_tiled_rendering
    if (IsSupported(Extension::QCOM_tiled_rendering))
    {
        V(glStartTilingQCOM(tile.x, tile.y, tile.width, tile.height, 0));
    }
  #endif

    // On Mali devices, the scissor for each drawcall tells the driver which tiles must be stored
    // Qualcomm devices ignore the scissor but have "GL_QCOM_tiled_rendering" for the same purpose
    V(glScissor(tile.x, tile.y, tile.width, tile.height));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::EndTile(RenderTarget*)
{
  #if GL_QCOM_tiled_rendering
    if (IsSupported(Extension::QCOM_tiled_rendering))
    {
        V(glEndTilingQCOM(GL_COLOR_BUFFER_BIT0_QCOM));
    }
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::ResolveRenderTarget(RenderTarget* surface, const Tile* tiles, uint32_t numTiles)
{
    Resolve((GLRenderTarget*)surface, tiles, numTiles);
    GL_POP_DEBUG_GROUP();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* GLRenderDevice::MapVertices(uint32_t bytes)
{
    return MapBuffer(mDynamicVB, bytes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::UnmapVertices()
{
    UnmapBuffer(mDynamicVB);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* GLRenderDevice::MapIndices(uint32_t bytes)
{
    return MapBuffer(mDynamicIB, bytes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::UnmapIndices()
{
    UnmapBuffer(mDynamicIB);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DrawBatch(const Batch& batch)
{
    ProgramInfo* program;

    if (batch.pixelShader != nullptr)
    {
        if (batch.singlePassStereo)
        {
            NS_ASSERT((uintptr_t)batch.pixelShader <= mCustomShadersVR.Size());
            program = &mCustomShadersVR[(int)(uintptr_t)batch.pixelShader - 1];
        }
        else
        {
            NS_ASSERT((uintptr_t)batch.pixelShader <= mCustomShaders.Size());
            program = &mCustomShaders[(int)(uintptr_t)batch.pixelShader - 1];
        }
    }
    else
    {
        NS_ASSERT(batch.shader.v < NS_COUNTOF(mPrograms));
        EnsureProgram(batch.shader.v, batch.singlePassStereo);
        program = batch.singlePassStereo ? &mProgramsVR[batch.shader.v] : &mPrograms[batch.shader.v];
    }

    UseProgram(program->object);
    SetRenderState(batch.renderState, batch.stencilRef);
    SetVertexFormat(program->vertexFormat, mDynamicVB.drawPos + batch.vertexOffset);
    SetTextures(batch);

    UploadUniforms(program, 0, batch.vertexUniforms[0]);
    UploadUniforms(program, 1, batch.vertexUniforms[1]);
    UploadUniforms(program, 2, batch.pixelUniforms[0]);
    UploadUniforms(program, 3, batch.pixelUniforms[1]);

    NS_ASSERT(GetInteger(GL_ELEMENT_ARRAY_BUFFER_BINDING) == (GLint)mDynamicIB.boundObject);
    const GLvoid *indices = (GLvoid*)(uintptr_t)(mDynamicIB.drawPos + 2 * batch.startIndex);

  #if NS_RENDERER_OPENGL
    if (batch.singlePassStereo)
    {
        V(glDrawElementsInstanced(GL_TRIANGLES, batch.numIndices, GL_UNSIGNED_SHORT, indices, 2));
    }
    else
    {
        V(glDrawElements(GL_TRIANGLES, batch.numIndices, GL_UNSIGNED_SHORT, indices));
    }
  #else
    V(glDrawElements(GL_TRIANGLES, batch.numIndices, GL_UNSIGNED_SHORT, indices));
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DeleteTexture(GLTexture* texture)
{
    GLuint object = texture->object;

    if (object != 0)
    {
        for (uint32_t i = 0; i < NS_COUNTOF(mBoundTextures); i++)
        {
            if (mBoundTextures[i] == object)
            {
                mBoundTextures[i] = 0;
            }
        }

        DeleteTexture(object);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DeleteRenderTarget(GLRenderTarget* surface)
{
    if (mBoundRenderTarget == surface)
    {
        mBoundRenderTarget = 0;
    }

    if (surface->stencil != 0)
    {
        DeleteRenderbuffer(surface->stencil);
    }

    if (surface->colorAA != 0)
    {
        DeleteRenderbuffer(surface->colorAA);
    }

    if (surface->fbo != 0)
    {
        DeleteFramebuffer(surface->fbo);
    }

    if (surface->fboResolved != 0)
    {
        DeleteFramebuffer(surface->fboResolved);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::InvalidateStateCache()
{
    mViewportWidth = INVALIDATED_RENDER_STATE;
    mViewportHeight = INVALIDATED_RENDER_STATE;
    mVertexArray = INVALIDATED_RENDER_STATE;
    mActiveTextureUnit = INVALIDATED_RENDER_STATE;
    mDynamicVB.boundObject = INVALIDATED_RENDER_STATE;
    mDynamicIB.boundObject = INVALIDATED_RENDER_STATE;
    mProgram = INVALIDATED_RENDER_STATE;
    mEnabledVertexAttribArrays = INVALIDATED_RENDER_STATE;
    mBlendEnabled = INVALIDATED_RENDER_STATE;
    mBlendFunc = INVALIDATED_RENDER_STATE;
    mStencilEnabled = INVALIDATED_RENDER_STATE;
    mStencilOp = INVALIDATED_RENDER_STATE;
    mStencilRef = INVALIDATED_RENDER_STATE;
    mDepthEnabled = INVALIDATED_RENDER_STATE;

    mUnpackStateSet = false;
    mBoundRenderTarget = 0;
    mUBOHash = 0;

    memset(mBoundTextures, 0, sizeof(mBoundTextures));
    memset(mBoundSamplers, 0, sizeof(mBoundSamplers));

    V(glDisable(GL_CULL_FACE));
    V(glBlendEquation(GL_FUNC_ADD));
    V(glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE));
    V(glDisable(GL_SAMPLE_COVERAGE));
    V(glDepthMask(GL_FALSE));
    V(glDepthFunc(GL_LEQUAL));
    V(glStencilMask(0xff));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::UnbindObjects()
{
    if (BindVertexArray_ != 0)
    {
        V(BindVertexArray_(0));
        mVertexArray = 0;
    }

    V(glBindBuffer(GL_ARRAY_BUFFER, 0));
    mDynamicVB.boundObject = 0;

    V(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    mDynamicIB.boundObject = 0;

  #if GL_VERSION_3_1 || GL_ES_VERSION_3_0 || GL_ARB_uniform_buffer_object
    if (IsSupported(Extension::ARB_uniform_buffer_object))
    {
        V(glBindBufferRange(GL_UNIFORM_BUFFER, 0, 0, 0, 0));
    }
  #endif

    for (uint32_t i = 0; i < TextureUnit::Count; i++)
    {
        V(glActiveTexture(GL_TEXTURE0 + i));
        V(glBindTexture(GL_TEXTURE_2D, 0));
        mBoundTextures[i] = 0;

        if (IsSupported(Extension::ARB_sampler_objects))
        {
            V(glBindSampler(i, 0));
            mBoundSamplers[i] = 0;
        }
    }

    mActiveTextureUnit = TextureUnit::Count - 1;

    V(glUseProgram(0));
    mProgram = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLRenderDevice::Page* GLRenderDevice::AllocatePage(DynamicBuffer& buffer)
{
    Page* page = (Page*)Alloc(sizeof(Page));
    memset(page, 0, sizeof(Page));

    V(glGenBuffers(1, &page->object));
    V(glBindBuffer(buffer.target, page->object));

    if (HaveBufferStorage())
    {
        GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        BufferStorage(buffer.target, buffer.size, flags);
        page->base = (uint8_t*)MapBufferRange(buffer.target, 0, buffer.size, flags);
        page->sync = 0;
    }
    else
    {
        V(glBufferData(buffer.target, buffer.size, 0, GL_STREAM_DRAW));
    }

    NS_LOG_TRACE("Page '%s[%d]' created (%d KB)", buffer.label, buffer.numPages, buffer.size);
    GL_OBJECT_LABEL(GL_BUFFER, page->object, "Noesis_%s[%d]", buffer.label, buffer.numPages);

    buffer.numPages++;
    return page;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::CreateBuffer(DynamicBuffer& buffer, GLenum target, uint32_t size,
    const char* label)
{
    buffer = { target, label, size };

    if (HaveBufferStorage())
    {
        NS_ASSERT(IsSupported(Extension::ARB_sync));
        buffer.currentPage = AllocatePage(buffer);
    }
    else
    {
        if (!HaveMapBufferRange())
        {
            buffer.cpuMemory = Alloc(size);
        }

        for (uint32_t i = 0; i < 6; i++)
        {
            Page* page = AllocatePage(buffer);
            page->next = buffer.freePages;
            buffer.freePages = page;
        }

        buffer.currentPage = buffer.freePages;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DestroyBuffer(DynamicBuffer& buffer) const
{
    Dealloc(buffer.cpuMemory);

    if (HaveBufferStorage())
    {
        if (buffer.currentPage)
        {
            V(glDeleteBuffers(1, &buffer.currentPage->object));
            if (buffer.currentPage->sync != 0) { V(glDeleteSync(buffer.currentPage->sync)); }
            Dealloc(buffer.currentPage);
        }

        for (Page* page = buffer.pendingPages; page != nullptr;)
        {
            Page* next = page->next;
            V(glDeleteBuffers(1, &page->object));
            if (page->sync != 0) { V(glDeleteSync(page->sync)); }
            Dealloc(page);
            page = next;
        }
    }

    for (Page* page = buffer.freePages; page != nullptr;)
    {
        Page* next = page->next;
        V(glDeleteBuffers(1, &page->object));
        if (page->sync != 0) { V(glDeleteSync(page->sync)); }
        Dealloc(page);
        page = next;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BindBuffer(DynamicBuffer& buffer)
{
    if (HaveVertexArrayObject())
    {
        if (buffer.target == GL_ARRAY_BUFFER || buffer.target == GL_ELEMENT_ARRAY_BUFFER)
        {
            BindVertexArray(mDefaultVAO);
        }
    }

    if (buffer.currentPage->object != buffer.boundObject)
    {
        NS_ASSERT(glIsBuffer(buffer.currentPage->object));
        V(glBindBuffer(buffer.target, buffer.currentPage->object));
        buffer.boundObject = buffer.currentPage->object;
    }

    NS_ASSERT(buffer.boundObject == buffer.currentPage->object);
    NS_ASSERT(GetInteger(buffer.target == GL_ARRAY_BUFFER ? GL_ARRAY_BUFFER_BINDING :
        buffer.target == GL_ELEMENT_ARRAY_BUFFER ? GL_ELEMENT_ARRAY_BUFFER_BINDING : 
        GL_UNIFORM_BUFFER_BINDING) == GLint(buffer.currentPage->object));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static GLint GLSize(uint32_t type)
{
    switch (type)
    {
        case Shader::Vertex::Format::Attr::Type::Float: return 1;
        case Shader::Vertex::Format::Attr::Type::Float2: return 2;
        case Shader::Vertex::Format::Attr::Type::Float4: return 4;
        case Shader::Vertex::Format::Attr::Type::UByte4Norm: return 4;
        case Shader::Vertex::Format::Attr::Type::UShort4Norm: return 4;
        default: NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static GLenum GLType(uint32_t type)
{
    switch (type)
    {
        case Shader::Vertex::Format::Attr::Type::Float: return GL_FLOAT;
        case Shader::Vertex::Format::Attr::Type::Float2: return GL_FLOAT;
        case Shader::Vertex::Format::Attr::Type::Float4: return GL_FLOAT;
        case Shader::Vertex::Format::Attr::Type::UByte4Norm: return GL_UNSIGNED_BYTE;
        case Shader::Vertex::Format::Attr::Type::UShort4Norm: return GL_UNSIGNED_SHORT;
        default: NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static GLboolean GLNormalized(uint32_t type)
{
    switch (type)
    {
        case Shader::Vertex::Format::Attr::Type::Float: return GL_FALSE;
        case Shader::Vertex::Format::Attr::Type::Float2: return GL_FALSE;
        case Shader::Vertex::Format::Attr::Type::Float4: return GL_FALSE;
        case Shader::Vertex::Format::Attr::Type::UByte4Norm: return GL_TRUE;
        case Shader::Vertex::Format::Attr::Type::UShort4Norm: return GL_TRUE;
        default: NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::SetVertexFormat(uint8_t format_, uintptr_t offset)
{
    NS_ASSERT(format_ < NS_COUNTOF(mVertexFormats));
    VertexFormat& format = mVertexFormats[format_];
    GLsizei stride = format.stride;

#if !NS_RENDERER_USE_EMS
  #if GL_VERSION_4_3 || GL_ES_VERSION_3_1 || GL_ARB_vertex_attrib_binding
    if (IsSupported(Extension::ARB_vertex_attrib_binding))
    {
        BindVertexArray(format.vao);
        V(glBindVertexBuffer(0, mDynamicVB.boundObject, offset, stride));

        if (format.vao_ib != mDynamicIB.boundObject)
        {
            V(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mDynamicIB.boundObject));
            format.vao_ib = mDynamicIB.boundObject;
        }
    }
    else
  #endif
#endif
    {
        NS_ASSERT(GetInteger(GL_ARRAY_BUFFER_BINDING) == (GLint)mDynamicVB.boundObject);
        
        uint8_t attributes = AttributesForFormat[format_];
        EnableVertexAttribArrays(attributes);

        for (uint32_t i = 0; i < Shader::Vertex::Format::Attr::Count; i++)
        {
            if (attributes & (1 << i))
            {
                uint8_t t = TypeForAttr[i];
                V(glVertexAttribPointer(i, GLSize(t), GLType(t), GLNormalized(t), stride, (void*)offset));
                offset += SizeForType[t];
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DisableClientState() const
{
#if NS_RENDERER_OPENGL
    // If external code enabled any of these client states (like Unity is doing in OSX) then our
    // Vertex Buffers are ignored and a slow path is followed inside the GL driver
    if (mGLVersion < NS_GL_VER(3,0))
    {
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_EDGE_FLAG_ARRAY);
        glDisableClientState(GL_FOG_COORD_ARRAY);
        glDisableClientState(GL_INDEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::EnableVertexAttribArrays(uint32_t state)
{
    if (mEnabledVertexAttribArrays == 0xffffffff)
    {
        mEnabledVertexAttribArrays = ~state;

        if (mDefaultVAO == 0)
        {
            DisableClientState();

            for (uint32_t i = Shader::Vertex::Format::Attr::Count; i < mMaxVertexAttribs; i++)
            {
                V(glDisableVertexAttribArray(i));
            }
        }
    }

    NS_ASSERT([&]()
    {
        for (uint32_t i = Shader::Vertex::Format::Attr::Count; i < mMaxVertexAttribs; i++)
        {
            if (VertexAttribArrayEnabled(i)) { return false; }
        }

        return true;
    }());

    uint32_t delta = state ^ mEnabledVertexAttribArrays;
    if (delta != 0)
    {
        for (uint32_t i = 0; i < Shader::Vertex::Format::Attr::Count; i++)
        {
            uint32_t mask = 1 << i;
            if (delta & mask)
            {
                if (state & mask)
                {
                    V(glEnableVertexAttribArray(i));
                }
                else
                {
                    V(glDisableVertexAttribArray(i));
                }
            }
        }

        mEnabledVertexAttribArrays = state;
    }

    NS_ASSERT([&]()
    {
        for (uint32_t i = 0; i < Shader::Vertex::Format::Attr::Count; i++)
        {
            if (VertexAttribArrayEnabled(i) != ((state & (1 << i)) > 0)) { return false; }
        }

        return true;
    }());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* GLRenderDevice::MapBuffer(DynamicBuffer& buffer, uint32_t size)
{
    NS_ASSERT(size <= buffer.size);
    NS_ASSERT(buffer.pos <= buffer.size);

    if (HaveBufferStorage())
    {
      #ifndef NS_PLATFORM_ANDROID
        // Moving forward within the same buffer (no_overwrite) is buggy on Android.
        // For now we leave it disabled as the performance is almost the same (just extra memory).
        // The following devices reported problems:
        //
        //   Model         GPU
        //   ------------------------
        //   OnePlus 6T    Adreno 630
        //   ------------------------
        if (buffer.pos + size > buffer.size)
      #endif
        {
            // We ran out of space in the current page, get a new one
            // Move the current one to pending and insert a GPU fence
            NS_ASSERT(IsSupported(Extension::ARB_sync));
            if (buffer.currentPage->sync != 0) { V(glDeleteSync(buffer.currentPage->sync)); }
            buffer.currentPage->sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            NS_ASSERT(buffer.currentPage->sync != 0);

            buffer.currentPage->next = buffer.pendingPages;
            buffer.pendingPages = buffer.currentPage;

            // If there is one free slot get it
            if (buffer.freePages != nullptr)
            {
                buffer.currentPage = buffer.freePages;
                buffer.freePages = buffer.freePages->next;
            }
            else
            {
                // Move pages already processed by GPU from pending to free
                Page** it = &buffer.pendingPages->next;
                while (*it != nullptr)
                {
                    GLenum r = glClientWaitSync((*it)->sync, 0, 0);
                    NS_ASSERT(r != GL_WAIT_FAILED);

                    if (r == GL_TIMEOUT_EXPIRED)
                    {
                        it = &((*it)->next);
                    }
                    else
                    {
                        // Once we find a processed page, the rest of pages must be also processed
                        Page* page = *it;
                        *it = nullptr;

                        NS_ASSERT(buffer.freePages == nullptr);
                        buffer.freePages = page;

                        NS_ASSERT([&]()
                        {
                            for (Page* page = buffer.freePages; page != nullptr; page = page->next)
                            {
                                if (glClientWaitSync(page->sync, 0, 0) != GL_ALREADY_SIGNALED)
                                {
                                    return false;
                                }
                            }

                            return true;
                        }());

                        break;
                    }
                }

                if (NS_LIKELY(buffer.freePages != nullptr))
                {
                    buffer.currentPage = buffer.freePages;
                    buffer.freePages = buffer.freePages->next;
                }
                else
                {
                    buffer.currentPage = AllocatePage(buffer);
                }
            }

            buffer.pos = 0;
        }

        buffer.drawPos = buffer.pos;
        buffer.pos += size;
        if (buffer.target != GL_UNIFORM_BUFFER) { BindBuffer(buffer); };
        return buffer.currentPage->base + buffer.drawPos;
    }
    else
    {
        // Buffer orphaning and no_overwrite semantics are buggy on Android, depending on the 
        // device it is very easy to have CPU stalls. We prefer to use a n-buffer scheme. It is
        // simpler and less buggy. Similar performance although less memory efficient.
        buffer.currentPage = buffer.currentPage->next;
        if (buffer.currentPage == 0) buffer.currentPage = buffer.freePages;

        BindBuffer(buffer);

        if (HaveMapBufferRange())
        {
            GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
            return MapBufferRange(buffer.target, 0, size, access);
        }
        else
        {
            buffer.pos = size;
            return buffer.cpuMemory;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::UnmapBuffer(DynamicBuffer& buffer)
{
    if (!HaveBufferStorage())
    {
        if (HaveMapBufferRange())
        {
            UnmapBuffer(buffer.target);
        }
        else
        {
            V(glBufferSubData(buffer.target, 0, buffer.pos, buffer.cpuMemory));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::CreateSamplers()
{
    memset(mSamplers, 0, sizeof(mSamplers));

    if (IsSupported(Extension::ARB_sampler_objects))
    {
        const char* MinMagStr[] = { "Nearest", "Linear" };
        const char* MipStr[] = { "Disabled", "Nearest", "Linear" };
        const char* WrapStr[] = { "ClampToEdge", "ClampToZero", "Repeat", "MirrorU", "MirrorV", "Mirror" };

        for (uint8_t minmag = 0; minmag < MinMagFilter::Count; minmag++)
        {
            for (uint8_t mip = 0; mip < MipFilter::Count; mip++)
            {
                for (uint8_t uv = 0; uv < WrapMode::Count; uv++)
                {
                    SamplerState s = {{uv, minmag, mip}};

                    NS_ASSERT(s.v < NS_COUNTOF(mSamplers));
                    V(glGenSamplers(1, &mSamplers[s.v]));

                    V(glSamplerParameteri(mSamplers[s.v], GL_TEXTURE_WRAP_S, GLWrapSMode(s)));
                    V(glSamplerParameteri(mSamplers[s.v], GL_TEXTURE_WRAP_T, GLWrapTMode(s)));
                    V(glSamplerParameteri(mSamplers[s.v], GL_TEXTURE_MIN_FILTER, GLMinFilter(s)));
                    V(glSamplerParameteri(mSamplers[s.v], GL_TEXTURE_MAG_FILTER, GLMagFilter(s)));

                  #ifdef NS_RENDERER_OPENGL
                    V(glSamplerParameterf(mSamplers[s.v], GL_TEXTURE_LOD_BIAS, -0.75f));
                  #endif

                    GL_OBJECT_LABEL(GL_SAMPLER, mSamplers[s.v], "Noesis_%s_%s_%s",
                        MinMagStr[minmag], MipStr[mip], WrapStr[uv]);
                }
            }
        }
    }
}

#if NS_RENDERER_OPENGL
    #include "Shader.140.vert.h"
    #include "Shader.140.stereo.vert.h"
    #include "Shader.140.frag.h"
#else
    #include "Shader.100es.vert.h"
    #include "Shader.300es.vert.h"
    #include "Shader.300es.stereo.vert.h"
    #include "Shader.100es.frag.h"
    #include "Shader.300es.frag.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::EnsureVsShader(uint32_t n, bool stereo)
{
    NS_ASSERT(n < NS_COUNTOF(mVsShaders));

    #define VSHADER ""
    #define VSHADER_1(x0) "#define " #x0 "\n"
    #define VSHADER_2(x0, x1) "#define " #x0 "\n#define " #x1 "\n"
    #define VSHADER_3(x0, x1, x2) "#define " #x0 "\n#define " #x1 "\n#define " #x2 "\n"
    #define VSHADER_4(x0, x1, x2, x3) "#define " #x0 "\n#define " #x1 "\n#define " #x2 "\n#define " #x3 "\n"
    #define VSHADER_5(x0, x1, x2, x3, x4) "#define " #x0 "\n#define " #x1 "\n#define " #x2 "\n#define " #x3 "\n#define " #x4 "\n"
    #define GLSLV(n) "#version " n "\n"

    auto vsShaders = stereo ? mVsShadersVR : mVsShaders;

    if (vsShaders[n] == 0)
    {
        const char* shaders[] =
        {
            VSHADER,
            VSHADER_1(HAS_COLOR),
            VSHADER_1(HAS_UV0),
            VSHADER_2(HAS_UV0, HAS_RECT),
            VSHADER_3(HAS_UV0, HAS_RECT, HAS_TILE),
            VSHADER_2(HAS_COLOR, HAS_COVERAGE),
            VSHADER_2(HAS_UV0, HAS_COVERAGE),
            VSHADER_3(HAS_UV0, HAS_COVERAGE, HAS_RECT),
            VSHADER_4(HAS_UV0, HAS_COVERAGE, HAS_RECT, HAS_TILE),
            VSHADER_3(HAS_COLOR, HAS_UV1, SDF),
            VSHADER_3(HAS_UV0, HAS_UV1, SDF),
            VSHADER_4(HAS_UV0, HAS_UV1, HAS_RECT, SDF),
            VSHADER_5(HAS_UV0, HAS_UV1, HAS_RECT, HAS_TILE, SDF),
            VSHADER_2(HAS_COLOR, HAS_UV1),
            VSHADER_2(HAS_UV0, HAS_UV1),
            VSHADER_3(HAS_UV0, HAS_UV1, HAS_RECT),
            VSHADER_4(HAS_UV0, HAS_UV1, HAS_RECT, HAS_TILE),
            VSHADER_3(HAS_COLOR, HAS_UV0, HAS_UV1),
            VSHADER_3(HAS_UV0, HAS_UV1, DOWNSAMPLE),
            VSHADER_3(HAS_COLOR, HAS_UV1, HAS_RECT),
            VSHADER_4(HAS_COLOR, HAS_UV0, HAS_RECT, HAS_IMAGE_POSITION)
        };

        const char* shaders_sRGB[] =
        {
            VSHADER,
            VSHADER_2(HAS_COLOR, SRGB),
            VSHADER_1(HAS_UV0),
            VSHADER_2(HAS_UV0, HAS_RECT),
            VSHADER_3(HAS_UV0, HAS_RECT, HAS_TILE),
            VSHADER_3(HAS_COLOR, HAS_COVERAGE, SRGB),
            VSHADER_2(HAS_UV0, HAS_COVERAGE),
            VSHADER_3(HAS_UV0, HAS_COVERAGE, HAS_RECT),
            VSHADER_4(HAS_UV0, HAS_COVERAGE, HAS_RECT, HAS_TILE),
            VSHADER_4(HAS_COLOR, HAS_UV1, SDF, SRGB),
            VSHADER_3(HAS_UV0, HAS_UV1, SDF),
            VSHADER_4(HAS_UV0, HAS_UV1, HAS_RECT, SDF),
            VSHADER_5(HAS_UV0, HAS_UV1, HAS_RECT, HAS_TILE, SDF),
            VSHADER_3(HAS_COLOR, HAS_UV1, SRGB),
            VSHADER_2(HAS_UV0, HAS_UV1),
            VSHADER_3(HAS_UV0, HAS_UV1, HAS_RECT),
            VSHADER_4(HAS_UV0, HAS_UV1, HAS_RECT, HAS_TILE),
            VSHADER_4(HAS_COLOR, HAS_UV0, HAS_UV1, SRGB),
            VSHADER_3(HAS_UV0, HAS_UV1, DOWNSAMPLE),
            VSHADER_4(HAS_COLOR, HAS_UV1, HAS_RECT, SRGB),
            VSHADER_5(HAS_COLOR, HAS_UV0, HAS_RECT, HAS_IMAGE_POSITION, SRGB)
        };

        static_assert(NS_COUNTOF(mVsShaders) == NS_COUNTOF(mVsShadersVR), "");
        static_assert(NS_COUNTOF(mVsShaders) == NS_COUNTOF(shaders), "");
        static_assert(NS_COUNTOF(mVsShaders) == NS_COUNTOF(shaders_sRGB), "");

        static const char* names[] = 
        {
            "Pos",
            "PosColor",
            "PosTex0",
            "PosTex0Rect",
            "PosTex0RectTile",
            "PosColorCoverage",
            "PosTex0Coverage",
            "PosTex0CoverageRect",
            "PosTex0CoverageRectTile",
            "PosColorTex1_SDF",
            "PosTex0Tex1_SDF",
            "PosTex0Tex1Rect_SDF",
            "PosTex0Tex1RectTile_SDF",
            "PosColorTex1",
            "PosTex0Tex1",
            "PosTex0Tex1Rect",
            "PosTex0Tex1RectTile",
            "PosColorTex0Tex1",
            "PosTex0Tex1_Downsample",
            "PosColorTex1Rect",
            "PosColorTex0RectImagePos"
        };

        static_assert(NS_COUNTOF(mVsShaders) == NS_COUNTOF(names), "");

        const char* shader = mCaps.linearRendering ? shaders_sRGB[n] : shaders[n];

        if (stereo)
        {
          #if NS_RENDERER_OPENGL
            const char* strings[] = { GLSLV("140"), shader, Shader_140_stereo_vert };
          #else
            const char* strings[] = { GLSLV("300 es"), shader, Shader_300es_stereo_vert };
          #endif
            mVsShadersVR[n] = CompileShader(GL_VERTEX_SHADER, strings, 3);
            GL_OBJECT_LABEL(GL_SHADER, mVsShadersVR[n], "Noesis_%s_Stereo_Shader", names[n]);
        }
        else
        {
          #if NS_RENDERER_OPENGL
            const char* strings[] = { GLSLV("140"), shader, Shader_140_vert };
            mVsShaders[n] = CompileShader(GL_VERTEX_SHADER, strings, 3);
          #else
            if (mGLVersion > NS_GL_VER(2, 0))
            {
                const char* strings[] = { GLSLV("300 es"), shader, Shader_300es_vert };
                mVsShaders[n] = CompileShader(GL_VERTEX_SHADER, strings, 3);
            }
            else
            {
                const char* strings[] = { GLSLV("100"), shader, Shader_100es_vert };
                mVsShaders[n] = CompileShader(GL_VERTEX_SHADER, strings, 3);
            }
          #endif

            GL_OBJECT_LABEL(GL_SHADER, mVsShaders[n], "Noesis_%s_Shader", names[n]);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static const char* FragmentSource(uint32_t n)
{
    #define FSHADER(x) "#define EFFECT_" #x "\n"
    #define FSHADER2(x, y) "#define EFFECT_" #x "\n#define PAINT_" #y "\n"
    #define FSHADER3(x, y, z) "#define EFFECT_" #x "\n#define PAINT_" #y "\n#define " #z "_PATTERN" "\n"

    switch (n)
    {
        case Shader::RGBA: return FSHADER(RGBA);
        case Shader::Mask: return FSHADER(MASK);
        case Shader::Clear: return FSHADER(CLEAR);

        case Shader::Path_Solid: return FSHADER2(PATH, SOLID);
        case Shader::Path_Linear: return FSHADER2(PATH, LINEAR);
        case Shader::Path_Radial: return FSHADER2(PATH, RADIAL);
        case Shader::Path_Pattern: return FSHADER2(PATH, PATTERN);
        case Shader::Path_Pattern_Clamp: return FSHADER3(PATH, PATTERN, CLAMP);
        case Shader::Path_Pattern_Repeat: return FSHADER3(PATH, PATTERN, REPEAT);
        case Shader::Path_Pattern_MirrorU: return FSHADER3(PATH, PATTERN, MIRRORU);
        case Shader::Path_Pattern_MirrorV: return FSHADER3(PATH, PATTERN, MIRRORV);
        case Shader::Path_Pattern_Mirror: return FSHADER3(PATH, PATTERN, MIRROR);

        case Shader::Path_AA_Solid: return FSHADER2(PATH_AA, SOLID);
        case Shader::Path_AA_Linear: return FSHADER2(PATH_AA, LINEAR);
        case Shader::Path_AA_Radial: return FSHADER2(PATH_AA, RADIAL);
        case Shader::Path_AA_Pattern: return FSHADER2(PATH_AA, PATTERN);
        case Shader::Path_AA_Pattern_Clamp: return FSHADER3(PATH_AA, PATTERN, CLAMP);
        case Shader::Path_AA_Pattern_Repeat: return FSHADER3(PATH_AA, PATTERN, REPEAT);
        case Shader::Path_AA_Pattern_MirrorU: return FSHADER3(PATH_AA, PATTERN, MIRRORU);
        case Shader::Path_AA_Pattern_MirrorV: return FSHADER3(PATH_AA, PATTERN, MIRRORV);
        case Shader::Path_AA_Pattern_Mirror: return FSHADER3(PATH_AA, PATTERN, MIRROR);

        case Shader::SDF_Solid: return FSHADER2(SDF, SOLID);
        case Shader::SDF_Linear: return FSHADER2(SDF, LINEAR);
        case Shader::SDF_Radial: return FSHADER2(SDF, RADIAL);
        case Shader::SDF_Pattern: return FSHADER2(SDF, PATTERN);
        case Shader::SDF_Pattern_Clamp: return FSHADER3(SDF, PATTERN, CLAMP);
        case Shader::SDF_Pattern_Repeat: return FSHADER3(SDF, PATTERN, REPEAT);
        case Shader::SDF_Pattern_MirrorU: return FSHADER3(SDF, PATTERN, MIRRORU);
        case Shader::SDF_Pattern_MirrorV: return FSHADER3(SDF, PATTERN, MIRRORV);
        case Shader::SDF_Pattern_Mirror: return FSHADER3(SDF, PATTERN, MIRROR);

        case Shader::Opacity_Solid: return FSHADER2(OPACITY, SOLID);
        case Shader::Opacity_Linear: return FSHADER2(OPACITY, LINEAR);
        case Shader::Opacity_Radial: return FSHADER2(OPACITY, RADIAL);
        case Shader::Opacity_Pattern: return FSHADER2(OPACITY, PATTERN);
        case Shader::Opacity_Pattern_Clamp: return FSHADER3(OPACITY, PATTERN, CLAMP);
        case Shader::Opacity_Pattern_Repeat: return FSHADER3(OPACITY, PATTERN, REPEAT);
        case Shader::Opacity_Pattern_MirrorU: return FSHADER3(OPACITY, PATTERN, MIRRORU);
        case Shader::Opacity_Pattern_MirrorV: return FSHADER3(OPACITY, PATTERN, MIRRORV);
        case Shader::Opacity_Pattern_Mirror: return FSHADER3(OPACITY, PATTERN, MIRROR);

        case Shader::Upsample: return FSHADER(UPSAMPLE);
        case Shader::Downsample: return FSHADER(DOWNSAMPLE);

        case Shader::Shadow: return FSHADER2(SHADOW, SOLID);
        case Shader::Blur: return FSHADER2(BLUR, SOLID);

        default: NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::EnsureProgram(uint32_t n, bool stereo)
{
    static_assert(NS_COUNTOF(mPrograms) == NS_COUNTOF(mProgramsVR), "");
    NS_ASSERT(n < NS_COUNTOF(mPrograms));

    auto programs = stereo ? mProgramsVR : mPrograms;
    auto vsShaders = stereo ? mVsShadersVR : mVsShaders;

    if (programs[n].object == 0)
    {
        GLuint pso;
        const char* glsl = FragmentSource(n);

    #if NS_RENDERER_OPENGL
        const char* strings[] = { GLSLV("140"), glsl, Shader_140_frag };
        pso = CompileShader(GL_FRAGMENT_SHADER, strings, 3);
    #else
        if (mGLVersion > NS_GL_VER(2, 0))
        {
            const char* strings[] = { GLSLV("300 es"), glsl, Shader_300es_frag };
            pso = CompileShader(GL_FRAGMENT_SHADER, strings, 3);
        }
        else
        {
            const char* strings[] = { GLSLV("100"), glsl, Shader_100es_frag };
            pso = CompileShader(GL_FRAGMENT_SHADER, strings, 3);
        }
    #endif

        uint32_t vsIndex = VertexForShader[n];
        EnsureVsShader(vsIndex, stereo);

        static const char* names[] = 
        {
            "RGBA",
            "Mask",
            "Clear",
            "Path_Solid",
            "Path_Linear",
            "Path_Radial",
            "Path_Pattern",
            "Path_Pattern_Clamp",
            "Path_Pattern_Repeat",
            "Path_Pattern_MirrorU",
            "Path_Pattern_MirrorV",
            "Path_Pattern_Mirror",
            "Path_AA_Solid",
            "Path_AA_Linear",
            "Path_AA_Radial",
            "Path_AA_Pattern",
            "Path_AA_Pattern_Clamp",
            "Path_AA_Pattern_Repeat",
            "Path_AA_Pattern_MirrorU",
            "Path_AA_Pattern_MirrorV",
            "Path_AA_Pattern_Mirror",
            "SDF_Solid",
            "SDF_Linear",
            "SDF_Radial",
            "SDF_Pattern",
            "SDF_Pattern_Clamp",
            "SDF_Pattern_Repeat",
            "SDF_Pattern_MirrorU",
            "SDF_Pattern_MirrorV",
            "SDF_Pattern_Mirror",
            "SDF_LCD_Solid",
            "SDF_LCD_Linear",
            "SDF_LCD_Radial",
            "SDF_LCD_Pattern",
            "SDF_LCD_Pattern_Clamp",
            "SDF_LCD_Pattern_Repeat",
            "SDF_LCD_Pattern_MirrorU",
            "SDF_LCD_Pattern_MirrorV",
            "SDF_LCD_Pattern_Mirror",
            "Opacity_Solid",
            "Opacity_Linear",
            "Opacity_Radial",
            "Opacity_Pattern",
            "Opacity_Pattern_Clamp",
            "Opacity_Pattern_Repeat",
            "Opacity_Pattern_MirrorU",
            "Opacity_Pattern_MirrorV",
            "Opacity_Pattern_Mirror",
            "Upsample",
            "Downsample",
            "Shadow",
            "Blur",
            "Custom_Effect"
        };

        static_assert(NS_COUNTOF(names) == Shader::Count, "");
        GL_OBJECT_LABEL(GL_SHADER, pso, "Noesis_%s_Shader", names[n]);

        programs[n].vertexFormat = FormatForVertex[vsIndex];
        LinkProgram(vsShaders[vsIndex], pso, programs[n], names[n]);
        DeleteShader(pso);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::CreatePrograms()
{
    memset(mVsShaders, 0, sizeof(mVsShaders));
    memset(mVsShadersVR, 0, sizeof(mVsShadersVR));
    memset(mPrograms, 0, sizeof(mPrograms));
    memset(mProgramsVR, 0, sizeof(mProgramsVR));

    // When the browser is not caching shaders (Firefox for example) it is better if we don't preload
    // shaders. In the rest of platforms like Win32 or Android, shader are cached internally without
    // having to use glGetProgramBinary or glProgramBinary

#ifndef NS_PLATFORM_EMSCRIPTEN
  #ifdef NS_PROFILE
    uint64_t t0 = HighResTimer::Ticks();
  #endif

    EnsureProgram(Shader::RGBA, false);
    EnsureProgram(Shader::Mask, false);
    EnsureProgram(Shader::Clear, false);

    EnsureProgram(Shader::Path_Solid, false);
    EnsureProgram(Shader::Path_Linear, false);
    EnsureProgram(Shader::Path_Radial, false);
    EnsureProgram(Shader::Path_Pattern, false);

    EnsureProgram(Shader::Path_AA_Solid, false);
    EnsureProgram(Shader::Path_AA_Linear, false);
    EnsureProgram(Shader::Path_AA_Radial, false);
    EnsureProgram(Shader::Path_AA_Pattern, false);

    EnsureProgram(Shader::SDF_Solid, false);
    EnsureProgram(Shader::SDF_Linear, false);
    EnsureProgram(Shader::SDF_Radial, false);
    EnsureProgram(Shader::SDF_Pattern, false);

    EnsureProgram(Shader::Opacity_Solid, false);
    EnsureProgram(Shader::Opacity_Linear, false);
    EnsureProgram(Shader::Opacity_Radial, false);
    EnsureProgram(Shader::Opacity_Pattern, false);

#if PRECREATE_VR_SHADERS
  #if NS_RENDERER_OPENGL
    if (IsSupported(Extension::AMD_vertex_shader_layer) ||
        IsSupported(Extension::ARB_shader_viewport_layer_array))
  #else
    if (IsSupported(Extension::OVR_multiview))
  #endif
    {
        EnsureProgram(Shader::RGBA, true);
        EnsureProgram(Shader::Mask, true);
        EnsureProgram(Shader::Clear, true);

        EnsureProgram(Shader::Path_Solid, true);
        EnsureProgram(Shader::Path_Linear, true);
        EnsureProgram(Shader::Path_Radial, true);
        EnsureProgram(Shader::Path_Pattern, true);

        EnsureProgram(Shader::Path_AA_Solid, true);
        EnsureProgram(Shader::Path_AA_Linear, true);
        EnsureProgram(Shader::Path_AA_Radial, true);
        EnsureProgram(Shader::Path_AA_Pattern, true);

        EnsureProgram(Shader::SDF_Solid, true);
        EnsureProgram(Shader::SDF_Linear, true);
        EnsureProgram(Shader::SDF_Radial, true);
        EnsureProgram(Shader::SDF_Pattern, true);

        EnsureProgram(Shader::Opacity_Solid, true);
        EnsureProgram(Shader::Opacity_Linear, true);
        EnsureProgram(Shader::Opacity_Radial, true);
        EnsureProgram(Shader::Opacity_Pattern, true);
    }
#endif

  #ifdef NS_PROFILE
    NS_LOG_TRACE("Shaders compiled in %.0f ms", 1000.0 * HighResTimer::Seconds(
        HighResTimer::Ticks() - t0));
  #endif
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::CreateVertexFormats()
{
    CreateBuffer(mDynamicVB, GL_ARRAY_BUFFER, DYNAMIC_VB_SIZE, "Vertices");
    CreateBuffer(mDynamicIB, GL_ELEMENT_ARRAY_BUFFER, DYNAMIC_IB_SIZE, "Indices");

  #if GL_VERSION_3_1 || GL_ES_VERSION_3_0 || GL_ARB_uniform_buffer_object
    if (IsSupported(Extension::ARB_uniform_buffer_object))
    {
        CreateBuffer(mUBO, GL_UNIFORM_BUFFER, 1024, "Constants");
        mUBOAlignment = GetInteger(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    }
  #endif

    mMaxVertexAttribs = GetInteger(GL_MAX_VERTEX_ATTRIBS);

    if (BindVertexArray_ != 0)
    {
        GenVertexArrays(1, &mDefaultVAO);
        V(BindVertexArray_(mDefaultVAO));
        GL_OBJECT_LABEL(GL_VERTEX_ARRAY, mDefaultVAO, "Noesis_Default_VAO");
    }

#if !NS_RENDERER_USE_EMS
  #if GL_VERSION_4_3 || GL_ES_VERSION_3_1 || GL_ARB_vertex_attrib_binding
    if (IsSupported(Extension::ARB_vertex_attrib_binding))
    {
        NS_ASSERT(BindVertexArray_ != 0);

        for (uint32_t i = 0; i < NS_COUNTOF(mVertexFormats); i++)
        {
            mVertexFormats[i].vao_ib = 0;
            GenVertexArrays(1, &mVertexFormats[i].vao);
            V(BindVertexArray_(mVertexFormats[i].vao));

            GLuint offset = 0;
            uint32_t attributes = AttributesForFormat[i];
            FixedString<256> label;

            for (uint32_t j = 0; j < Shader::Vertex::Format::Attr::Count; j++)
            {
                if (attributes & (1 << j))
                {
                    V(glEnableVertexAttribArray(j));
                    V(glVertexAttribBinding(j, 0));

                    uint8_t t = TypeForAttr[j];
                    V(glVertexAttribFormat(j, GLSize(t), GLType(t), GLNormalized(t), offset));
                    offset += SizeForType[t];

                    auto AttribName = [](uint32_t attr)
                    {
                        switch (attr)
                        {
                            case Shader::Vertex::Format::Attr::Pos: return "Pos";
                            case Shader::Vertex::Format::Attr::Color: return "Color";
                            case Shader::Vertex::Format::Attr::Tex0: return "Tex0";
                            case Shader::Vertex::Format::Attr::Tex1: return "Tex1";
                            case Shader::Vertex::Format::Attr::Coverage: return "Coverage";
                            case Shader::Vertex::Format::Attr::Rect: return "Rect";
                            case Shader::Vertex::Format::Attr::Tile: return "Tile";
                            case Shader::Vertex::Format::Attr::ImagePos: return "ImagePos";
                            default: NS_ASSERT_UNREACHABLE;
                        }
                    };

                    label += AttribName(j);
                }
            }

            NS_ASSERT(offset == SizeForFormat[i]);
            mVertexFormats[i].stride = SizeForFormat[i];

            GL_OBJECT_LABEL(GL_VERTEX_ARRAY, mVertexFormats[i].vao, "Noesis_%s_Array", label.Str());
        }
    }
    else
  #endif
#endif
    {
        for (uint32_t i = 0; i < NS_COUNTOF(mVertexFormats); i++)
        {
            mVertexFormats[i].vao = 0;
            mVertexFormats[i].stride = SizeForFormat[i];
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLboolean GLRenderDevice::HaveMapBufferRange() const
{
    return MapBufferRange_ != 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLvoid* GLRenderDevice::MapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length,
    GLbitfield access) const
{
    NS_ASSERT(MapBufferRange_ != 0);
    GLvoid* ptr;
    V(ptr = MapBufferRange_(target, offset, length, access));
    return ptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::UnmapBuffer(GLenum target) const
{
    NS_ASSERT(UnmapBuffer_ != 0);
    V(UnmapBuffer_(target));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLboolean GLRenderDevice::HaveBufferStorage() const
{
    return BufferStorage_ != 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BufferStorage(GLenum target, GLsizeiptr size, GLbitfield flags)
{
    NS_ASSERT(BufferStorage_ != 0);
    V(BufferStorage_(target, size, nullptr, flags));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLboolean GLRenderDevice::HaveVertexArrayObject() const
{
    return GenVertexArrays_ != 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::GenVertexArrays(GLsizei n, GLuint *arrays) const
{
    NS_ASSERT(GenVertexArrays_ != 0);
    V(GenVertexArrays_(n, arrays));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BindVertexArray(GLuint name)
{
    NS_ASSERT(BindVertexArray_ != 0);
    NS_ASSERT(name == 0 || IsVertexArray(name));

    if (name != mVertexArray)
    {
        V(BindVertexArray_(name));
        mVertexArray = name;
    }

    NS_ASSERT(GetInteger(GL_VERTEX_ARRAY_BINDING) == GLint(name));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DeleteVertexArrays(GLsizei n, const GLuint *arrays) const
{
    NS_ASSERT(DeleteVertexArrays_ != 0);
    V(DeleteVertexArrays_(n, arrays));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLboolean GLRenderDevice::IsVertexArray(GLuint array) const
{
    NS_ASSERT(IsVertexArray_ != 0);
    return IsVertexArray_(array);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLuint GLRenderDevice::RenderbufferStorage(GLsizei samples, GLenum format, GLsizei width,
    GLsizei height) const
{
    GLuint buffer;
    V(glGenRenderbuffers(1, &buffer));
    V(glBindRenderbuffer(GL_RENDERBUFFER, buffer));
    RenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, width, height);
    return buffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::RenderbufferStorageMultisample(GLenum target, GLsizei samples,
    GLenum internalformat, GLsizei width, GLsizei height) const
{
    NS_ASSERT(samples <= (GLsizei)MaxSamples());

    if (samples == 0 || RenderbufferStorageMultisample_ == 0)
    {
        // At least one device (Mali-400) gives GL_INVALID_OPERATION if we dont follow this path
        // In this device EXT_multisampled_render_to_texture is enabled but GL_MAX_SAMPLES is 0
        V(glRenderbufferStorage(target, internalformat, width, height));
    }
    else
    {
        V(RenderbufferStorageMultisample_(target, samples, internalformat, width, height));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLboolean GLRenderDevice::HaveMultisampledRenderToTexture() const
{
    return FramebufferTexture2DMultisample_ != 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::FramebufferTexture2DMultisample(GLenum target, GLenum attachment,
    GLenum textarget, GLuint texture, GLint level, GLsizei samples) const
{
    NS_ASSERT(FramebufferTexture2DMultisample_ != 0);
    V(FramebufferTexture2DMultisample_(target, attachment, textarget, texture, level, samples));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLuint GLRenderDevice::CreateStencil(const char* label, GLsizei width, GLsizei height,
    GLsizei samples) const
{
#if NS_RENDERER_OPENGL
    bool haveS8 = IsSupported(Extension::ARB_ES3_compatibility);
    GLenum format = haveS8 ? GL_STENCIL_INDEX8 : GL_DEPTH24_STENCIL8;
#else
    GLenum format = GL_STENCIL_INDEX8;
#endif

    NS_LOG_TRACE("Stencil %d x %d %dx (0x%x)", width, height, samples > 0 ? samples : 1, format);
    GLuint buffer = RenderbufferStorage(samples, format, width, height);
    GL_OBJECT_LABEL(GL_RENDERBUFFER, buffer, "Noesis_%s_Stencil", label);
    return buffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLuint GLRenderDevice::CreateColor(const char* label, GLsizei width, GLsizei height,
    GLsizei samples) const
{
#if NS_RENDERER_OPENGL
    GLenum format = mCaps.linearRendering ? GL_SRGB8_ALPHA8 : GL_RGBA8;
#else
    GLenum format = mCaps.linearRendering ? GL_SRGB8_ALPHA8 :
        IsSupported(Extension::OES_rgb8_rgba8) ? GL_RGBA8 : GL_RGB565;
#endif

    NS_LOG_TRACE("Color %d x %d %dx (0x%x)", width, height, samples, format);
    GLuint buffer = RenderbufferStorage(samples, format, width, height);
    GL_OBJECT_LABEL(GL_RENDERBUFFER, buffer, "Noesis_%s_AA", label);
    return buffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::CreateFBO(const char* label, GLsizei samples, GLuint texture, GLuint stencil,
    GLuint colorAA, GLuint& fbo, GLuint& fboResolved) const
{
    GLint prevFBO;
    V(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO));

    V(glGenFramebuffers(1, &fbo));
    V(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

    const GLenum FB = GL_FRAMEBUFFER;
    const GLenum COLOR0 = GL_COLOR_ATTACHMENT0;

    // Draw Color Buffer
    if (samples == 0)
    {
        NS_ASSERT(colorAA == 0);
        V(glFramebufferTexture2D(FB, COLOR0, GL_TEXTURE_2D, texture, 0));
    }
    else if (HaveMultisampledRenderToTexture())
    {
        NS_ASSERT(colorAA == 0);
        FramebufferTexture2DMultisample(FB, COLOR0, GL_TEXTURE_2D, texture, 0, samples);
    }
    else
    {
        NS_ASSERT(colorAA != 0);
        V(glFramebufferRenderbuffer(FB, COLOR0, GL_RENDERBUFFER, colorAA));
    }

    // Stencil Buffer
    if (stencil != 0)
    {
    #if NS_RENDERER_OPENGL
        bool haveS8 = IsSupported(Extension::ARB_ES3_compatibility);
        GLenum attachment = haveS8 ? GL_STENCIL_ATTACHMENT : GL_DEPTH_STENCIL_ATTACHMENT;
    #else
        GLenum attachment = GL_STENCIL_ATTACHMENT;
    #endif
        V(glFramebufferRenderbuffer(FB, attachment, GL_RENDERBUFFER, stencil));
    }

    NS_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    GL_OBJECT_LABEL(GL_FRAMEBUFFER, fbo, "Noesis_%s_FBO", label);

    // Clear surface to always start with #0000 color
    // This is also needed to avoid crashes on Google Pixel 6 & 7 devices (#2450)
    V(glDisable(GL_SCISSOR_TEST));
    V(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
    V(glClear(GL_COLOR_BUFFER_BIT));

    // Resolve framebuffer
    if (colorAA != 0)
    {
        V(glGenFramebuffers(1, &fboResolved));
        V(glBindFramebuffer(GL_FRAMEBUFFER, fboResolved));
        V(glFramebufferTexture2D(FB, COLOR0, GL_TEXTURE_2D, texture, 0));
        NS_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        GL_OBJECT_LABEL(GL_FRAMEBUFFER, fboResolved, "Noesis_%s_FBO_Resolve", label);
    }

    V(glBindFramebuffer(GL_FRAMEBUFFER, prevFBO));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::Resolve(GLRenderTarget* target, const Tile* tiles, uint32_t numTiles)
{
    NS_ASSERT(GetInteger(GL_FRAMEBUFFER_BINDING) == (GLint)target->fbo);

    if (target->fboResolved != 0)
    {
        V(glBindFramebuffer(GL_READ_FRAMEBUFFER, target->fbo));
        V(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target->fboResolved));
        mBoundRenderTarget = 0;

        for (uint32_t i = 0; i < numTiles; i++)
        {
            const Tile& tile = tiles[i];
            V(glScissor(tile.x, tile.y, tile.width, tile.height));

        #if NS_RENDERER_USE_EAGL
            if (IsSupported(Extension::APPLE_framebuffer_multisample))
            {
                V(glResolveMultisampleFramebufferAPPLE());
            }
            else
        #endif
            {
                GLint x0 = tile.x;
                GLint y0 = tile.y;
                GLint x1 = tile.x + tile.width;
                GLint y1 = tile.y + tile.height;

                NS_ASSERT(HaveBlitFramebuffer());
                BlitFramebuffer(x0, y0, x1, y1, x0, y0, x1, y1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            }
        }

        GLenum attachments[] = { GL_COLOR_ATTACHMENT0, GL_STENCIL_ATTACHMENT };
        DiscardFramebuffer(GL_READ_FRAMEBUFFER, 2, attachments);
    }
    else
    {
        GLenum attachments[] = { GL_STENCIL_ATTACHMENT };
        DiscardFramebuffer(GL_FRAMEBUFFER, 1, attachments);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BindRenderTarget(GLRenderTarget* renderTarget)
{
    if (renderTarget != mBoundRenderTarget)
    {
        mBoundRenderTarget = renderTarget;

        if (mBoundRenderTarget != 0)
        {
            NS_ASSERT(mBoundRenderTarget->fbo == 0 || glIsFramebuffer(mBoundRenderTarget->fbo));
            V(glBindFramebuffer(GL_FRAMEBUFFER, mBoundRenderTarget->fbo));
            Viewport(mBoundRenderTarget->width, mBoundRenderTarget->height);
        }
    }

    NS_ASSERT(!renderTarget || GetInteger(GL_FRAMEBUFFER_BINDING) == (GLint)renderTarget->fbo);
    NS_ASSERT(!renderTarget || GetInteger(GL_VIEWPORT, 0) == (GLint)0);
    NS_ASSERT(!renderTarget || GetInteger(GL_VIEWPORT, 1) == (GLint)0);
    NS_ASSERT(!renderTarget || GetInteger(GL_VIEWPORT, 2) == (GLint)renderTarget->width);
    NS_ASSERT(!renderTarget || GetInteger(GL_VIEWPORT, 3) == (GLint)renderTarget->height);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DiscardFramebuffer(GLenum target, GLsizei numAttachments,
    const GLenum* attachments) const
{
    if (InvalidateFramebuffer_ != 0)
    {
        V(InvalidateFramebuffer_(target, numAttachments, attachments));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool GLRenderDevice::HaveBlitFramebuffer() const
{
    return BlitFramebuffer_ != 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
    GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) const
{
    NS_ASSERT(BlitFramebuffer_ != 0);
    V(BlitFramebuffer_(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DeleteFramebuffer(GLuint framebuffer) const
{
    NS_ASSERT(glIsFramebuffer(framebuffer));
    V(glDeleteFramebuffers(1, &framebuffer));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DeleteRenderbuffer(GLuint renderbuffer) const
{
    NS_ASSERT(glIsRenderbuffer(renderbuffer));
    V(glDeleteRenderbuffers(1, &renderbuffer));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool GLRenderDevice::HaveBorderClamp() const
{
    return IsSupported(Extension::ARB_texture_border_clamp) || IsSupported(
        Extension::EXT_texture_border_clamp);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool GLRenderDevice::HaveTexStorage() const
{
    return TexStorage2D_ != 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool GLRenderDevice::HaveTexRG() const
{
    return IsSupported(Extension::ARB_texture_rg) || IsSupported(Extension::EXT_texture_rg);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::GetGLFormat(TextureFormat::Enum texFormat, GLenum& internalFormat,
    GLenum& format)
{
    switch (texFormat)
    {
        case TextureFormat::RGBA8:
        case TextureFormat::RGBX8:
        {
            if (mCaps.linearRendering)
            {
                NS_ASSERT(HaveTexStorage());
                internalFormat = GL_SRGB8_ALPHA8;
                format = GL_RGBA;
            }
            else
            {
                internalFormat = HaveTexStorage() ? GL_RGBA8 : GL_RGBA;
                format = GL_RGBA;
            }

            break;
        }
        case TextureFormat::R8:
        {
            if (HaveTexRG())
            {
                internalFormat = HaveTexStorage() ? GL_R8 : GL_RED;
                format = GL_RED;
            }
            else
            {
                internalFormat = HaveTexStorage() ? GL_LUMINANCE8 : GL_LUMINANCE;
                format = GL_LUMINANCE;
            }
            break;
        }
        default:
            NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLint GLRenderDevice::GLWrapSMode(SamplerState sampler) const
{
    switch (sampler.f.wrapMode)
    {
        case WrapMode::ClampToEdge:
        {
            return GL_CLAMP_TO_EDGE;
        }
        case WrapMode::ClampToZero:
        {
            return HaveBorderClamp() ? GL_CLAMP_TO_BORDER : GL_CLAMP_TO_EDGE;
        }
        case WrapMode::Repeat:
        {
            return GL_REPEAT;
        }
        case WrapMode::MirrorU:
        {
            return GL_MIRRORED_REPEAT;
        }
        case WrapMode::MirrorV:
        {
            return GL_REPEAT;
        }
        case WrapMode::Mirror:
        {
            return GL_MIRRORED_REPEAT;
        }
        default:
            NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLint GLRenderDevice::GLWrapTMode(SamplerState sampler) const
{
    switch (sampler.f.wrapMode)
    {
        case WrapMode::ClampToEdge:
        {
            return GL_CLAMP_TO_EDGE;
            break;
        }
        case WrapMode::ClampToZero:
        {
            return HaveBorderClamp() ? GL_CLAMP_TO_BORDER : GL_CLAMP_TO_EDGE;
            break;
        }
        case WrapMode::Repeat:
        {
            return GL_REPEAT;
            break;
        }
        case WrapMode::MirrorU:
        {
            return GL_REPEAT;
            break;
        }
        case WrapMode::MirrorV:
        {
            return GL_MIRRORED_REPEAT;
            break;
        }
        case WrapMode::Mirror:
        {
            return GL_MIRRORED_REPEAT;
            break;
        }
        default:
            NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLint GLRenderDevice::GLMinFilter(SamplerState sampler) const
{
    switch (sampler.f.minmagFilter)
    {
        case MinMagFilter::Nearest:
        {
            switch (sampler.f.mipFilter)
            {
                case MipFilter::Disabled:
                {
                    return GL_NEAREST;
                }
                case MipFilter::Nearest:
                {
                    return GL_NEAREST_MIPMAP_NEAREST;
                }
                case MipFilter::Linear:
                {
                    return GL_NEAREST_MIPMAP_LINEAR;
                }
                default:
                {
                    NS_ASSERT_UNREACHABLE;
                }
            }
        }
        case MinMagFilter::Linear:
        {
            switch (sampler.f.mipFilter)
            {
                case MipFilter::Disabled:
                {
                    return GL_LINEAR;
                }
                case MipFilter::Nearest:
                {
                    return GL_LINEAR_MIPMAP_NEAREST;
                }
                case MipFilter::Linear:
                {
                    return GL_LINEAR_MIPMAP_LINEAR;
                }
                default:
                {
                    NS_ASSERT_UNREACHABLE;
                }
            }
        }
        default:
        {
            NS_ASSERT_UNREACHABLE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLint GLRenderDevice::GLMagFilter(SamplerState sampler) const
{
    switch (sampler.f.minmagFilter)
    {
        case MinMagFilter::Nearest:
        {
            return GL_NEAREST;
        }
        case MinMagFilter::Linear:
        {
            return GL_LINEAR;
        }
        default:
            NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat,
    GLsizei width, GLsizei height)
{
    NS_ASSERT(TexStorage2D_ != 0);
    V(TexStorage2D_(target, levels, internalformat, width, height));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::InitTexture(const char* label, GLTexture* texture, GLenum internalFormat,
    GLenum format)
{
    V(glGenTextures(1, &texture->object));

    SamplerState state = {{ WrapMode::Repeat, MinMagFilter::Linear, MipFilter::Nearest }};
    uint32_t levels = texture->levels;

    if (levels == 1)
    {
        // Hint the driver to avoid allocating all the mipmaps
        state.f.mipFilter = MipFilter::Disabled;
    }

    // Force setting all parameters
    texture->state.v = ~state.v;

    ActiveTexture(0);
    BindTexture2D(texture->object);
    SetTextureState(texture, state);

  #ifdef NS_RENDERER_OPENGL
    V(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.75f));
  #endif

    if (IsSupported(Extension::APPLE_texture_max_level))
    {
        V(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, levels - 1));
    }

    uint32_t width = texture->width;
    uint32_t height = texture->height;

    NS_LOG_TRACE("Texture %d x %d x %d (0x%x)", width, height, levels, internalFormat);

    if (!HaveTexStorage())
    {
        for (uint32_t i = 0; i < levels; i++)
        {
            V(glTexImage2D(GL_TEXTURE_2D, i, internalFormat, width, height, 0, format,
                GL_UNSIGNED_BYTE, 0));
            width = Max(1U, width / 2);
            height = Max(1U, height / 2);
        }
    }
    else
    {
        TexStorage2D(GL_TEXTURE_2D, levels, internalFormat, width, height);
    }

    GL_OBJECT_LABEL(GL_TEXTURE, texture->object, "Noesis_%s", label);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::SetTextureState(GLTexture* texture, SamplerState state)
{
    if (IsSupported(Extension::ARB_sampler_objects))
    {
        NS_ASSERT(state.v < NS_COUNTOF(mSamplers));
        BindSampler(mSamplers[state.v]);
    }
    else
    {
        SamplerState delta;
        delta.v = texture->state.v ^ state.v;

        if (delta.v != 0)
        {
            if (delta.f.wrapMode != 0)
            {
                V(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GLWrapSMode(state)));
                V(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GLWrapTMode(state)));
            }

            if (delta.f.minmagFilter != 0 || delta.f.mipFilter != 0)
            {
                V(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GLMinFilter(state)));
                V(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GLMagFilter(state)));
            }
        }

        texture->state = state;

        NS_ASSERT(GetTexParameter(GL_TEXTURE_WRAP_S) == GLWrapSMode(state));
        NS_ASSERT(GetTexParameter(GL_TEXTURE_WRAP_T) == GLWrapTMode(state));
        NS_ASSERT(GetTexParameter(GL_TEXTURE_MIN_FILTER) == GLMinFilter(state));
        NS_ASSERT(GetTexParameter(GL_TEXTURE_MAG_FILTER) == GLMagFilter(state));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::SetTexture(uint32_t unit, GLTexture* texture, SamplerState state)
{
    NS_ASSERT(unit < NS_COUNTOF(mBoundTextures));
    NS_ASSERT(texture->object == 0 || glIsTexture(texture->object));

    bool cached = mBoundTextures[unit] == texture->object;

    if (IsSupported(Extension::ARB_sampler_objects))
    {
        cached = cached && mSamplers[state.v] != 0 && mSamplers[state.v] == mBoundSamplers[unit];
    }
    else
    {
        // As the state of external textures is not known we set it everytime
        if (texture->device == 0)
        {
            texture->state.v = ~state.v;
        }

        cached = cached && texture->state.v == state.v;
    }

    if (!cached)
    {
        ActiveTexture(unit);
        BindTexture2D(texture->object);
        SetTextureState(texture, state);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::SetTextures(const Batch& batch)
{
    if (batch.pattern != 0)
    {
        SetTexture(TextureUnit::Pattern, (GLTexture*)batch.pattern, batch.patternSampler);
    }

    if (batch.ramps != 0)
    {
        SetTexture(TextureUnit::Ramps, (GLTexture*)batch.ramps, batch.rampsSampler);
    }

    if (batch.image != 0)
    {
        SetTexture(TextureUnit::Image, (GLTexture*)batch.image, batch.imageSampler);
    }

    if (batch.glyphs != 0)
    {
        SetTexture(TextureUnit::Glyphs, (GLTexture*)batch.glyphs, batch.glyphsSampler);
    }

    if (batch.shadow != 0)
    {
        SetTexture(TextureUnit::Shadow, (GLTexture*)batch.shadow, batch.shadowSampler);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::ActiveTexture(uint32_t unit)
{
    if (mActiveTextureUnit != unit)
    {
        V(glActiveTexture(GL_TEXTURE0 + unit));
        mActiveTextureUnit = unit;
    }

    NS_ASSERT(GetInteger(GL_ACTIVE_TEXTURE) == GL_TEXTURE0 + (GLint)unit);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BindTexture2D(GLuint texture)
{
    NS_ASSERT(mActiveTextureUnit < NS_COUNTOF(mBoundTextures));

    if (mBoundTextures[mActiveTextureUnit] != texture)
    {
        V(glBindTexture(GL_TEXTURE_2D, texture));
        mBoundTextures[mActiveTextureUnit] = texture;
    }

    NS_ASSERT(GetInteger(GL_TEXTURE_BINDING_2D) == (GLint)texture);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::SetUnpackState()
{
    if (!mUnpackStateSet)
    {
        V(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

#if NS_RENDERER_OPENGL
        V(glPixelStorei(GL_UNPACK_SWAP_BYTES, 0));
        V(glPixelStorei(GL_UNPACK_LSB_FIRST, 0));
        V(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
        V(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
        V(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));
#endif

        mUnpackStateSet = true;
    }

    NS_ASSERT(GetInteger(GL_UNPACK_ALIGNMENT) == 1);

#if NS_RENDERER_OPENGL
    NS_ASSERT(GetInteger(GL_UNPACK_SWAP_BYTES) == 0);
    NS_ASSERT(GetInteger(GL_UNPACK_LSB_FIRST) == 0);
    NS_ASSERT(GetInteger(GL_UNPACK_ROW_LENGTH) == 0);
    NS_ASSERT(GetInteger(GL_UNPACK_SKIP_PIXELS) == 0);
    NS_ASSERT(GetInteger(GL_UNPACK_SKIP_ROWS) == 0);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::TexSubImage2D(GLint level, GLint xOffset, GLint yOffset, GLint width,
    GLint height, GLenum format, const GLvoid* data)
{
    SetUnpackState();
    V(glTexSubImage2D(GL_TEXTURE_2D, level, xOffset, yOffset, width, height, format,
        GL_UNSIGNED_BYTE, data));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BindSampler(GLuint sampler)
{
    NS_ASSERT(glIsSampler(sampler));
    NS_ASSERT(mActiveTextureUnit < NS_COUNTOF(mBoundSamplers));
    NS_ASSERT(GetInteger(GL_ACTIVE_TEXTURE) == GL_TEXTURE0 + (GLint)mActiveTextureUnit);

    if (mBoundSamplers[mActiveTextureUnit] != sampler)
    {
        V(glBindSampler(mActiveTextureUnit, sampler));
        mBoundSamplers[mActiveTextureUnit] = sampler;
    }

    NS_ASSERT(GetInteger(GL_SAMPLER_BINDING) == (GLint)sampler);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DeleteSampler(GLuint sampler) const
{
    NS_ASSERT(glIsSampler(sampler));
    V(glDeleteSamplers(1, &sampler));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DeleteTexture(GLuint texture) const
{
    NS_ASSERT(glIsTexture(texture));
    V(glDeleteTextures(1, &texture));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLuint GLRenderDevice::CompileShader(GLenum type, const char** strings, uint32_t count) const
{
    GLuint shader = glCreateShader(type);
    V(glShaderSource(shader, count, strings, NULL));
    V(glCompileShader(shader));

    GLint status;
    V(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
    if (status == 0)
    {
        GLint len;
        V(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len));

        Vector<char, 512> error;
        error.Reserve(len);
        V(glGetShaderInfoLog(shader, len, 0, (GLchar*)error.Begin()));
        error.ForceSize(len);

        NS_ERROR("Shader compilation failed:\n%s", (const char*)error.Data());
    }

    return shader;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::LinkProgram(GLuint vso, GLuint pso, ProgramInfo& p, const char* label)
{
    GLuint po = glCreateProgram();
    V(glAttachShader(po, vso));
    V(glAttachShader(po, pso));

    V(glBindAttribLocation(po, 0, "attr_pos"));
    V(glBindAttribLocation(po, 1, "attr_color"));
    V(glBindAttribLocation(po, 2, "attr_uv0"));
    V(glBindAttribLocation(po, 3, "attr_uv1"));
    V(glBindAttribLocation(po, 4, "attr_coverage"));
    V(glBindAttribLocation(po, 5, "attr_rect"));
    V(glBindAttribLocation(po, 6, "attr_tile"));
    V(glBindAttribLocation(po, 7, "attr_imagePos"));

    V(glLinkProgram(po));
    V(glDetachShader(po, vso));
    V(glDetachShader(po, pso));

    GLint status;
    V(glGetProgramiv(po, GL_LINK_STATUS, &status));
    if (status == 0)
    {
        GLint len;
        V(glGetProgramiv(po, GL_INFO_LOG_LENGTH, &len));

        Vector<char, 512> error;
        error.Reserve(len);
        V(glGetProgramInfoLog(po, len, 0, (GLchar*)error.Begin()));
        error.ForceSize(len);

        NS_ERROR("Link program failed: %s", (const char*)error.Data());
        return;
    }

    GL_OBJECT_LABEL(GL_PROGRAM, po, "Noesis_%s_Program", label);
    p.object = po;

    // Bind textures

    BindTextureLocation(po, "pattern", TextureUnit::Pattern);
    BindTextureLocation(po, "ramps", TextureUnit::Ramps);
    BindTextureLocation(po, "image", TextureUnit::Image);
    BindTextureLocation(po, "glyphs", TextureUnit::Glyphs);
    BindTextureLocation(po, "shadow", TextureUnit::Shadow);

    // Bind uniforms

  #if GL_VERSION_3_1 || GL_ES_VERSION_3_0 || GL_ARB_uniform_buffer_object
    p.uboIndex = GL_INVALID_INDEX;

    if (IsSupported(Extension::ARB_uniform_buffer_object))
    {
        // We are only using Uniform Buffers for custom pixel shaders
        p.uboIndex = glGetUniformBlockIndex(p.object, "Buffer1_ps");
        if (p.uboIndex != GL_INVALID_INDEX)
        {
            glUniformBlockBinding(p.object, p.uboIndex, 0);
        }
    }
  #endif

    p.cbufferLocation[0] = glGetUniformLocation(p.object, "cbuffer0_vs");
    p.cbufferHash[0] = 0;

    p.cbufferLocation[1] = glGetUniformLocation(p.object, "cbuffer1_vs");
    p.cbufferHash[1] = 0;

    p.cbufferLocation[2] = glGetUniformLocation(p.object, "cbuffer0_ps");
    p.cbufferHash[2] = 0;

    p.cbufferLocation[3] = glGetUniformLocation(p.object, "cbuffer1_ps");
    p.cbufferHash[3] = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BindTextureLocation(GLuint po, const char* name, GLint unit)
{
    GLint location = glGetUniformLocation(po, name);
    if (location != -1)
    {
        UseProgram(po);
        V(glUniform1i(location, unit));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static uint32_t Align(uint32_t n, uint32_t alignment)
{
    NS_ASSERT(IsPow2(alignment));
    return (n + (alignment - 1)) & ~(alignment - 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::UploadUniforms(ProgramInfo* program, uint32_t index, const UniformData& data)
{
  #if GL_VERSION_3_1 || GL_ES_VERSION_3_0 || GL_ARB_uniform_buffer_object
    if (index == 3 && program->uboIndex != GL_INVALID_INDEX)
    {
        // For custom pixel shaders upload uniforms using a Uniform Buffer Object
        if (data.numDwords > 0 && data.hash != mUBOHash)
        {
            uint32_t size = data.numDwords * sizeof(uint32_t);
            void* ptr = MapBuffer(mUBO, Align(size, (uint32_t)mUBOAlignment));
            memcpy(ptr, data.values, size);
            UnmapBuffer(mUBO);

            V(glBindBufferRange(GL_UNIFORM_BUFFER, 0, mUBO.currentPage->object, mUBO.drawPos, size));
            mUBOHash = data.hash;
        }
    }
    else
  #endif
    {
        if (program->cbufferLocation[index] != -1 && program->cbufferHash[index] != data.hash)
        {
            V(glUniform1fv(program->cbufferLocation[index], data.numDwords, (float*)data.values));
            program->cbufferHash[index] = data.hash;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::UseProgram(GLuint name)
{
    NS_ASSERT(name != 0);
    NS_ASSERT(glIsProgram(name));

    if (mProgram != name)
    {
        V(glUseProgram(name));
        mProgram = name;
    }

    NS_ASSERT(GetInteger(GL_CURRENT_PROGRAM) == GLint(name));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DeleteShader(GLuint shader) const
{
    NS_ASSERT(glIsShader(shader));
    V(glDeleteShader(shader));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DeleteProgram(GLuint program) const
{
    NS_ASSERT(glIsProgram(program));
    V(glDeleteProgram(program));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BlendEnable()
{
    if (mBlendEnabled != 1)
    {
        V(glEnable(GL_BLEND));
        mBlendEnabled = 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BlendDisable()
{
    if (mBlendEnabled != 0)
    {
        V(glDisable(GL_BLEND));
        mBlendEnabled = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BlendFuncSrcOver()
{
    if (mBlendFunc != BlendMode::SrcOver)
    {
        V(glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
        mBlendFunc = BlendMode::SrcOver;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BlendFuncSrcOverMultiply()
{
    if (mBlendFunc != BlendMode::SrcOver_Multiply)
    {
        V(glBlendFuncSeparate(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
        mBlendFunc = BlendMode::SrcOver_Multiply;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BlendFuncSrcOverScreen()
{
    if (mBlendFunc != BlendMode::SrcOver_Screen)
    {
        V(glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_COLOR, GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
        mBlendFunc = BlendMode::SrcOver_Screen;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BlendFuncSrcOverAdditive()
{
    if (mBlendFunc != BlendMode::SrcOver_Additive)
    {
        V(glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
        mBlendFunc = BlendMode::SrcOver_Additive;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::StencilEnable()
{
    if (mStencilEnabled != 1)
    {
        V(glEnable(GL_STENCIL_TEST));
        mStencilEnabled = 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::StencilDisable()
{
    if (mStencilEnabled != 0)
    {
        V(glDisable(GL_STENCIL_TEST));
        mStencilEnabled = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DepthEnable()
{
    if (mDepthEnabled != 1)
    {
        V(glEnable(GL_DEPTH_TEST));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DepthDisable()
{
    if (mDepthEnabled != 0)
    {
        V(glDisable(GL_DEPTH_TEST));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::StencilOpKeep(uint8_t stencilRef)
{
    if (mStencilOp != StencilMode::Equal_Keep)
    {
        V(glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP));
        mStencilOp = StencilMode::Equal_Keep;
    }

    if (stencilRef != mStencilRef)
    {
        V(glStencilFunc(GL_EQUAL, stencilRef, 0xff));
        mStencilRef = stencilRef;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::StencilOpIncr(uint8_t stencilRef)
{
    if (mStencilOp != StencilMode::Equal_Incr)
    {
        V(glStencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP));
        mStencilOp = StencilMode::Equal_Incr;
    }

    if (stencilRef != mStencilRef)
    {
        V(glStencilFunc(GL_EQUAL, stencilRef, 0xff));
        mStencilRef = stencilRef;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::StencilOpDecr(uint8_t stencilRef)
{
    if (mStencilOp != StencilMode::Equal_Decr)
    {
        V(glStencilOp(GL_KEEP, GL_KEEP, GL_DECR_WRAP));
        mStencilOp = StencilMode::Equal_Decr;
    }

    if (stencilRef != mStencilRef)
    {
        V(glStencilFunc(GL_EQUAL, stencilRef, 0xff));
        mStencilRef = stencilRef;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::StencilOpClear()
{
    if (mStencilOp != StencilMode::Clear)
    {
        V(glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO));
        mStencilOp = StencilMode::Clear;
    }

    if (mStencilRef != 0xFFFF)
    {
        V(glStencilFunc(GL_ALWAYS, 0, 0xff));
        mStencilRef = 0xFFFF;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::SetRenderState(RenderState state, uint8_t stencilRef)
{
    RenderState delta;
    delta.v = mStencilRef == INVALIDATED_RENDER_STATE ? 0xff : state.v ^ mRenderState.v;

    if (delta.v != 0 || mStencilRef != stencilRef)
    {
        if (delta.f.colorEnable != 0)
        {
            GLboolean enable = state.f.colorEnable > 0;
            V(glColorMask(enable, enable, enable, enable));
        }

        if (delta.f.blendMode != 0)
        {
            switch (state.f.blendMode)
            {
                case BlendMode::Src:
                {
                    BlendDisable();
                    break;
                }
                case BlendMode::SrcOver:
                {
                    BlendEnable();
                    BlendFuncSrcOver();
                    break;
                }
                case BlendMode::SrcOver_Multiply:
                {
                    BlendEnable();
                    BlendFuncSrcOverMultiply();
                    break;
                }
                case BlendMode::SrcOver_Screen:
                {
                    BlendEnable();
                    BlendFuncSrcOverScreen();
                    break;
                }
                case BlendMode::SrcOver_Additive:
                {
                    BlendEnable();
                    BlendFuncSrcOverAdditive();
                    break;
                }
                default:
                    NS_ASSERT_UNREACHABLE;
            }
        }

        if (delta.f.stencilMode != 0 || mStencilRef != stencilRef)
        {
            switch (state.f.stencilMode)
            {
                case StencilMode::Disabled:
                {
                    DepthDisable();
                    StencilDisable();
                    break;
                }
                case StencilMode::Equal_Keep:
                {
                    DepthDisable();
                    StencilEnable();
                    StencilOpKeep(stencilRef);
                    break;
                }
                case StencilMode::Equal_Incr:
                {
                    DepthDisable();
                    StencilEnable();
                    StencilOpIncr(stencilRef);
                    break;
                }
                case StencilMode::Equal_Decr:
                {
                    DepthDisable();
                    StencilEnable();
                    StencilOpDecr(stencilRef);
                    break;
                }
                case StencilMode::Clear:
                {
                    DepthDisable();
                    StencilEnable();
                    StencilOpClear();
                    break;
                }
                case StencilMode::Disabled_ZTest:
                {
                    DepthEnable();
                    StencilDisable();
                    break;
                }
                case StencilMode::Equal_Keep_ZTest:
                {
                    DepthEnable();
                    StencilEnable();
                    StencilOpKeep(stencilRef);
                    break;
                }
                default:
                {
                    NS_ASSERT_UNREACHABLE;
                }
            }
        }

        mRenderState = state;
    }

    NS_ASSERT(GetBoolean(GL_COLOR_WRITEMASK, 0) == (state.f.colorEnable > 0));
    NS_ASSERT(GetBoolean(GL_COLOR_WRITEMASK, 1) == (state.f.colorEnable > 0));
    NS_ASSERT(GetBoolean(GL_COLOR_WRITEMASK, 2) == (state.f.colorEnable > 0));
    NS_ASSERT(GetBoolean(GL_COLOR_WRITEMASK, 3) == (state.f.colorEnable > 0));
    NS_ASSERT(GetInteger(GL_DEPTH_FUNC) == GL_LEQUAL);

    NS_ASSERT([&]()
    {
        switch (state.f.blendMode)
        {
            case BlendMode::Src:
            {
                return !glIsEnabled(GL_BLEND);
            }
            case BlendMode::SrcOver:
            {
                return glIsEnabled(GL_BLEND) &&
                    GetInteger(GL_BLEND_SRC_RGB) == GL_ONE &&
                    GetInteger(GL_BLEND_DST_RGB) == GL_ONE_MINUS_SRC_ALPHA &&
                    GetInteger(GL_BLEND_SRC_ALPHA) == GL_ONE &&
                    GetInteger(GL_BLEND_DST_ALPHA) == GL_ONE_MINUS_SRC_ALPHA;
            }
            case BlendMode::SrcOver_Multiply:
            {
                return glIsEnabled(GL_BLEND) &&
                    GetInteger(GL_BLEND_SRC_RGB) == GL_DST_COLOR &&
                    GetInteger(GL_BLEND_DST_RGB) == GL_ONE_MINUS_SRC_ALPHA &&
                    GetInteger(GL_BLEND_SRC_ALPHA) == GL_ONE &&
                    GetInteger(GL_BLEND_DST_ALPHA) == GL_ONE_MINUS_SRC_ALPHA;
            }
            case BlendMode::SrcOver_Screen:
            {
                return glIsEnabled(GL_BLEND) &&
                    GetInteger(GL_BLEND_SRC_RGB) == GL_ONE &&
                    GetInteger(GL_BLEND_DST_RGB) == GL_ONE_MINUS_SRC_COLOR &&
                    GetInteger(GL_BLEND_SRC_ALPHA) == GL_ONE &&
                    GetInteger(GL_BLEND_DST_ALPHA) == GL_ONE_MINUS_SRC_ALPHA;
            }
            case BlendMode::SrcOver_Additive:
            {
                return glIsEnabled(GL_BLEND) &&
                    GetInteger(GL_BLEND_SRC_RGB) == GL_ONE &&
                    GetInteger(GL_BLEND_DST_RGB) == GL_ONE &&
                    GetInteger(GL_BLEND_SRC_ALPHA) == GL_ONE &&
                    GetInteger(GL_BLEND_DST_ALPHA) == GL_ONE_MINUS_SRC_ALPHA;
            }
            default:
                return false;
        }
    }());

    NS_ASSERT([&]()
    {
        switch (state.f.stencilMode)
        {
            case StencilMode::Disabled:
            {
                return !glIsEnabled(GL_STENCIL_TEST) && !glIsEnabled(GL_DEPTH_TEST);
            }
            case StencilMode::Equal_Keep:
            {
                return glIsEnabled(GL_STENCIL_TEST) && !glIsEnabled(GL_DEPTH_TEST) &&
                    GetInteger(GL_STENCIL_FAIL) == GL_KEEP &&
                    GetInteger(GL_STENCIL_PASS_DEPTH_FAIL) == GL_KEEP &&
                    GetInteger(GL_STENCIL_PASS_DEPTH_PASS) == GL_KEEP &&
                    GetInteger(GL_STENCIL_FUNC) == GL_EQUAL &&
                    GetInteger(GL_STENCIL_REF) == (GLint)stencilRef &&
                    GetInteger(GL_STENCIL_VALUE_MASK) == 0xff;
            }
            case StencilMode::Equal_Incr:
            {
                return glIsEnabled(GL_STENCIL_TEST) && !glIsEnabled(GL_DEPTH_TEST) &&
                    GetInteger(GL_STENCIL_FAIL) == GL_KEEP &&
                    GetInteger(GL_STENCIL_PASS_DEPTH_FAIL) == GL_KEEP &&
                    GetInteger(GL_STENCIL_PASS_DEPTH_PASS) == GL_INCR_WRAP &&
                    GetInteger(GL_STENCIL_FUNC) == GL_EQUAL &&
                    GetInteger(GL_STENCIL_REF) == (GLint)stencilRef &&
                    GetInteger(GL_STENCIL_VALUE_MASK) == 0xff;
            }
            case StencilMode::Equal_Decr:
            {
                return glIsEnabled(GL_STENCIL_TEST) && !glIsEnabled(GL_DEPTH_TEST) &&
                    GetInteger(GL_STENCIL_FAIL) == GL_KEEP &&
                    GetInteger(GL_STENCIL_PASS_DEPTH_FAIL) == GL_KEEP &&
                    GetInteger(GL_STENCIL_PASS_DEPTH_PASS) == GL_DECR_WRAP &&
                    GetInteger(GL_STENCIL_FUNC) == GL_EQUAL &&
                    GetInteger(GL_STENCIL_REF) == (GLint)stencilRef &&
                    GetInteger(GL_STENCIL_VALUE_MASK) == 0xff;
            }
            case StencilMode::Clear:
            {
                return glIsEnabled(GL_STENCIL_TEST) && !glIsEnabled(GL_DEPTH_TEST) &&
                    GetInteger(GL_STENCIL_FAIL) == GL_ZERO &&
                    GetInteger(GL_STENCIL_PASS_DEPTH_FAIL) == GL_ZERO &&
                    GetInteger(GL_STENCIL_PASS_DEPTH_PASS) == GL_ZERO &&
                    GetInteger(GL_STENCIL_FUNC) == GL_ALWAYS;
            }
            case StencilMode::Disabled_ZTest:
            {
                return !glIsEnabled(GL_STENCIL_TEST) && glIsEnabled(GL_DEPTH_TEST);
            }
            case StencilMode::Equal_Keep_ZTest:
            {
                return glIsEnabled(GL_STENCIL_TEST) && glIsEnabled(GL_DEPTH_TEST) &&
                    GetInteger(GL_STENCIL_FAIL) == GL_KEEP &&
                    GetInteger(GL_STENCIL_PASS_DEPTH_FAIL) == GL_KEEP &&
                    GetInteger(GL_STENCIL_PASS_DEPTH_PASS) == GL_KEEP &&
                    GetInteger(GL_STENCIL_FUNC) == GL_EQUAL &&
                    GetInteger(GL_STENCIL_REF) == (GLint)stencilRef &&
                    GetInteger(GL_STENCIL_VALUE_MASK) == 0xff;
            }

            default:
                return false;
        }
    }());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::Viewport(uint32_t width, uint32_t height)
{
    if (mViewportWidth != width || mViewportHeight != height)
    {
        V(glViewport(0, 0, width, height));
        mViewportWidth = width;
        mViewportHeight = height;
    }

    NS_ASSERT(GetInteger(GL_VIEWPORT, 0) == (GLint)0);
    NS_ASSERT(GetInteger(GL_VIEWPORT, 1) == (GLint)0);
    NS_ASSERT(GetInteger(GL_VIEWPORT, 2) == (GLint)width);
    NS_ASSERT(GetInteger(GL_VIEWPORT, 3) == (GLint)height);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::BindAPI()
{
#if NS_RENDERER_USE_WGL
    mOpenGL32 = LoadLibraryA("opengl32.dll");
    NS_ASSERT(mOpenGL32 != 0);

    wglGetProcAddress = (PFNWGLGETPROCADDRESSPROC)GetProcAddress(mOpenGL32, "wglGetProcAddress");
    wglGetCurrentContext = (PFNWGLGETCURRENTCONTEXT)GetProcAddress(mOpenGL32, "wglGetCurrentContext");

    #define GL_IMPORT(_optional, _proto, _func) \
    { \
        _func = (_proto)wglGetProcAddress(#_func); \
        if (_func == 0) \
        { \
            _func = (_proto)GetProcAddress(mOpenGL32, #_func); \
        } \
        NS_ASSERT(_func != 0 || _optional); \
    }
    #include "GLImports.h"
    #undef GL_IMPORT

#elif NS_RENDERER_USE_EGL
    #define GL_IMPORT(_optional, _proto, _func) \
    { \
        _func = (_proto)eglGetProcAddress(#_func); \
        NS_ASSERT(_func != 0 || _optional); \
    }
    #include "GLImports.h"
    #undef GL_IMPORT

#elif NS_RENDERER_USE_GLX
    #define GL_IMPORT(_optional, _proto, _func) \
    { \
        _func = (_proto)glXGetProcAddressARB((const GLubyte *)#_func); \
        NS_ASSERT(_func != 0 || _optional); \
    }
    #include "GLImports.h"
    #undef GL_IMPORT

#elif NS_RENDERER_USE_NSGL
    const char* oglPath = "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL";
    void* ogl = dlopen(oglPath, RTLD_NOLOAD);
    NS_ASSERT(ogl != 0);

    #define GL_IMPORT(_optional, _proto, _func) \
    { \
        _func = (_proto)dlsym(ogl, #_func); \
        NS_ASSERT(_func != 0 || _optional); \
    }
    #include "GLImports.h"
    #undef GL_IMPORT

    dlclose(ogl);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::DetectGLVersion()
{
    uint32_t major;
    uint32_t minor;
    const char* version = (const char*)glGetString(GL_VERSION);
    NS_ASSERT(version != 0);

#if NS_RENDERER_OPENGL
    int n = sscanf(version, "%d.%d", &major, &minor);
    NS_ASSERT(n == 2);
#else
    if (sscanf(version, "OpenGL ES %d.%d", &major, &minor) != 2)
    {
        char profile[2];
        int n_ = sscanf(version, "OpenGL ES-%c%c %d.%d", profile, profile + 1, &major, &minor);
        NS_ASSERT(n_ == 4);
    }
#endif

    mGLVersion = 10 * major + minor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::FindExtensions()
{
#if NS_RENDERER_OPENGL
    #define GL_CORE(major, minor) NS_GL_VER(major, minor)
    #define GL_ES_CORE(major, minor) 0
#else
    #define GL_CORE(major, minor) 0
    #define GL_ES_CORE(major, minor) NS_GL_VER(major, minor)
#endif

    DetectGLVersion();

    struct Extension_
    {
        const char* name;
        uint32_t coreInVersion;
    };

    const Extension_ Extensions[Extension::Count] =
    {
        { "GL_ANGLE_framebuffer_blit",              GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_ANGLE_framebuffer_multisample",       GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_EXT_map_buffer_range",                GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_EXT_buffer_storage",                  GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_EXT_discard_framebuffer",             GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_EXT_framebuffer_blit",                GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_EXT_framebuffer_multisample",         GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_EXT_multisampled_render_to_texture",  GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_EXT_texture_storage",                 GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_EXT_texture_rg",                      GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_EXT_texture_border_clamp",            GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_APPLE_framebuffer_multisample",       GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_APPLE_texture_max_level",             GL_CORE(2,1) + GL_ES_CORE(3,0) },
        { "GL_APPLE_vertex_array_object",           GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_ARB_debug_output",                    GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_ARB_map_buffer_range",                GL_CORE(3,0) + GL_ES_CORE(3,0) },
        { "GL_ARB_buffer_storage",                  GL_CORE(4,4) + GL_ES_CORE(0,0) },
        { "GL_ARB_sync",                            GL_CORE(3,2) + GL_ES_CORE(3,0) },
        { "GL_ARB_vertex_array_object",             GL_CORE(3,0) + GL_ES_CORE(3,0) },
        { "GL_ARB_framebuffer_object",              GL_CORE(3,0) + GL_ES_CORE(3,0) },
        { "GL_ARB_invalidate_subdata",              GL_CORE(4,3) + GL_ES_CORE(3,0) },
        { "GL_ARB_ES3_compatibility",               GL_CORE(4,3) + GL_ES_CORE(3,0) },
        { "GL_ARB_texture_storage",                 GL_CORE(4,2) + GL_ES_CORE(3,0) },
        { "GL_ARB_texture_rg",                      GL_CORE(3,0) + GL_ES_CORE(3,0) },
        { "GL_ARB_uniform_buffer_object",           GL_CORE(3,1) + GL_ES_CORE(3,0) },
        { "GL_ARB_vertex_attrib_binding",           GL_CORE(4,3) + GL_ES_CORE(3,1) },
        { "GL_ARB_sampler_objects",                 GL_CORE(3,3) + GL_ES_CORE(3,0) },
        { "GL_ARB_texture_border_clamp",            GL_CORE(1,3) + GL_ES_CORE(3,2) },
        { "GL_ARB_debug",                           GL_CORE(4,3) + GL_ES_CORE(3,2) },
        { "GL_ARB_shader_viewport_layer_array",     GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_OES_rgb8_rgba8",                      GL_CORE(3,0) + GL_ES_CORE(3,0) },
        { "GL_OES_vertex_array_object",             GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_IMG_multisampled_render_to_texture",  GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_KHR_debug",                           GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_QCOM_tiled_rendering",                GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_AMD_vertex_shader_layer",             GL_CORE(0,0) + GL_ES_CORE(0,0) },
        { "GL_OVR_multiview",                       GL_CORE(0,0) + GL_ES_CORE(0,0) },
    };

    memset(mExtensions, 0, sizeof(mExtensions));

    {
        NS_LOG_TRACE("------------------GL extensions------------------");

        if (mGLVersion >= NS_GL_VER(3,0))
        {
            GLint numExtensions = 0;
            V(glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions));

            for (GLint i = 0; i < numExtensions; i++)
            {
                const char* extension = (const char*)glGetStringi(GL_EXTENSIONS, i);

                for (uint32_t j = 0; j < Extension::Count; j++)
                {
                    if (StrEquals(extension, Extensions[j].name))
                    {
                        mExtensions[j].supported = true;
                        break;
                    }
                }
            }
        }
        else
        {
            const char* extensions = (const char*)glGetString(GL_EXTENSIONS);

            if (extensions != 0)
            {
                for (uint32_t i = 0; i < Extension::Count; i++)
                {
                    if (StrFindFirst(extensions, Extensions[i].name) != -1)
                    {
                        mExtensions[i].supported = true;
                    }
                }
            }
        }

        for (uint32_t i = 0; i < Extension::Count; i++)
        {
            // Mark ARB extensions already in Core as supported
            uint32_t coreInVersion = Extensions[i].coreInVersion;
            if (!mExtensions[i].supported && coreInVersion != 0 && mGLVersion >= coreInVersion)
            {
                mExtensions[i].supported = true;
                NS_LOG_TRACE("[+] %s", Extensions[i].name);
            }
            else
            {
                NS_LOG_TRACE("[%c] %s", mExtensions[i].supported ? '*' : ' ', Extensions[i].name);
            }
        }
    }

    MapBufferRange_ = 0;
    UnmapBuffer_ = 0;
    BufferStorage_ = 0;
    BindVertexArray_ = 0;
    DeleteVertexArrays_ = 0;
    GenVertexArrays_ = 0;
    IsVertexArray_ = 0;
    BlitFramebuffer_ = 0;
    RenderbufferStorageMultisample_ = 0;
    FramebufferTexture2DMultisample_ = 0;
    InvalidateFramebuffer_ = 0;
    PushDebugGroup_ = 0;
    PopDebugGroup_ = 0;
    ObjectLabel_ = 0;
    TexStorage2D_ = 0;

    ////////////////////
    // MapBufferRange //
    ////////////////////
#if !NS_RENDERER_USE_EMS
    if (IsSupported(Extension::ARB_map_buffer_range))
    {
        MapBufferRange_ = glMapBufferRange;
        UnmapBuffer_ = glUnmapBuffer;
    }
    else if (IsSupported(Extension::EXT_map_buffer_range))
    {
      #if GL_EXT_map_buffer_range
        MapBufferRange_ = glMapBufferRangeEXT;
        UnmapBuffer_ = glUnmapBufferOES;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }
#endif

    ////////////////////
    // BufferStorage  //
    ////////////////////
    if (IsSupported(Extension::ARB_buffer_storage))
    {
      #if GL_VERSION_4_4 || GL_ARB_buffer_storage
        BufferStorage_ = glBufferStorage;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }
    else if (IsSupported(Extension::EXT_buffer_storage))
    {
      #if GL_EXT_buffer_storage
        // Implementation on Mali is broken, we are getting random flashes sometimes leading to crashes
        // Model: Motorola One | GPU: Mali-G72
        // Model: Samsung S8   | GPU: Mali-G71
        // Model: Huawei P30   | GPU: Mali-G76
        if (!StrStartsWith((const char*)glGetString(GL_RENDERER), "Mali"))
        {
            BufferStorage_ = glBufferStorageEXT;
        }
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }

    ///////////////////////
    // VertexArrayObject //
    ///////////////////////
    if (IsSupported(Extension::ARB_vertex_array_object))
    {
        BindVertexArray_ = glBindVertexArray;
        DeleteVertexArrays_ = glDeleteVertexArrays;
        GenVertexArrays_ = glGenVertexArrays;
        IsVertexArray_ = glIsVertexArray;
    }
    else if (IsSupported(Extension::APPLE_vertex_array_object))
    {
      #if GL_APPLE_vertex_array_object
        BindVertexArray_ = glBindVertexArrayAPPLE;
        DeleteVertexArrays_ = glDeleteVertexArraysAPPLE;
        GenVertexArrays_ = glGenVertexArraysAPPLE;
        IsVertexArray_ = glIsVertexArrayAPPLE;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }
    else if (IsSupported(Extension::OES_vertex_array_object))
    {
      #if GL_OES_vertex_array_object
        BindVertexArray_ = glBindVertexArrayOES;
        DeleteVertexArrays_ = glDeleteVertexArraysOES;
        GenVertexArrays_ = glGenVertexArraysOES;
        IsVertexArray_ = glIsVertexArrayOES;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }

    /////////////////////
    // BlitFramebuffer //
    /////////////////////
    if (IsSupported(Extension::ARB_framebuffer_object))
    {
        BlitFramebuffer_ = glBlitFramebuffer;
    }
    else if (IsSupported(Extension::EXT_framebuffer_blit))
    {
      #if GL_EXT_framebuffer_blit
        BlitFramebuffer_ = glBlitFramebufferEXT;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }
    else if (IsSupported(Extension::ANGLE_framebuffer_blit))
    {
      #if GL_ANGLE_framebuffer_blit
        BlitFramebuffer_ = glBlitFramebufferANGLE;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }

    ////////////////////////////////////
    // RenderbufferStorageMultisample //
    ////////////////////////////////////
    if (IsSupported(Extension::EXT_multisampled_render_to_texture))
    {
      #if GL_EXT_multisampled_render_to_texture
        RenderbufferStorageMultisample_ = glRenderbufferStorageMultisampleEXT;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }
    else if (IsSupported(Extension::IMG_multisampled_render_to_texture))
    {
      #if GL_IMG_multisampled_render_to_texture
        RenderbufferStorageMultisample_ = glRenderbufferStorageMultisampleIMG;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }
    else if (IsSupported(Extension::ARB_framebuffer_object))
    {
        RenderbufferStorageMultisample_ = glRenderbufferStorageMultisample;
    }
    else if (IsSupported(Extension::EXT_framebuffer_multisample))
    {
      #if GL_EXT_framebuffer_multisample
        RenderbufferStorageMultisample_ = glRenderbufferStorageMultisampleEXT;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }
    else if (IsSupported(Extension::APPLE_framebuffer_multisample))
    {
      #if GL_APPLE_framebuffer_multisample
        RenderbufferStorageMultisample_ = glRenderbufferStorageMultisampleAPPLE;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }
    else if (IsSupported(Extension::ANGLE_framebuffer_multisample))
    {
      #if GL_ANGLE_framebuffer_multisample
        RenderbufferStorageMultisample_ = glRenderbufferStorageMultisampleANGLE;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }

    /////////////////////////////////////
    // FramebufferTexture2DMultisample //
    /////////////////////////////////////
    if (IsSupported(Extension::EXT_multisampled_render_to_texture))
    {
      #if GL_EXT_multisampled_render_to_texture
        FramebufferTexture2DMultisample_ = glFramebufferTexture2DMultisampleEXT;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }
    else if (IsSupported(Extension::IMG_multisampled_render_to_texture))
    {
      #if GL_IMG_multisampled_render_to_texture
        FramebufferTexture2DMultisample_ = glFramebufferTexture2DMultisampleIMG;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }

    ///////////////////////////
    // InvalidateFramebuffer //
    ///////////////////////////
    if (IsSupported(Extension::ARB_invalidate_subdata))
    {
      #if GL_VERSION_4_3 || GL_ES_VERSION_3_0 || ARB_invalidate_subdata
        InvalidateFramebuffer_ = glInvalidateFramebuffer;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }
    else if (IsSupported(Extension::EXT_discard_framebuffer))
    {
      #if GL_EXT_discard_framebuffer
        InvalidateFramebuffer_ = glDiscardFramebufferEXT;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }

    ////////////////////
    // PushDebugGroup //
    ////////////////////
#if !NS_RENDERER_USE_EMS
    if (IsSupported(Extension::ARB_debug))
    {
      #if GL_VERSION_4_3 || GL_ES_VERSION_3_2
        PushDebugGroup_ = glPushDebugGroup;
        PopDebugGroup_ = glPopDebugGroup;
        ObjectLabel_ = glObjectLabel;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }
    else if (IsSupported(Extension::KHR_debug))
    {
      #if GL_KHR_debug
        #if NS_RENDERER_OPENGL
            PushDebugGroup_ = glPushDebugGroup;
            PopDebugGroup_ = glPopDebugGroup;
            ObjectLabel_ = glObjectLabel;
        #else
            PushDebugGroup_ = glPushDebugGroupKHR;
            PopDebugGroup_ = glPopDebugGroupKHR;
            ObjectLabel_ = glObjectLabelKHR;
        #endif
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }
#endif

    ////////////////
    // TexStorage //
    ////////////////
    if (IsSupported(Extension::ARB_texture_storage))
    {
        TexStorage2D_ = glTexStorage2D;
    }
    else if (IsSupported(Extension::EXT_texture_storage))
    {
      #if GL_EXT_texture_storage
        TexStorage2D_ = glTexStorage2DEXT;
      #else
        NS_ASSERT_UNREACHABLE;
      #endif
    }

#if !NS_RENDERER_USE_EMS
    // Check that TexStorage is not bugged (AMD Catalyst 13.90)
    if (TexStorage2D_ != 0)
    {
        GLuint name;
        V(glGenTextures(1, &name));
        V(glBindTexture(GL_TEXTURE_2D, name));

        while (glGetError() != GL_NO_ERROR);

        uint8_t data;
        TexStorage2D_(GL_TEXTURE_2D, 1, GL_RGBA8, 2, 2);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);

        if (glGetError() != GL_NO_ERROR)
        {
            mExtensions[Extension::ARB_texture_storage].supported = false;
            TexStorage2D_ = 0;
            while (glGetError() != GL_NO_ERROR);
        }

        V(glDeleteTextures(1, &name));
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool GLRenderDevice::IsSupported(Extension::Enum extension) const
{
    return mExtensions[extension].supported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t GLRenderDevice::MaxSamples() const
{
    if (IsSupported(Extension::EXT_multisampled_render_to_texture))
    {
        return GetInteger(GL_MAX_SAMPLES_EXT);
    }
    else if (IsSupported(Extension::IMG_multisampled_render_to_texture))
    {
        return GetInteger(GL_MAX_SAMPLES_IMG);
    }
    else if (IsSupported(Extension::ARB_framebuffer_object))
    {
        return GetInteger(GL_MAX_SAMPLES);
    }
    else if (IsSupported(Extension::EXT_framebuffer_multisample))
    {
        return GetInteger(GL_MAX_SAMPLES_EXT);
    }
    else if (IsSupported(Extension::ANGLE_framebuffer_multisample))
    {
        return GetInteger(GL_MAX_SAMPLES_ANGLE);
    }
    else if (IsSupported(Extension::APPLE_framebuffer_multisample))
    {
        return GetInteger(GL_MAX_SAMPLES_APPLE);
    }

    return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::FillCaps(bool sRGB)
{
    mCaps.centerPixelOffset = 0.0f;
    mCaps.linearRendering = sRGB;
    mCaps.depthRangeZeroToOne = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::PushDebugGroup(const char* label) const
{
    if (PushDebugGroup_ != 0)
    {
        V(PushDebugGroup_(GL_DEBUG_SOURCE_APPLICATION, 0, -1, label));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::PopDebugGroup() const
{
    if (PopDebugGroup_ != 0)
    {
        V(PopDebugGroup_());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GLRenderDevice::ObjectLabel(GLenum id, GLuint name, const char* label, ...) const
{
    if (ObjectLabel_ != 0)
    {
        char label_[128];

        va_list args;
        va_start(args, label);
        vsnprintf(label_, sizeof(label_), label, args);
        va_end(args);

        V(ObjectLabel_(id, name, -1, label_));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLint GLRenderDevice::GetInteger(GLenum name) const
{
    GLint vv;
    V(glGetIntegerv(name, &vv));
    return vv;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLint GLRenderDevice::GetInteger(GLenum name, uint32_t i) const
{
    NS_ASSERT(i < 4);
    GLint vv[4];
    V(glGetIntegerv(name, vv));
    return vv[i];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLfloat GLRenderDevice::GetFloat(GLenum name) const
{
    GLfloat vv;
    V(glGetFloatv(name, &vv));
    return vv;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLfloat GLRenderDevice::GetFloat(GLenum name, uint32_t i) const
{
    NS_ASSERT(i < 4);
    GLfloat vv[4];
    V(glGetFloatv(name, vv));
    return vv[i];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLboolean GLRenderDevice::GetBoolean(GLenum name, uint32_t i) const
{
    NS_ASSERT(i < 4);
    GLboolean vv[4];
    V(glGetBooleanv(name, vv));
    return vv[i];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLboolean GLRenderDevice::VertexAttribArrayEnabled(GLuint index) const
{
    GLint vv;
    V(glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &vv));
    return vv > 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLint GLRenderDevice::GetTexParameter(GLenum name) const
{
    GLint vv;
    V(glGetTexParameteriv(GL_TEXTURE_2D, name, &vv));
    return vv;
}
