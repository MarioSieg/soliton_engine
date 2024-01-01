// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

/*
 * Copyright 2014-2015 Daniel Collin. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bx/allocator.h>
#include <bx/math.h>
#include <mimalloc.h>
#include "imgui.h"
#include "font_awesome_pro_5.hpp"
#include "imgui_impl_glfw.h"

#include "vs_ocornut_imgui.bin.inl"
#include "fs_ocornut_imgui.bin.inl"
#include "vs_imgui_image.bin.inl"
#include "fs_imgui_image.bin.inl"

#define IMGUI_FLAGS_NONE        UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

static const bgfx::EmbeddedShader s_embeddedShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(vs_imgui_image),
    BGFX_EMBEDDED_SHADER(fs_imgui_image),

    BGFX_EMBEDDED_SHADER_END()
};

inline bool checkAvailTransientBuffers(uint32_t _numVertices, const bgfx::VertexLayout& _layout, uint32_t _numIndices) {
    return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, _layout)
        && (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices));
}

#include <array>
#include <span>

#include "jetbrains_mono.ttf.inl"
#include "font_awesome.ttf.inl"

struct ImguiContext final {
    void render(ImDrawData* _drawData) {
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fb_width = static_cast<int>(_drawData->DisplaySize.x * _drawData->FramebufferScale.x);
        int fb_height = static_cast<int>(_drawData->DisplaySize.y * _drawData->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
            return;

        bgfx::setViewName(m_viewId, "ImGui");
        bgfx::setViewMode(m_viewId, bgfx::ViewMode::Sequential);

        const bgfx::Caps* caps = bgfx::getCaps();
        {
            float ortho[16];
            float x = _drawData->DisplayPos.x;
            float y = _drawData->DisplayPos.y;
            float width = _drawData->DisplaySize.x;
            float height = _drawData->DisplaySize.y;

            bx::mtxOrtho(ortho, x, x + width, y + height, y, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
            bgfx::setViewTransform(m_viewId, nullptr, ortho);
            bgfx::setViewRect(m_viewId, 0, 0, static_cast<uint16_t>(width), static_cast<uint16_t>(height));
        }

        const ImVec2 clipPos = _drawData->DisplayPos; // (0,0) unless using multi-viewports
        const ImVec2 clipScale = _drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        for (int32_t ii = 0, num = _drawData->CmdListsCount; ii < num; ++ii) {
            bgfx::TransientVertexBuffer tvb;
            bgfx::TransientIndexBuffer tib;

            const ImDrawList* drawList = _drawData->CmdLists[ii];
            uint32_t numVertices = static_cast<uint32_t>(drawList->VtxBuffer.size());
            uint32_t numIndices = static_cast<uint32_t>(drawList->IdxBuffer.size());

            if (!checkAvailTransientBuffers(numVertices, m_layout, numIndices)) {
                // not enough space in transient buffer just quit drawing the rest...
                break;
            }

            bgfx::allocTransientVertexBuffer(&tvb, numVertices, m_layout);
            bgfx::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

            ImDrawVert* verts = (ImDrawVert*)tvb.data;
            bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert));

            ImDrawIdx* indices = (ImDrawIdx*)tib.data;
            bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx));

            bgfx::Encoder* encoder = bgfx::begin();

            for (const ImDrawCmd *cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd;
                 ++cmd) {
                if (cmd->UserCallback) {
                    cmd->UserCallback(drawList, cmd);
                }
                else if (0 != cmd->ElemCount) {
                    uint64_t state = 0
                        | BGFX_STATE_WRITE_RGB
                        | BGFX_STATE_WRITE_A
                        | BGFX_STATE_MSAA;

                    bgfx::TextureHandle th = m_texture;
                    bgfx::ProgramHandle program = m_program;

                    if (nullptr != cmd->TextureId) {
                        union {
                            ImTextureID ptr;

                            struct {
                                bgfx::TextureHandle handle;
                                uint8_t flags;
                                uint8_t mip;
                            } s;
                        } texture = {cmd->TextureId};
                        state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.s.flags)
                                     ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
                                     : BGFX_STATE_NONE;
                        th = texture.s.handle;
                        if (0 != texture.s.mip) {
                            const float lodEnabled[4] = {static_cast<float>(texture.s.mip), 1.0f, 0.0f, 0.0f};
                            bgfx::setUniform(u_imageLodEnabled, lodEnabled);
                            program = m_imageProgram;
                        }
                    }
                    else {
                        state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
                    }

                    // Project scissor/clipping rectangles into framebuffer space
                    ImVec4 clipRect;
                    clipRect.x = (cmd->ClipRect.x - clipPos.x) * clipScale.x;
                    clipRect.y = (cmd->ClipRect.y - clipPos.y) * clipScale.y;
                    clipRect.z = (cmd->ClipRect.z - clipPos.x) * clipScale.x;
                    clipRect.w = (cmd->ClipRect.w - clipPos.y) * clipScale.y;

                    if (clipRect.x < fb_width
                        && clipRect.y < fb_height
                        && clipRect.z >= 0.0f
                        && clipRect.w >= 0.0f) {
                        const uint16_t xx = static_cast<uint16_t>(bx::max(clipRect.x, 0.0f));
                        const uint16_t yy = static_cast<uint16_t>(bx::max(clipRect.y, 0.0f));
                        encoder->setScissor(xx, yy
                                            , static_cast<uint16_t>(bx::min(clipRect.z, 65535.0f) - xx)
                                            , static_cast<uint16_t>(bx::min(clipRect.w, 65535.0f) - yy)
                        );

                        encoder->setState(state);
                        encoder->setTexture(0, s_tex, th);
                        encoder->setVertexBuffer(0, &tvb, cmd->VtxOffset, numVertices);
                        encoder->setIndexBuffer(&tib, cmd->IdxOffset, cmd->ElemCount);
                        encoder->submit(m_viewId, program);
                        }
                }
                 }

            bgfx::end(encoder);
        }
    }

    void create() {
        IMGUI_CHECKVERSION();

        m_viewId = 255;

        ImGui::SetAllocatorFunctions(+[](size_t x, void*) -> void* {
        	return mi_malloc(x);
        }, +[](void* k, void*) {
        	mi_free(k);
        }, nullptr);

        m_imgui = ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();

        io.DisplaySize = ImVec2(1280.0f, 720.0f);
        io.DeltaTime = 1.0f / 60.0f;
        io.IniFilename = nullptr;

        ImGui::StyleColorsDark();

        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

        bgfx::RendererType::Enum type = bgfx::getRendererType();
        m_program = bgfx::createProgram(
            bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui")
            , bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui")
            , true
        );

        u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
        m_imageProgram = bgfx::createProgram(
            bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_imgui_image")
            , bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_imgui_image")
            , true
        );

        m_layout
            .begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();

        s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

        constexpr float fontSize = 18.0f;

        // add primary text font:
        ImFontConfig config { };
        config.FontDataOwnedByAtlas = false;
        config.MergeMode = false;
        ImFont* primaryFont = io.Fonts->AddFontFromMemoryTTF(
            const_cast<void*>(static_cast<const void*>(k_ttf_jet_brains_mono)),
            sizeof(k_ttf_jet_brains_mono) / sizeof(*k_ttf_jet_brains_mono),
            fontSize, // font size
            &config,
            io.Fonts->GetGlyphRangesDefault()
        );

        // now add font awesome icons:
        config.MergeMode = true;
        config.DstFont = primaryFont;
        struct font_range final {
            std::span<const std::uint8_t> data {};
            std::array<char16_t, 3> ranges {};
        };
        static constexpr font_range range = { k_font_awesome_ttf, { ICON_MIN_FA, ICON_MAX_FA, 0 } };
        ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
            const_cast<void*>(static_cast<const void*>(range.data.data())),
            static_cast<int>(range.data.size()),
            fontSize-3.0f,
            &config,
            reinterpret_cast<const ImWchar*>(range.ranges.data())
        );
        static_assert(sizeof(ImWchar) == sizeof(char16_t));

        // create font atlas now:

        uint8_t* data;
        int32_t width;
        int32_t height;
        io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);
        m_texture = bgfx::createTexture2D(
            static_cast<uint16_t>(width)
            , static_cast<uint16_t>(height)
            , false
            , 1
            , bgfx::TextureFormat::BGRA8
            , 0
            , bgfx::copy(data, width * height * 4)
        );
    }

    void destroy() {
        ImGui::DestroyContext(m_imgui);
        bgfx::destroy(s_tex);
        bgfx::destroy(m_texture);
        bgfx::destroy(u_imageLodEnabled);
        bgfx::destroy(m_imageProgram);
        bgfx::destroy(m_program);
    }

    void beginFrame(
        int _width
        , int _height
        , bgfx::ViewId _viewId
    ) {
        m_viewId = _viewId;

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(_width), static_cast<float>(_height));
        ImGui::NewFrame();
    }

    void endFrame() {
        ImGui::Render();
        render(ImGui::GetDrawData());
    }

    ImGuiContext* m_imgui;
    bgfx::VertexLayout m_layout;
    bgfx::ProgramHandle m_program;
    bgfx::ProgramHandle m_imageProgram;
    bgfx::TextureHandle m_texture;
    bgfx::UniformHandle s_tex;
    bgfx::UniformHandle u_imageLodEnabled;
    bgfx::ViewId m_viewId;
};

static ImguiContext s_imgui_ctx {};

namespace ImGuiEx {
    void Create(GLFWwindow* window) {
        s_imgui_ctx.create();
        ImGui_ImplGlfw_InitForOther(window, true);
        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    void Destroy() {
        ImGui_ImplGlfw_Shutdown();
        s_imgui_ctx.destroy();
    }

    void BeginFrame(uint16_t _width, uint16_t _height, bgfx::ViewId _viewId) {
        ImGui_ImplGlfw_NewFrame();
        s_imgui_ctx.beginFrame(_width, _height, _viewId);
    }

    void EndFrame() {
        s_imgui_ctx.endFrame();
    }
}
