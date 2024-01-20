// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "mesh.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/cimport.h>

using namespace DirectX;

static_assert(sizeof(DirectX::XMFLOAT3) == sizeof(aiVector3D));
static_assert(alignof(DirectX::XMFLOAT3) == alignof(aiVector3D));

auto compute_aabb(const aiMesh& mesh) noexcept -> BoundingBox {
    XMVECTOR min { XMVectorReplicate(1E10F) };
    XMVECTOR max { XMVectorReplicate(-1E10F) };
    XMVECTOR tmp;
    for (const aiVector3D* __restrict__ vertex = mesh.mVertices, *__restrict__ const end = vertex + mesh.mNumVertices; vertex < end; ++vertex) {
        tmp = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(vertex));
        min = XMVectorMin(min, tmp);
        max = XMVectorMax(max, tmp);
    }
    BoundingBox r { };
    BoundingBox::CreateFromPoints(r, min, max);
    return r;
}

auto compute_aabb(const aiMesh& mesh, FXMMATRIX transform) noexcept -> BoundingBox {
    XMVECTOR min { XMVectorReplicate(1E10F) };
    XMVECTOR max { XMVectorReplicate(-1E10F) };
    XMVECTOR tmp;
    for (const aiVector3D* __restrict__ vertex = mesh.mVertices, *__restrict__ const end = vertex + mesh.mNumVertices; vertex < end; ++vertex) {
        tmp = XMVector3Transform(XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(vertex)), transform);
        min = XMVectorMin(min, tmp);
        max = XMVectorMax(max, tmp);
    }
    BoundingBox r { };
    BoundingBox::CreateFromPoints(r, min, max);
    return r;
}

auto compute_aabb(const std::span<const vertex> vertices) noexcept -> BoundingBox {
    XMVECTOR min { XMVectorReplicate(1e10f) };
    XMVECTOR max { XMVectorReplicate(-1e10f) };
    XMVECTOR tmp;
    for (const vertex* __restrict__ vertex = vertices.data(), *__restrict__ const end = vertex + vertices.size(); vertex < end; ++vertex) {
        tmp = XMLoadFloat3(&vertex->position);
        min = XMVectorMin(min, tmp);
        max = XMVectorMax(max, tmp);
    }
    BoundingBox r { };
    BoundingBox::CreateFromPoints(r, min, max);
    return r;
}

static const bgfx::VertexLayout k_vertex_layout = [] {
    bgfx::VertexLayout layout { };
    layout
    .begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float)
    .end();
    passert(layout.getStride() == sizeof(vertex));
    passert(layout.getOffset(bgfx::Attrib::Position) == offsetof(vertex, position));
    passert(layout.getOffset(bgfx::Attrib::Normal) == offsetof(vertex, normal));
    passert(layout.getOffset(bgfx::Attrib::TexCoord0) == offsetof(vertex, uv));
    passert(layout.getOffset(bgfx::Attrib::Tangent) == offsetof(vertex, tangent));
    return layout;
}();

mesh::mesh(const std::span<const vertex> vertices, const std::span<const index> indices, const index_format format) {
    passert(!vertices.empty());
    passert(!indices.empty());
    const bgfx::VertexBufferHandle vb = [&vertices]{
        const bgfx::Memory* const vbmem = bgfx::alloc(vertices.size()*sizeof(vertex));
        passert(vbmem != nullptr);
        const vertex* __restrict__ const begin = &*vertices.begin();
        const vertex* __restrict__ const end = begin + vertices.size();
        auto* __restrict__ const dst = reinterpret_cast<vertex*>(vbmem->data);
        std::copy(begin, end, dst);
        const bgfx::VertexBufferHandle vb = bgfx::createVertexBuffer(vbmem, k_vertex_layout);
        passert(bgfx::isValid(vb));
        return vb;
    }();
    const bgfx::IndexBufferHandle ib = [&indices, format]{
        const bgfx::Memory* const ibmem { bgfx::alloc(indices.size()*sizeof(index)) };
        passert(ibmem != nullptr);
        const index* __restrict__ const begin = &*indices.begin();
        const index* __restrict__ const end = begin + indices.size();
        auto* __restrict__ const dst = reinterpret_cast<index*>(ibmem->data);
        std::copy(begin, end, dst);
        const auto indexFormat { format == index_format::i32 ? BGFX_BUFFER_INDEX32 : BGFX_BUFFER_NONE };
        const bgfx::IndexBufferHandle ib = bgfx::createIndexBuffer(ibmem, indexFormat);
        passert(bgfx::isValid(ib));
        return ib;
    }();
    vertex_buffer = handle{vb};
    index_buffer = handle{ib};
    vertex_count = vertices.size();
    index_count = indices.size();
    aabb = compute_aabb(vertices);
}

