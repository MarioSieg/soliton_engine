// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "dynamic_sky.hpp"
#include "../vulkancore/context.hpp"
#include "../graphics_subsystem.hpp"

namespace lu::graphics::pipelines {
	// Precomputed luminance of sunlight in XYZ colorspace.
	// Computed using code from Game Engine Gems, Volume One, chapter 15. Implementation based on Dr. Richard Bird model.
	// This table is used for piecewise linear interpolation. Transitions from and to 0.0 at sunset and sunrise are highly inaccurate
	static std::map<float, XMFLOAT3> sunLuminanceXYZTable =
	{
		{  5.0f, {  0.000000f,  0.000000f,  0.000000f } },
		{  7.0f, { 12.703322f, 12.989393f,  9.100411f } },
		{  8.0f, { 13.202644f, 13.597814f, 11.524929f } },
		{  9.0f, { 13.192974f, 13.597458f, 12.264488f } },
		{ 10.0f, { 13.132943f, 13.535914f, 12.560032f } },
		{ 11.0f, { 13.088722f, 13.489535f, 12.692996f } },
		{ 12.0f, { 13.067827f, 13.467483f, 12.745179f } },
		{ 13.0f, { 13.069653f, 13.469413f, 12.740822f } },
		{ 14.0f, { 13.094319f, 13.495428f, 12.678066f } },
		{ 15.0f, { 13.142133f, 13.545483f, 12.526785f } },
		{ 16.0f, { 13.201734f, 13.606017f, 12.188001f } },
		{ 17.0f, { 13.182774f, 13.572725f, 11.311157f } },
		{ 18.0f, { 12.448635f, 12.672520f,  8.267771f } },
		{ 20.0f, {  0.000000f,  0.000000f,  0.000000f } },
	};

	// Precomputed luminance of sky in the zenith point in XYZ colorspace.
	// Computed using code from Game Engine Gems, Volume One, chapter 15. Implementation based on Dr. Richard Bird model.
	// This table is used for piecewise linear interpolation. Day/night transitions are highly inaccurate.
	// The scale of luminance change in Day/night transitions is not preserved.
	// Luminance at night was increased to eliminate need the of HDR render.
	static std::map<float, XMFLOAT3> skyLuminanceXYZTable =
	{
		{  0.0f, { 0.308f,    0.308f,    0.411f    } },
		{  1.0f, { 0.308f,    0.308f,    0.410f    } },
		{  2.0f, { 0.301f,    0.301f,    0.402f    } },
		{  3.0f, { 0.287f,    0.287f,    0.382f    } },
		{  4.0f, { 0.258f,    0.258f,    0.344f    } },
		{  5.0f, { 0.258f,    0.258f,    0.344f    } },
		{  7.0f, { 0.962851f, 1.000000f, 1.747835f } },
		{  8.0f, { 0.967787f, 1.000000f, 1.776762f } },
		{  9.0f, { 0.970173f, 1.000000f, 1.788413f } },
		{ 10.0f, { 0.971431f, 1.000000f, 1.794102f } },
		{ 11.0f, { 0.972099f, 1.000000f, 1.797096f } },
		{ 12.0f, { 0.972385f, 1.000000f, 1.798389f } },
		{ 13.0f, { 0.972361f, 1.000000f, 1.798278f } },
		{ 14.0f, { 0.972020f, 1.000000f, 1.796740f } },
		{ 15.0f, { 0.971275f, 1.000000f, 1.793407f } },
		{ 16.0f, { 0.969885f, 1.000000f, 1.787078f } },
		{ 17.0f, { 0.967216f, 1.000000f, 1.773758f } },
		{ 18.0f, { 0.961668f, 1.000000f, 1.739891f } },
		{ 20.0f, { 0.264f,    0.264f,    0.352f    } },
		{ 21.0f, { 0.264f,    0.264f,    0.352f    } },
		{ 22.0f, { 0.290f,    0.290f,    0.386f    } },
		{ 23.0f, { 0.303f,    0.303f,    0.404f    } },
	};

