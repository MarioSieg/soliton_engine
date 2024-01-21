// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "prelude.hpp"
#include "texture.hpp"
#include "../math/DirectXMath.h"

class pbr_light_probe final : public no_copy, public no_move {
public:
    explicit pbr_light_probe(const std::string& name)
        : tex{name + "_lod.dds", false, BGFX_TEXTURE_SRGB},
        tex_irr{name + "_irr.dds", false, BGFX_TEXTURE_SRGB}
    {

    }

    texture tex;
    texture tex_irr;
};

class pbr_renderer final : public no_copy, public no_move {
    static constexpr std::size_t num_vec4 = 11;
public:
    pbr_renderer() {
        m_lightDir[0] = 0.0f;
        m_lightDir[1] = -1.0f;
        m_lightDir[2] = 0.0f;
        m_lightCol[0] = 1.0f;
        m_lightCol[1] = 1.0f;
        m_lightCol[2] = 1.0f;
        m_glossiness = 0.7f;
        m_exposure = 0.0f;
        m_bgType = 2.0f;
        m_reflectivity = 0.85f;
        m_rgbDiff[0] = 1.0f;
        m_rgbDiff[1] = 1.0f;
        m_rgbDiff[2] = 1.0f;
        m_rgbSpec[0] = 1.0f;
        m_rgbSpec[1] = 1.0f;
        m_rgbSpec[2] = 1.0f;
        m_metalOrSpec = 0.0f;
        m_u_params = handle{bgfx::createUniform("u_params", bgfx::UniformType::Vec4, num_vec4)};
        m_u_normalMtx = handle{bgfx::createUniform("u_normalMtx", bgfx::UniformType::Mat3)};
        m_s_texLUT = handle{bgfx::createUniform("s_texLUT", bgfx::UniformType::Sampler)};
        m_s_texCube = handle{bgfx::createUniform("s_texCube", bgfx::UniformType::Sampler)};
        m_s_texCubeIrr = handle{bgfx::createUniform("s_texCubeIrr", bgfx::UniformType::Sampler)};
        m_s_texAlbedo = handle{bgfx::createUniform("s_texAlbedo", bgfx::UniformType::Sampler)};
        m_s_texNormal = handle{bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler)};
        m_s_texMetallic = handle{bgfx::createUniform("s_texMetallic", bgfx::UniformType::Sampler)};
        m_s_texRoughness = handle{bgfx::createUniform("s_texRoughness", bgfx::UniformType::Sampler)};
        m_s_texAO = handle{bgfx::createUniform("s_texAO", bgfx::UniformType::Sampler)};
    }

    auto submit_material_data(
        DirectX::FXMMATRIX world,
        bgfx::TextureHandle albedo_map,
        bgfx::TextureHandle normal_map,
        bgfx::TextureHandle metallic_map,
        bgfx::TextureHandle roughness_map,
        bgfx::TextureHandle ao_map
    ) const -> void {
        bgfx::setUniform(*m_u_params, m_params, num_vec4);
        DirectX::XMVECTOR det;
        DirectX::XMMATRIX normal = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&det, world));
        DirectX::XMFLOAT3X3 normalMatrix;
        DirectX::XMStoreFloat3x3(&normalMatrix, normal);
        bgfx::setUniform(*m_u_normalMtx, &normalMatrix);
        constexpr std::uint32_t flags = BGFX_SAMPLER_MIN_ANISOTROPIC|BGFX_SAMPLER_MAG_ANISOTROPIC;
        std::uint8_t stage = 0;
        bgfx::setTexture(stage++, *m_s_texLUT, *m_pbr_lut.handle);
        bgfx::setTexture(stage++, *m_s_texCube, *m_probe.tex.handle, flags);
        bgfx::setTexture(stage++, *m_s_texCubeIrr, *m_probe.tex_irr.handle, flags);
        bgfx::setTexture(stage++, *m_s_texAlbedo, albedo_map, flags);
        bgfx::setTexture(stage++, *m_s_texNormal, normal_map, flags);
        bgfx::setTexture(stage++, *m_s_texMetallic, roughness_map, flags);
        bgfx::setTexture(stage++, *m_s_texRoughness, metallic_map, flags);
        bgfx::setTexture(stage++, *m_s_texAO, ao_map, flags);
    }

    ~pbr_renderer() = default;

    union {
        struct {
            union {
                float m_mtx[16];
                /* 0*/ struct { float m_mtx0[4]; };
                /* 1*/ struct { float m_mtx1[4]; };
                /* 2*/ struct { float m_mtx2[4]; };
                /* 3*/ struct { float m_mtx3[4]; };
            };
            /* 4*/ struct { float m_glossiness, m_reflectivity, m_exposure, m_bgType; };
            /* 5*/ struct { float m_metalOrSpec, m_unused5[3]; };
            /* 6*/ struct { float m_cameraPos[3], m_unused7[1]; };
            /* 7*/ struct { float m_rgbDiff[4]; };
            /* 8*/ struct { float m_rgbSpec[4]; };
            /* 9*/ struct { float m_lightDir[3], m_unused10[1]; };
            /*10*/ struct { float m_lightCol[3], m_unused11[1]; };
        };

        float m_params[num_vec4*4] {};
    };

private:
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
