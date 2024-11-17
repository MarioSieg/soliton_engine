// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../graphics_pipeline.hpp"
#include "../texture.hpp"
#include "../mesh.hpp"
#include "../vulkancore/command_buffer.hpp"
#include "../../scene/components.hpp"

namespace lu::graphics::pipelines {
    // Performs piecewise linear interpolation of a Color parameter.
    class DynamicValueController
    {
        typedef std::map<float, XMFLOAT3> KeyMap;

    public:
        DynamicValueController()
        {
        }

        ~DynamicValueController()
        {
        }

        void SetMap(const KeyMap& keymap)
        {
            m_keyMap = keymap;
        }

        XMFLOAT3 GetValue(float time) const
        {
            typename KeyMap::const_iterator itUpper = m_keyMap.upper_bound(time + 1e-6f);
            typename KeyMap::const_iterator itLower = itUpper;
            --itLower;

            if (itLower == m_keyMap.end())
            {
                return itUpper->second;
            }

            if (itUpper == m_keyMap.end())
            {
                return itLower->second;
            }

            float lowerTime = itLower->first;
            const XMFLOAT3& lowerVal = itLower->second;
            float upperTime = itUpper->first;
            const XMFLOAT3& upperVal = itUpper->second;

            if (lowerTime == upperTime)
            {
                return lowerVal;
            }

            return interpolate(lowerTime, lowerVal, upperTime, upperVal, time);
        };

        void Clear()
        {
            m_keyMap.clear();
        };

    private:
        XMFLOAT3 interpolate(float lowerTime, const XMFLOAT3& lowerVal, float upperTime, const XMFLOAT3& upperVal, float time) const
        {
            const float tt = (time - lowerTime) / (upperTime - lowerTime);
            XMFLOAT3 result;
            XMStoreFloat3(&result, XMVectorLerp(XMLoadFloat3(&lowerVal), XMLoadFloat3(&upperVal), tt));
            return result;
        };

        KeyMap	m_keyMap;
    };

    class dynamic_sky_pipeline final : public graphics_pipeline {
    public:
        static inline DynamicValueController m_sunLuminanceXYZ;
        static inline DynamicValueController m_skyLuminanceXYZ;
        static constexpr std::size_t grid_size = 32;

        explicit dynamic_sky_pipeline();
        ~dynamic_sky_pipeline() override;

        virtual auto on_bind(vkb::command_buffer& cmd) const -> void override;

        auto render_sky(vkb::command_buffer& cmd) const -> void;
        static auto compute_perez_coeff(float turbidity, eastl::array<XMFLOAT4, 5>& out) noexcept -> void;

    protected:
        virtual auto configure_shaders(eastl::vector<eastl::shared_ptr<shader>>& cfg) -> void override;
        virtual auto configure_pipeline_layout(eastl::vector<vk::DescriptorSetLayout>& layouts, eastl::vector<vk::PushConstantRange>& ranges) -> void override;
        virtual auto configure_rasterizer(vk::PipelineRasterizationStateCreateInfo& cfg) -> void override;
        virtual auto configure_depth_stencil(vk::PipelineDepthStencilStateCreateInfo& cfg) -> void override;

    private:
        eastl::optional<mesh> m_grid {};
    };
}
