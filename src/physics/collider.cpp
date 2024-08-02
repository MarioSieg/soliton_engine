
#include "collider.hpp"

namespace lu::physics {
    auto collider::new_box(const DirectX::XMFLOAT3A& half_extent) -> collider {
        return collider{
            shape_type::box,
            eastl::make_unique<JPH::BoxShape>(eastl::bit_cast<JPH::Vec3>(half_extent))
        };
    }

    auto collider::new_sphere(const float radius) -> collider {
        return collider{
            shape_type::sphere,
            eastl::make_unique<JPH::SphereShape>(radius)
        };
    }

    auto collider::new_cylinder(const float half_height, const float radius) -> collider {
        return collider{
            shape_type::cylinder,
            eastl::make_unique<JPH::CylinderShape>(half_height, radius)
        };
    }

    auto collider::new_capsule(const float half_height, const float radius) -> collider {
        return collider{
            shape_type::capsule,
            eastl::make_unique<JPH::CapsuleShape>(half_height, radius)
        };
    }

    auto collider::new_mesh(const eastl::span<const graphics::vertex> vertices, const eastl::span<const graphics::index> indices) -> collider {
        std::vector<JPH::Float3, JPH::STLAllocator<JPH::Float3>> verts {};
        std::vector<JPH::IndexedTriangle, JPH::STLAllocator<JPH::IndexedTriangle>> triangles {};
        verts.reserve(vertices.size());
        triangles.reserve(indices.size() / 3);
        for (auto&& v : vertices) {
            verts.emplace_back(eastl::bit_cast<JPH::Float3>(v.position));
        }
        for (std::size_t i = 0; i < indices.size(); i += 3) {
            JPH::IndexedTriangle tri {};
            tri.mIdx[0] = indices[i];
            tri.mIdx[1] = indices[i+1];
            tri.mIdx[2] = indices[i+2];
            triangles.emplace_back(tri);
        }
        JPH::Shape::ShapeResult result {};
        return collider{
            shape_type::mesh,
            eastl::make_unique<JPH::MeshShape>(JPH::MeshShapeSettings{verts, triangles}, result)
        };
    }
}