mesh::mesh(std::string&& path, const std::underlying_type_t<aiPostProcessSteps> post_process_steps) {
    log_info("Loading mesh from '{}'", path, post_process_steps);
    Assimp::Importer importer { };
    const aiScene* const scene = importer.ReadFile(path, post_process_steps);
    passert(scene && scene->mNumMeshes > 0);
    const aiMesh& assimpMesh = **scene->mMeshes;
    passert(assimpMesh.mNumFaces > 0);
    passert(assimpMesh.mNumVertices > 0);
    passert(assimpMesh.HasPositions());
    const bgfx::Memory* const ibmem = bgfx::alloc(static_cast<std::size_t>(assimpMesh.mNumFaces)*3*sizeof(index));
    passert(ibmem != nullptr);
    std::memset(ibmem->data, 0, ibmem->size);
    auto* idst = reinterpret_cast<index*>(ibmem->data);
    for (const aiFace* i = assimpMesh.mFaces, *const e = assimpMesh.mFaces + assimpMesh.mNumFaces; i < e; ++i) {
        if (i->mNumIndices != 3) [[unlikely]] continue;
        const std::uint32_t* j = i->mIndices;
        *idst++ = static_cast<index>(*j);
        *idst++ = static_cast<index>(*++j);
        *idst++ = static_cast<index>(*++j);
        assert(idst <= reinterpret_cast<index*>(ibmem->data) + ibmem->size/sizeof(index));
    }
    const bgfx::Memory* const vbmem = bgfx::alloc(assimpMesh.mNumVertices*sizeof(vertex));
    passert(vbmem != nullptr);
    std::memset(vbmem->data, 0, vbmem->size);
    std::span outVertices { reinterpret_cast<vertex*>(vbmem->data), assimpMesh.mNumVertices };
    for (const aiVector3D* vt = assimpMesh.mVertices; vertex& v : outVertices) {
        v.position = std::bit_cast<XMFLOAT3>(*vt++);
    }
    static constexpr auto channel_uv = 0;
    if (assimpMesh.HasTextureCoords(channel_uv)) [[likely]] {
        for (const aiVector3D* vt = &assimpMesh.mTextureCoords[channel_uv][0]; vertex& v : outVertices) {
            std::memcpy(&v.uv, vt, sizeof(XMFLOAT2)); // only copy first 2 floats
            ++vt;
        }
    }
    if (assimpMesh.HasNormals()) [[likely]] {
        for (const aiVector3D* vt = assimpMesh.mNormals; vertex& v : outVertices) {
            v.normal = std::bit_cast<XMFLOAT3>(*vt++);
        }
    }
    if (assimpMesh.HasTangentsAndBitangents()) [[likely]] {
        for (const aiVector3D* vt = assimpMesh.mTangents; vertex& v : outVertices) {
            v.tangent = std::bit_cast<XMFLOAT3>(*vt++);
        }
    }
    vertex_buffer = handle{bgfx::createVertexBuffer(vbmem, k_vertex_layout)};
    index_buffer = handle{bgfx::createIndexBuffer(ibmem, BGFX_BUFFER_INDEX32)};
    file_path = std::move(path);
}

auto mesh::render(const bgfx::ViewId view, const bgfx::ProgramHandle program) const -> void {
    bgfx::setVertexBuffer(0, *vertex_buffer);
    bgfx::setIndexBuffer(*index_buffer);
    bgfx::submit(view, program);
}
