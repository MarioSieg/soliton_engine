////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __RENDER_GLRENDERDEVICE_H__
#define __RENDER_GLRENDERDEVICE_H__


#include <NsRender/RenderDevice.h>
#include <NsCore/Vector.h>

#include "GLHeaders.h"


class GLTexture;
class GLRenderTarget;


namespace NoesisApp
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// GLRenderDevice
////////////////////////////////////////////////////////////////////////////////////////////////////
class GLRenderDevice final: public Noesis::RenderDevice
{
public:
    GLRenderDevice(bool sRGB = false);
    ~GLRenderDevice();

    // Creates a Noesis texture from a GL texture
    static Noesis::Ptr<Noesis::Texture> WrapTexture(GLuint object, uint32_t width, uint32_t height,
        uint32_t levels, bool isInverted, bool hasAlpha);

    void* CreatePixelShader(const char* label, uint8_t shader, const char* glsl);
    void ClearPixelShaders();

private:
    /// From RenderDevice
    //@{
    const Noesis::DeviceCaps& GetCaps() const override;
    Noesis::Ptr<Noesis::RenderTarget> CreateRenderTarget(const char* label, uint32_t width,
        uint32_t height, uint32_t sampleCount, bool needsStencil) override;
    Noesis::Ptr<Noesis::RenderTarget> CloneRenderTarget(const char* label,
        Noesis::RenderTarget* surface) override;
    Noesis::Ptr<Noesis::Texture> CreateTexture(const char* label, uint32_t width, uint32_t height,
        uint32_t numLevels, Noesis::TextureFormat::Enum format, const void** data) override;
    void UpdateTexture(Noesis::Texture* texture, uint32_t level, uint32_t x, uint32_t y,
        uint32_t width, uint32_t height, const void* data) override;
    void BeginOffscreenRender() override;
    void EndOffscreenRender() override;
    void BeginOnscreenRender() override;
    void EndOnscreenRender() override;
    void SetRenderTarget(Noesis::RenderTarget* surface) override;
    void BeginTile(Noesis::RenderTarget* surface, const Noesis::Tile& tile) override;
    void EndTile(Noesis::RenderTarget* surface) override;
    void ResolveRenderTarget(Noesis::RenderTarget* surface, const Noesis::Tile* tiles,
        uint32_t numTiles) override;
    void* MapVertices(uint32_t bytes) override;
    void UnmapVertices() override;
    void* MapIndices(uint32_t bytes) override;
    void UnmapIndices() override;
    void DrawBatch(const Noesis::Batch& batch) override;
    //@}

    friend class ::GLTexture;
    friend class ::GLRenderTarget;
    void DeleteTexture(GLTexture* texture);
    void DeleteRenderTarget(GLRenderTarget* surface);

    void InvalidateStateCache();
    void UnbindObjects();

