// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "prelude.hpp"
#include "texture.hpp"

class pbr_light_probe final : public no_copy, public no_move {
public:
    explicit pbr_light_probe(const std::string& name)
        : tex{name + "_lod.dds", false, BGFX_SAMPLER_U_CLAMP|BGFX_SAMPLER_V_CLAMP|BGFX_SAMPLER_W_CLAMP},
        tex_irr{name + "_irr.dds", false, BGFX_SAMPLER_U_CLAMP|BGFX_SAMPLER_V_CLAMP|BGFX_SAMPLER_W_CLAMP} { }

    texture tex;
    texture tex_irr;
};

class pbr_renderer final : public no_copy, public no_move {
    static constexpr std::size_t num_vec4 = 12;
public:
    pbr_renderer() {
        m_lightDir[0] = -0.8f;
        m_lightDir[1] = 0.2f;
        m_lightDir[2] = -0.5f;
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
        m_doDiffuse = static_cast<float>(false);
        m_doSpecular = static_cast<float>(false);
        m_doDiffuseIbl = static_cast<float>(true);
        m_doSpecularIbl = static_cast<float>(true);
        m_metalOrSpec = 0.0f;
        m_u_params = handle{bgfx::createUniform("u_params", bgfx::UniformType::Vec4, num_vec4)};
        m_s_texCube = handle{bgfx::createUniform("s_texCube", bgfx::UniformType::Sampler)};
        m_s_texCubeIrr = handle{bgfx::createUniform("s_texCubeIrr", bgfx::UniformType::Sampler)};
    }
    auto submit_uniforms() const -> void {
        bgfx::setUniform(*m_u_params, m_params, num_vec4);
        bgfx::setTexture(0, *m_s_texCube, *m_probe.tex.handle);
        bgfx::setTexture(1, *m_s_texCubeIrr, *m_probe.tex_irr.handle);
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
            /* 6*/ struct { float m_doDiffuse, m_doSpecular, m_doDiffuseIbl, m_doSpecularIbl; };
            /* 7*/ struct { float m_cameraPos[3], m_unused7[1]; };
            /* 8*/ struct { float m_rgbDiff[4]; };
            /* 9*/ struct { float m_rgbSpec[4]; };
            /*10*/ struct { float m_lightDir[3], m_unused10[1]; };
            /*11*/ struct { float m_lightCol[3], m_unused11[1]; };
        };

        float m_params[num_vec4*4] {};
    };

private:
    pbr_light_probe m_probe {"media/textures/bolonga"};
    handle<bgfx::UniformHandle> m_u_params {};
    handle<bgfx::UniformHandle> m_s_texCube {};
    handle<bgfx::UniformHandle> m_s_texCubeIrr {};
};
