// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "physics_subsystem.hpp"
#include "../core/kernel.hpp"

#include <mimalloc.h>
#define RND_IMPLEMENTATION
#include <rnd.h>
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
#include <Jolt/Physics/Collision/PhysicsMaterialSimple.h>
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

	static thread_local rnd_gamerand_t prng;

	static inline auto next_f32_in_range(const float min, const float max) noexcept -> float {
		return min + (max - min) * rnd_gamerand_nextf(&prng);
	}

	class ContactListenerImpl : public JPH::ContactListener {
		virtual auto OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
		                            const JPH::ContactManifold& inManifold,
		                            JPH::ContactSettings& ioSettings) -> void override {
			//const JPH::Vec3 offset {
			//	next_f32_in_range(0.01f, 0.1f),
			//	next_f32_in_range(0.01f, 0.1f),
			//	next_f32_in_range(0.01f, 0.1f)
			//};
			//ioSettings.mRelativeAngularSurfaceVelocity = offset;
			ioSettings.mCombinedFriction = std::sqrt(inBody1.GetFriction() * inBody2.GetFriction());
			ioSettings.mCombinedRestitution = std::max(inBody1.GetRestitution(), inBody2.GetRestitution());
		}
		virtual auto OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2,
		                                const JPH::ContactManifold& inManifold,
		                                JPH::ContactSettings& ioSettings) -> void override {
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
    	m_contact_listener = std::make_unique<ContactListenerImpl>();

    	m_physics_system.Init(
    		k_max_bodies,
    		k_num_mutexes,
    		k_max_body_pairs,
    		k_max_contacts,
    		*m_broad_phase,
    		*m_broad_phase_filter,
    		*m_object_layer_pair_filter
    	);
    	m_physics_system.SetGravity(JPH::Vec3{0.0f, -9.81f, 0.0f}); // set gravity to earth's gravity
    	m_physics_system.SetContactListener(&*m_contact_listener);
    	rnd_gamerand_seed(&prng, 0xdeadbeef);
    }

    physics_subsystem::~physics_subsystem() {
    	m_contact_listener.reset();
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
#if 0
    	auto* sphere_mesh = new graphics::mesh("assets/meshes/melon/melon.obj");
    	auto* mat = new graphics::material{};
    	auto* albedo = new graphics::texture("assets/meshes/melon/albedo.jpg");
    	auto* normal = new graphics::texture("assets/meshes/melon/normal.jpg");
    	mat->albedo_map = albedo;
    	mat->normal_map = normal;
    	mat->flush_property_updates();

    	scene.filter<const c_transform, const c_mesh_renderer>().each([&](const c_transform& transform, const c_mesh_renderer& renderer) {
    		if (renderer.meshes.empty()) {
    			return;
    		}
    		const auto& mesh = renderer.meshes.front();
    		JPH::Shape::ShapeResult result {};
		    JPH::BodyCreationSettings static_object {
				 new JPH::MeshShape(JPH::MeshShapeSettings{mesh->verts, mesh->triangles}, result),
				lunam_vec4_to_jbh_vec3(transform.position),
				std::bit_cast<JPH::Quat>(transform.rotation),
				JPH::EMotionType::Static,
				Layers::NON_MOVING
			};
    		// make static objects concrete like:
    		static_object.mFriction = 0.9f;
    		static_object.mRestitution = 0.0f;
			bi.CreateAndAddBody(static_object, JPH::EActivation::DontActivate);
    	});

    	const auto make_sphere = [&](float x, float y, float z) {
    		flecs::entity sphere = scene.spawn(nullptr);
    		c_transform* transform = sphere.get_mut<c_transform>();
    		transform->position.x = x;
    		transform->position.y = y;
    		transform->position.z = z;
    		transform->scale.x = 0.01f;
    		transform->scale.y = 0.01f;
    		transform->scale.z = 0.01f;
    		c_mesh_renderer* renderer = sphere.get_mut<c_mesh_renderer>();
    		renderer->meshes.emplace_back(sphere_mesh);
    		renderer->materials.emplace_back(mat);
    		JPH::BodyCreationSettings sphere_settings {
    			new JPH::SphereShape{1.0f*0.15f},
				lunam_vec4_to_jbh_vec3(sphere.get<c_transform>()->position),
				std::bit_cast<JPH::Quat>(sphere.get<c_transform>()->rotation),
				JPH::EMotionType::Kinematic,
				Layers::MOVING
			};
    		// set realistic soccer ball properties like mass, friction, etc.
    		sphere_settings.mMassPropertiesOverride = JPH::MassProperties{1.0f};
    		sphere_settings.mLinearDamping = 0.1f;
    		sphere_settings.mAngularDamping = 0.1f;
    		sphere_settings.mRestitution = 0.5f;
    		sphere_settings.mFriction = 0.5f;
    		sphere_settings.mAllowDynamicOrKinematic = true;

    		JPH::BodyID sphere_body = bi.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
    		sphere.get_mut<c_rigidbody>()->body_id = sphere_body;
    	};

    	for (int i = 0; i < 26; i++) {
			for (int j = 0; j < 26; j++) {
				for (int k = 0; k < 26; k++) {
					make_sphere(-30.0f+i, 20.0f+j, -30.0f+k);
				}
			}
		}

    	log_info("Balls: {}", 26*26*26);
#endif

    	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
    	// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
    	// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
    	m_physics_system.OptimizeBroadPhase();
    }

    HOTPROC auto physics_subsystem::on_post_tick() -> void {
    	const double delta = kernel::get().get_delta_time();
    	const int n_steps = 1;
    	m_physics_system.Update(static_cast<float>(delta), n_steps, &*m_temp_allocator, &*m_job_system);
    	const auto& active = scene::get_active();
    	if (!active) [[unlikely]] {
    		return;
    	}
    	auto& bi = m_physics_system.GetBodyInterface();
#if 0
    	bool is_key_down = ImGui::IsKeyPressed(ImGuiKey_Space, false);
    	active->filter<c_rigidbody, c_transform>().each([&](c_rigidbody& rb, c_transform& transform) {
			JPH::BodyID body_id = rb.body_id;
    		if (is_key_down) [[unlikely]] {
				bi.SetMotionType(body_id, JPH::EMotionType::Dynamic, JPH::EActivation::Activate);
			}
    		JPH::Vec3 pos = bi.GetPosition(body_id);
			transform.position.x = pos.GetX();
    		transform.position.y = pos.GetY();
    		transform.position.z = pos.GetZ();
    		transform.rotation = std::bit_cast<DirectX::XMFLOAT4>(bi.GetRotation(body_id));
		});
#endif
    }
}
