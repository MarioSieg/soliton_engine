// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include <bx/math.h>

#include "pbr.hpp"

pbr_light_probe::pbr_light_probe(const std::string& name)
    : tex{name + "_lod.dds", false, BGFX_TEXTURE_SRGB},
    tex_irr{name + "_irr.dds", false, BGFX_TEXTURE_SRGB} {}

pbr_renderer::pbr_renderer() {
    std::memset(m_params, 0, sizeof(m_params));
    m_lightDir[0] = 0.0f;
    m_lightDir[1] = -1.0f;
    m_lightDir[2] = 0.0f;
    m_lightCol[0] = 1.0f;
    m_lightCol[1] = 1.0f;
    m_lightCol[2] = 1.0f;
    m_roughness = 1.0f;
    m_exposure = 1.0f;
    m_bgType = 2.0f;
    m_metalness = 0.001f;
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
    bx::mtxRotateY(m_mtx,0.0f);
}

auto pbr_renderer::set_camera_pos(const DirectX::XMFLOAT3& pos) noexcept -> void {
    std::memcpy(m_cameraPos, &pos, sizeof(DirectX::XMFLOAT3));
}

static constexpr std::uint32_t flags = BGFX_SAMPLER_MIN_ANISOTROPIC|BGFX_SAMPLER_MAG_ANISOTROPIC;

auto pbr_renderer::submit_material_data(
    DirectX::FXMMATRIX world,
    bgfx::TextureHandle albedo_map,
    bgfx::TextureHandle normal_map,
    bgfx::TextureHandle metallic_map,
    bgfx::TextureHandle roughness_map,
    bgfx::TextureHandle ao_map) const -> void {
    bgfx::setUniform(*m_u_params, m_params, num_vec4);
    DirectX::XMVECTOR det;
    DirectX::XMMATRIX normal = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&det, world));
    DirectX::XMFLOAT3X3 normalMatrix;
    DirectX::XMStoreFloat3x3(&normalMatrix, normal);
    bgfx::setUniform(*m_u_normalMtx, &normalMatrix);
    std::uint8_t stage = 0;
    bgfx::setTexture(stage++, *m_s_texLUT, *m_pbr_lut.handle);
    bgfx::setTexture(stage++, *m_s_texCube, *m_probe.tex.handle);
    bgfx::setTexture(stage++, *m_s_texCubeIrr, *m_probe.tex_irr.handle);
    bgfx::setTexture(stage++, *m_s_texAlbedo, albedo_map, flags);
    bgfx::setTexture(stage++, *m_s_texNormal, normal_map, flags);
    bgfx::setTexture(stage++, *m_s_texMetallic, roughness_map, flags);
    bgfx::setTexture(stage++, *m_s_texRoughness, metallic_map, flags);
    bgfx::setTexture(stage++, *m_s_texAO, ao_map, flags);
}
