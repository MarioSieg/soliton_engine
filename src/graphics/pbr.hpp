// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "prelude.hpp"
#include "texture.hpp"
#include "../math/DirectXMath.h"

class pbr_light_probe final : public no_copy, public no_move {
public:
    explicit pbr_light_probe(const std::string& name);

    texture tex;
    texture tex_irr;
};

class pbr_renderer final : public no_copy, public no_move {
    static constexpr std::size_t num_vec4 = 11;
public:
    pbr_renderer();

    auto set_camera_pos(const DirectX::XMFLOAT3& pos) noexcept -> void;
    auto submit_material_data(
        DirectX::FXMMATRIX world,
        bgfx::TextureHandle albedo_map,
        bgfx::TextureHandle normal_map,
        bgfx::TextureHandle metallic_map,
        bgfx::TextureHandle roughness_map,
        bgfx::TextureHandle ao_map
    ) const -> void;
    [[nodiscard]] auto get_roughness() const noexcept -> float { return m_roughness; }
    auto set_roughness(float r) noexcept -> void {
        m_roughness = std::clamp(r, 0.01f, 0.99f);
    }
    [[nodiscard]] auto get_metalness() const noexcept -> float { return m_metalness; }
    auto set_metalness(float r) noexcept -> void {
        m_metalness = std::clamp(r, 0.01f, 0.99f);
    }

    ~pbr_renderer() = default;

private:
    union {
        struct {
            union {
                float m_mtx[16];
                /* 0*/ struct { float m_mtx0[4]; };
                /* 1*/ struct { float m_mtx1[4]; };
                /* 2*/ struct { float m_mtx2[4]; };
                /* 3*/ struct { float m_mtx3[4]; };
            };
            /* 4*/ struct { float m_roughness, m_metalness, m_exposure, m_bgType; };
            /* 5*/ struct { float m_metalOrSpec, m_unused5[3]; };
            /* 6*/ struct { float m_cameraPos[3], m_unused7[1]; };
            /* 7*/ struct { float m_rgbDiff[4]; };
            /* 8*/ struct { float m_rgbSpec[4]; };
            /* 9*/ struct { float m_lightDir[3], m_unused10[1]; };
            /*10*/ struct { float m_lightCol[3], m_unused11[1]; };
        };

        float m_params[num_vec4*4];
    };

    texture m_pbr_lut {"media/textures/ibl_brdf_lut.png", false, BGFX_TEXTURE_NONE};
    pbr_light_probe m_probe {"media/textures/bolonga"};
    handle<bgfx::UniformHandle> m_u_params {};
    handle<bgfx::UniformHandle> m_u_normalMtx {};
    handle<bgfx::UniformHandle> m_s_texCube {};
    handle<bgfx::UniformHandle> m_s_texCubeIrr {};
    handle<bgfx::UniformHandle> m_s_texAlbedo {};
    handle<bgfx::UniformHandle> m_s_texNormal {};
    handle<bgfx::UniformHandle> m_s_texMetallic {};
    handle<bgfx::UniformHandle> m_s_texRoughness {};
    handle<bgfx::UniformHandle> m_s_texAO {};
    handle<bgfx::UniformHandle> m_s_texLUT {};
};
