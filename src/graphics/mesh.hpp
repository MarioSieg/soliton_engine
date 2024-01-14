// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "prelude.hpp"

#include <assimp/scene.h>

#include "../math/DirectXMath.h"
#include "../math/DirectXCollision.h"
#include "assimp/postprocess.h"

enum struct index_format : std::uint8_t {
    i16,
    i32
};

struct vertex final {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 uv;
    DirectX::XMFLOAT3 tangent;
};

using index = std::uint32_t;

class mesh final : public no_copy, public no_move {
public:
    std::string file_path {"memory"};
    handle<bgfx::VertexBufferHandle> vertex_buffer {};
    handle<bgfx::IndexBufferHandle> index_buffer {};
    std::size_t vertex_count = 0;
    std::size_t index_count = 0;
    DirectX::BoundingBox aabb {};

    mesh(
        std::span<const vertex> vertices,
        std::span<const index> indices,
        index_format format = index_format::i32
    );
    mesh(std::string&& path, std::underlying_type_t<aiPostProcessSteps> post_process_steps = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);
    auto render(bgfx::ViewId view, bgfx::ProgramHandle program) const -> void;
};

extern const bgfx::VertexLayout k_vertex_layout;