    struct Page;
    struct DynamicBuffer;
    Page* AllocatePage(DynamicBuffer& buffer);
    void CreateBuffer(DynamicBuffer& buffer, GLenum target, uint32_t size, const char* label);
    void DestroyBuffer(DynamicBuffer& buffer) const;
    void BindBuffer(DynamicBuffer& buffer);
    void SetVertexFormat(uint8_t format, uintptr_t offset);
    void DisableClientState() const;
    void EnableVertexAttribArrays(uint32_t state);
    void* MapBuffer(DynamicBuffer& buffer, uint32_t size);
    void UnmapBuffer(DynamicBuffer& buffer);
    void CreateSamplers();
    void EnsureVsShader(uint32_t n, bool stereo);
    void EnsureProgram(uint32_t n, bool stereo);
    void CreatePrograms();
    void CreateVertexFormats();
    GLboolean HaveMapBufferRange() const;
    GLvoid* MapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length,
        GLbitfield access) const;
    void UnmapBuffer(GLenum target) const;
    GLboolean HaveBufferStorage() const;
    void BufferStorage(GLenum target, GLsizeiptr size, GLbitfield flags);
    GLboolean HaveVertexArrayObject() const;
    void GenVertexArrays(GLsizei n, GLuint *arrays) const;
    void BindVertexArray(GLuint name);
    void DeleteVertexArrays(GLsizei n, const GLuint *arrays) const;
    GLboolean IsVertexArray(GLuint array) const;

    GLuint RenderbufferStorage(GLsizei samples, GLenum format, GLsizei width, GLsizei height) const;
    void RenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat,
        GLsizei width, GLsizei height) const;
    GLboolean HaveMultisampledRenderToTexture() const;
    void FramebufferTexture2DMultisample(GLenum target, GLenum attachment, GLenum textarget,
        GLuint texture, GLint level, GLsizei samples) const;
    GLuint CreateStencil(const char* label, GLsizei width, GLsizei height, GLsizei samples) const;
    GLuint CreateColor(const char* label, GLsizei width, GLsizei height, GLsizei samples) const;
    void CreateFBO(const char* label, GLsizei samples, GLuint texture, GLuint stencil,
        GLuint colorAA, GLuint& fbo, GLuint& fboResolved) const;
    void Resolve(GLRenderTarget* target, const Noesis::Tile* tiles, uint32_t numTiles);
    void BindRenderTarget(GLRenderTarget* renderTarget);
    void DiscardFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments) const;
    bool HaveBlitFramebuffer() const;
    void BlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0,
        GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) const;
    void DeleteFramebuffer(GLuint framebuffer) const;
    void DeleteRenderbuffer(GLuint renderbuffer) const;

    bool HaveBorderClamp() const;
    bool HaveTexStorage() const;
    bool HaveTexRG() const;
    void GetGLFormat(Noesis::TextureFormat::Enum texFormat, GLenum& internalFormat, GLenum& format);
    GLint GLWrapSMode(Noesis::SamplerState sampler) const;
    GLint GLWrapTMode(Noesis::SamplerState sampler) const;
    GLint GLMinFilter(Noesis::SamplerState sampler) const;
    GLint GLMagFilter(Noesis::SamplerState sampler) const;

    void TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width,
        GLsizei height);
    void InitTexture(const char* label, GLTexture* texture, GLenum internalFormat, GLenum format);
    void SetTextureState(GLTexture* texture, Noesis::SamplerState state);
    void SetTexture(uint32_t unit, GLTexture* texture, Noesis::SamplerState state);
    void SetTextures(const Noesis::Batch& batch);
    void ActiveTexture(uint32_t unit);
    void BindTexture2D(GLuint texture);
    void SetUnpackState();
    void TexSubImage2D(GLint level, GLint xoffset, GLint yoffset, GLint width, GLint height,
        GLenum format, const GLvoid* data);
    void BindSampler(GLuint sampler);
    void DeleteSampler(GLuint sampler) const;
    void DeleteTexture(GLuint texture) const;

    struct ProgramInfo;
    GLuint CompileShader(GLenum type, const char** strings, uint32_t count) const;
    void LinkProgram(GLuint vso, GLuint pso, ProgramInfo& p, const char* label);

    void BindTextureLocation(GLuint po, const char* name, GLint unit);
    void UploadUniforms(ProgramInfo* program, uint32_t index, const Noesis::UniformData& data);
    void UseProgram(GLuint name);
    void DeleteShader(GLuint shader) const;
    void DeleteProgram(GLuint program) const;

    union GLRenderState;
    void BlendEnable();
    void BlendDisable();
    void BlendFuncSrcOver();
    void BlendFuncSrcOverMultiply();
    void BlendFuncSrcOverScreen();
    void BlendFuncSrcOverAdditive();
    void StencilEnable();
    void StencilDisable();
    void DepthEnable();
    void DepthDisable();
    void StencilOpKeep(uint8_t stencilRef);
    void StencilOpIncr(uint8_t stencilRef);
    void StencilOpDecr(uint8_t stencilRef);
    void StencilOpClear();
    void SetRenderState(Noesis::RenderState state, uint8_t stencilRef);
    void Viewport(uint32_t width, uint32_t height);

    void BindAPI();
    void DetectGLVersion();
    void FindExtensions();
    uint32_t MaxSamples() const;
    void FillCaps(bool sRGB);
 
    void PushDebugGroup(const char* label) const;
    void PopDebugGroup() const;
    void ObjectLabel(GLenum id, GLuint name, const char* label, ...) const;

    GLint GetInteger(GLenum name) const;
    GLint GetInteger(GLenum name, uint32_t i) const;
    GLfloat GetFloat(GLenum name) const;
    GLfloat GetFloat(GLenum name, uint32_t i) const;
    GLboolean GetBoolean(GLenum name, uint32_t i) const;
    GLboolean VertexAttribArrayEnabled(GLuint index) const;
    GLint GetTexParameter(GLenum name) const;

private:
    uint32_t mGLVersion;

#if NS_RENDERER_USE_WGL
    HMODULE mOpenGL32;
    PFNWGLGETPROCADDRESSPROC wglGetProcAddress;
    PFNWGLGETCURRENTCONTEXT wglGetCurrentContext;
