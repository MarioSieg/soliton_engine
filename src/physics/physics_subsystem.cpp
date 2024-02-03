// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "physics_subsystem.hpp"
#include "../core/kernel.hpp"

#include <mimalloc.h>
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include "../graphics/graphics_subsystem.hpp"

namespace physics {
	namespace Layers {
		static constexpr JPH::ObjectLayer NON_MOVING = 0;
		static constexpr JPH::ObjectLayer MOVING = 1;
		static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
	};
	namespace BroadPhaseLayers {
		static constexpr JPH::BroadPhaseLayer NON_MOVING {0};
		static constexpr JPH::BroadPhaseLayer MOVING {1};
		static constexpr uint NUM_LAYERS = 2;
	};

	// BroadPhaseLayerInterface implementation
	// This defines a mapping between object and broadphase layers.
	class BPLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface {
	public:
		BPLayerInterfaceImpl() {
			// Create a mapping table from object to broad phase layer
			mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
			mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
		}
		virtual uint GetNumBroadPhaseLayers() const override {
			return BroadPhaseLayers::NUM_LAYERS;
		}
		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
			JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
			return mObjectToBroadPhase[inLayer];
		}
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
			return "";
		}
#endif
	private:
		JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS] {};
	};

	/// Class that determines if an object layer can collide with a broadphase layer
	class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
			switch (inLayer1) {
				case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayers::MOVING;
				case Layers::MOVING: return true;
				default: return false;
			}
		}
	};

	/// Class that determines if two object layers can collide
	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
			switch (inObject1) {
				case Layers::NON_MOVING: return inObject2 == Layers::MOVING; // Non moving only collides with moving
				case Layers::MOVING: return true; // Moving collides with everything
				default: return false;
			}
		}
	};

    static auto trace_proc(const char* msg, ...) -> void {
        va_list list;
        va_start(list, msg);
        char buf[0x1000];
        vsnprintf(buf, sizeof(buf), msg, list);
        va_end(list);
        log_info("[Physics]: ", buf);
    }

    physics_subsystem::physics_subsystem() : subsystem{"Physics"} {
        JPH::RegisterDefaultAllocator(); // TODO mimalloc
        JPH::Allocate = +[](const std::size_t size) -> void* {
              return mi_malloc(size);
        };
        JPH::Free = +[](void* ptr) -> void {
            mi_free(ptr);
        };
        //JPH::AlignedAllocate = +[](const std::size_t size, const std::size_t align) -> void* {
        //    return mi_malloc_aligned(size, align);
        //};
        //JPH::AlignedFree = +[](void* ptr) -> void {
        //    mi_free_aligned(ptr, 0);
        //};
        JPH::Trace = &trace_proc;
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
        m_temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(k_temp_allocator_size);
        m_job_system = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, k_num_threads);
    	m_broad_phase = std::make_unique<BPLayerInterfaceImpl>();
		m_broad_phase_filter = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
    	m_object_layer_pair_filter = std::make_unique<ObjectLayerPairFilterImpl>();

    	m_physics_system.Init(
    		k_max_bodies,
    		k_num_mutexes,
    		k_max_body_pairs,
    		k_max_contacts,
    		*m_broad_phase,
    		*m_broad_phase_filter,
    		*m_object_layer_pair_filter
    	);
    }

    physics_subsystem::~physics_subsystem() {
    	m_object_layer_pair_filter.reset();
    	m_broad_phase_filter.reset();
    	m_broad_phase.reset();
        m_job_system.reset();
        m_temp_allocator.reset();
    	JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
    }

	[[nodiscard]] static inline auto lunam_vec3_to_jbh_vec3(const DirectX::XMFLOAT3& v) noexcept -> JPH::RVec3 {
    	return JPH::Vec3{v.x, v.y, v.z};
    }

	[[nodiscard]] static inline auto lunam_vec4_to_jbh_vec3(const DirectX::XMFLOAT4& v) noexcept -> JPH::RVec3 {
    	return JPH::Vec3{v.x, v.y, v.z};
    }

    auto physics_subsystem::on_start(scene& scene) -> void {
    	auto& bi = m_physics_system.GetBodyInterface();

    	auto* sphere_mesh = new graphics::mesh("assets/meshes/fussball/fussball.fbx");
    	auto* mat = new graphics::material{};
    	auto* albedo = new graphics::texture("assets/meshes/fussball/albedo.png");
    	auto* normal = new graphics::texture("assets/meshes/fussball/normal.png");
    	mat->albedo_map = albedo;
    	mat->normal_map = normal;
    	mat->flush_property_updates();

    	scene.filter<const c_transform, const c_mesh_renderer>().each([&](const c_transform& transform, const c_mesh_renderer& renderer) {
    		if (renderer.meshes.empty()) {
    			return;
    		}
    		const auto& mesh = renderer.meshes.front();
    		JPH::Shape::ShapeResult result {};
		    const JPH::BodyCreationSettings static_object {
				 new JPH::MeshShape(JPH::MeshShapeSettings{mesh->verts, mesh->triangles}, result),
				lunam_vec4_to_jbh_vec3(transform.position),
				std::bit_cast<JPH::Quat>(transform.rotation),
				JPH::EMotionType::Static,
				Layers::NON_MOVING
			};
			bi.CreateAndAddBody(static_object, JPH::EActivation::DontActivate);
    	});

    	const auto make_sphere = [&](float x, float z) {
    		flecs::entity sphere = scene.spawn(nullptr);
    		c_transform* transform = sphere.get_mut<c_transform>();
    		transform->position.x = x;
    		transform->position.z = z;
    		transform->position.y = 150.0f;
    		transform->scale.x = 0.0005f;
    		transform->scale.y = 0.0005f;
    		transform->scale.z = 0.0005f;
    		c_mesh_renderer* renderer = sphere.get_mut<c_mesh_renderer>();
    		renderer->meshes.emplace_back(sphere_mesh);
    		renderer->materials.emplace_back(mat);
    		JPH::BodyCreationSettings sphere_settings {
    			new JPH::SphereShape{1.0f*0.25f},
				lunam_vec4_to_jbh_vec3(sphere.get<c_transform>()->position),
				std::bit_cast<JPH::Quat>(sphere.get<c_transform>()->rotation),
				JPH::EMotionType::Dynamic,
				Layers::MOVING
			};
    		// set realistic soccer ball properties like mass, friction, etc.
    		sphere_settings.mMassPropertiesOverride = JPH::MassProperties{1.0f};
    		sphere_settings.mLinearDamping = 0.1f;
    		sphere_settings.mAngularDamping = 0.1f;
    		sphere_settings.mRestitution = 0.5f;
    		sphere_settings.mFriction = 0.5f;

    		JPH::BodyID sphere_body = bi.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
    		sphere.get_mut<c_rigidbody>()->body_id = sphere_body;
    	};

    	for (int i = 0; i < 64; i++) {
			for (int j = 0; j < 64; j++) {
				make_sphere(-64.0f + i * 2.0f, -64.0f + j * 2.0f);
			}
		}

    	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
    	// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
    	// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
    	m_physics_system.OptimizeBroadPhase();
    }

    auto physics_subsystem::on_post_tick() -> void {
    	const double delta = kernel::get().get_delta_time();
    	const int n_steps = 1;
    	m_physics_system.Update(static_cast<float>(delta), n_steps, &*m_temp_allocator, &*m_job_system);
    	const auto& active = scene::get_active();
    	if (!active) [[unlikely]] {
    		return;
    	}
    	auto& bi = m_physics_system.GetBodyInterface();
    	active->filter<c_rigidbody, c_transform>().each([&](c_rigidbody& rb, c_transform& transform) {
			JPH::BodyID body_id = rb.body_id;
    		JPH::Vec3 pos = bi.GetPosition(body_id);
			transform.position.x = pos.GetX();
    		transform.position.y = pos.GetY();
    		transform.position.z = pos.GetZ();
    		transform.rotation = std::bit_cast<DirectX::XMFLOAT4>(bi.GetRotation(body_id));
		});
    }
}