    dynamic_sky_pipeline::dynamic_sky_pipeline() : graphics_pipeline{"dynamic_sky"} {
        eastl::vector<vertex> vertices {};
        eastl::vector<std::uint32_t> indices {};
        vertices.resize(grid_size * grid_size);
        indices.resize((grid_size - 1) * (grid_size - 1) * 6);
        for (std::size_t i = 0; i < grid_size; ++i){
            for (std::size_t j = 0; j < grid_size; ++j) {
                vertex& v = vertices[i * grid_size + j];
                v.position.x = static_cast<float>(j) / (grid_size - 1) * 2.0f - 1.0f;
                v.position.y = static_cast<float>(i) / (grid_size - 1) * 2.0f - 1.0f;
            }
        }
        std::size_t k = 0;
        for (std::size_t i = 0; i < grid_size - 1; ++i) {
            for (std::size_t j = 0; j < grid_size - 1; ++j) {
                indices[k++] = static_cast<std::uint16_t>(j + 0 + grid_size*(i + 0));
                indices[k++] = static_cast<std::uint16_t>(j + 1 + grid_size*(i + 0));
                indices[k++] = static_cast<std::uint16_t>(j + 0 + grid_size*(i + 1));
                indices[k++] = static_cast<std::uint16_t>(j + 1 + grid_size*(i + 0));
                indices[k++] = static_cast<std::uint16_t>(j + 1 + grid_size*(i + 1));
                indices[k++] = static_cast<std::uint16_t>(j + 0 + grid_size*(i + 1));
            }
        }
    	eastl::vector<std::uint32_t> indices2 {};
    	indices2.resize(k);
    	eastl::copy(indices.begin(), indices.begin() + k, indices2.begin());
        m_grid.emplace(vertices, indices2, false);
    	m_sunLuminanceXYZ.SetMap(sunLuminanceXYZTable);
    	m_skyLuminanceXYZ.SetMap(skyLuminanceXYZTable);
    }

    dynamic_sky_pipeline::~dynamic_sky_pipeline() {

    }

    auto dynamic_sky_pipeline::configure_shaders(eastl::vector<eastl::shared_ptr<shader>>& cfg) -> void {
        auto vs = shader_cache::get().get_shader(shader_variant{"/RES/shaders/src/dynamic_sky.vert", shader_stage::vertex});
        auto fs = shader_cache::get().get_shader(shader_variant{"/RES/shaders/src/dynamic_sky.frag", shader_stage::fragment});
        cfg.emplace_back(vs);
        cfg.emplace_back(fs);
    }

    auto dynamic_sky_pipeline::configure_pipeline_layout(eastl::vector<vk::DescriptorSetLayout>& layouts, eastl::vector<vk::PushConstantRange>& ranges) -> void {
    	layouts.emplace_back(shared_buffers::get()->get_layout());
    }

    auto dynamic_sky_pipeline::configure_rasterizer(vk::PipelineRasterizationStateCreateInfo& cfg) -> void {
        graphics_pipeline::configure_rasterizer(cfg);
        cfg.cullMode = vk::CullModeFlagBits::eFront;
    }

    auto dynamic_sky_pipeline::on_bind(vkb::command_buffer& cmd) const -> void {
        graphics_pipeline::on_bind(cmd);
    	const std::uint32_t dynamic_offset = shared_buffers::get()->per_frame_ubo.get_dynamic_aligned_size() * vkb::ctx().get_current_concurrent_frame_idx();
    	cmd.bind_graphics_descriptor_set(shared_buffers::get()->get_set(), LU_GLSL_DESCRIPTOR_SET_IDX_PER_FRAME, &dynamic_offset);
    }

    auto dynamic_sky_pipeline::render_sky(vkb::command_buffer& cmd) const -> void {
        cmd.draw_mesh(*m_grid);
    }

    auto dynamic_sky_pipeline::compute_perez_coeff(float turbidity, eastl::array<XMFLOAT4, 5>& out) noexcept -> void {
        static constexpr XMFLOAT3A ABCDE[] = {
            { -0.2592f, -0.2608f, -1.4630f },
            {  0.0008f,  0.0092f,  0.4275f },
            {  0.2125f,  0.2102f,  5.3251f },
            { -0.8989f, -1.6537f, -2.5771f },
            {  0.0452f,  0.0529f,  0.3703f },
        };
        static constexpr XMFLOAT3A ABCDE_t[] = {
            { -0.0193f, -0.0167f,  0.1787f },
            { -0.0665f, -0.0950f, -0.3554f },
            { -0.0004f, -0.0079f, -0.0227f },
            { -0.0641f, -0.0441f,  0.1206f },
            { -0.0033f, -0.0109f, -0.0670f },
        };
        XMVECTOR turb = XMVectorReplicate(turbidity);
        for (std::size_t i=0; i < out.size(); ++i) {
            XMVECTOR tmp = XMVectorMultiplyAdd(XMLoadFloat3A(ABCDE_t+i), turb, XMLoadFloat3A(ABCDE+i));
        	XMVectorSetW(tmp, 0.0f);
            XMStoreFloat4(&out[i], tmp);
        }
    }

    auto dynamic_sky_pipeline::configure_depth_stencil(vk::PipelineDepthStencilStateCreateInfo& cfg) -> void {
        graphics_pipeline::configure_depth_stencil(cfg);
        cfg.depthTestEnable = vk::False;
        cfg.depthWriteEnable = vk::False;
    }
}