#endif

    #define GL_IMPORT(_optional, _proto, _func) _proto _func
    #include "GLImports.h"
    #undef GL_IMPORT

    uint32_t mMaxVertexAttribs;
    uint32_t mUBOAlignment;

    struct VertexFormat
    {
        GLuint vao;
        GLuint vao_ib;
        uint32_t stride;
    };

    VertexFormat mVertexFormats[Noesis::Shader::Vertex::Format::Count];

    struct Page
    {
        GLuint object;
        uint8_t* base;
        GLsync sync;
        Page* next;
    };

    struct DynamicBuffer
    {
        GLenum target;
        const char* label;

        uint32_t size;
        uint32_t pos;
        uint32_t drawPos;

        void* cpuMemory;

        Page* currentPage;
        Page* freePages;
        Page* pendingPages;

        uint32_t numPages;

        GLuint boundObject;
    };

    DynamicBuffer mDynamicVB;
    DynamicBuffer mDynamicIB;
    DynamicBuffer mUBO;

    GLuint mDefaultVAO;

    Noesis::DeviceCaps mCaps;

    struct ProgramInfo
    {
        uint8_t vertexFormat;
        GLuint object;

        GLuint uboIndex;
        GLint cbufferLocation[4];
        uint32_t cbufferHash[4];
    };

    GLuint mVsShaders[21];
    GLuint mVsShadersVR[21];
    ProgramInfo mPrograms[Noesis::Shader::Count];
    ProgramInfo mProgramsVR[Noesis::Shader::Count];
    GLuint mSamplers[64];

    Noesis::Vector<ProgramInfo> mCustomShaders;
    Noesis::Vector<ProgramInfo> mCustomShadersVR;

    struct TextureUnit
    {
        enum Enum
        {
            Pattern,
            Ramps,
            Image,
            Glyphs,
            Shadow,

            Count
        };
    };

    //Cached state
    //@{
    uint32_t mViewportWidth, mViewportHeight;
    GLuint mVertexArray;
    uint32_t mActiveTextureUnit;
    uint32_t mBlendEnabled;
    uint32_t mBlendFunc;
    uint32_t mStencilRef;
    uint32_t mStencilEnabled;
    uint32_t mStencilOp;
    uint32_t mDepthEnabled;
    Noesis::RenderState mRenderState;
    GLuint mBoundTextures[TextureUnit::Count];
    GLuint mBoundSamplers[TextureUnit::Count];
    GLuint mProgram;
    uint32_t mUBOHash;
    uint32_t mEnabledVertexAttribArrays;
    GLRenderTarget* mBoundRenderTarget;
    bool mUnpackStateSet;
    //@}

    struct Extension
    {
        enum Enum
        {
            ANGLE_framebuffer_blit,
            ANGLE_framebuffer_multisample,
            EXT_map_buffer_range,
            EXT_buffer_storage,
            EXT_discard_framebuffer,
            EXT_framebuffer_blit,
            EXT_framebuffer_multisample,
            EXT_multisampled_render_to_texture,
            EXT_texture_storage,
            EXT_texture_rg,
            EXT_texture_border_clamp,
            APPLE_framebuffer_multisample,
            APPLE_texture_max_level,
            APPLE_vertex_array_object,
            ARB_debug_output,
            ARB_map_buffer_range,
            ARB_buffer_storage,
            ARB_sync,
            ARB_vertex_array_object,
            ARB_framebuffer_object,
            ARB_invalidate_subdata,
            ARB_ES3_compatibility,
            ARB_texture_storage,
            ARB_texture_rg,
            ARB_uniform_buffer_object,
            ARB_vertex_attrib_binding,
            ARB_sampler_objects,
            ARB_texture_border_clamp,
            ARB_debug,
            ARB_shader_viewport_layer_array,
            OES_rgb8_rgba8,
            OES_vertex_array_object,
            IMG_multisampled_render_to_texture,
            KHR_debug,
            QCOM_tiled_rendering,
            AMD_vertex_shader_layer,
            OVR_multiview,

            Count
        };

        bool supported;
    };

    bool IsSupported(Extension::Enum extension) const;
    Extension mExtensions[Extension::Count];

    GLvoid* (GL_APIENTRYP MapBufferRange_)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
    GLboolean (GL_APIENTRYP UnmapBuffer_)(GLenum);
    void (GL_APIENTRYP BufferStorage_)(GLenum, GLsizeiptr, const void*, GLbitfield);
    void (GL_APIENTRYP BindVertexArray_)(GLuint);
    void (GL_APIENTRYP DeleteVertexArrays_)(GLsizei, const GLuint *);
    void (GL_APIENTRYP GenVertexArrays_)(GLsizei, GLuint *);
    GLboolean (GL_APIENTRYP IsVertexArray_)(GLuint array);
    void (GL_APIENTRYP BlitFramebuffer_)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint,
        GLbitfield, GLenum);
    void (GL_APIENTRYP RenderbufferStorageMultisample_)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
    void (GL_APIENTRYP FramebufferTexture2DMultisample_)(GLenum, GLenum, GLenum, GLuint, GLint,
        GLsizei);
    void (GL_APIENTRYP InvalidateFramebuffer_)(GLenum, GLsizei, const GLenum *);
    void (GL_APIENTRYP PushDebugGroup_)(GLenum, GLuint, GLsizei, const GLchar *);
    void (GL_APIENTRYP PopDebugGroup_)(void);
    void (GL_APIENTRYP ObjectLabel_)(GLenum, GLuint, GLsizei, const GLchar *);
    void (GL_APIENTRYP TexStorage2D_)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
};

}

#endif
