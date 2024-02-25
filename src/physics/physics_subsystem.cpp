// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "physics_subsystem.hpp"
#include "../core/kernel.hpp"

#include <mimalloc.h>
#define RND_IMPLEMENTATION
#include <execution>
#include <rnd.h>
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/PhysicsMaterialSimple.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include "../graphics/graphics_subsystem.hpp"
#include "../scripting/scripting_subsystem.hpp"

using scripting::scripting_subsystem;

namespace physics {
	namespace Layers {
		static constexpr JPH::ObjectLayer NON_MOVING = 0;
		static constexpr JPH::ObjectLayer MOVING = 1;
		static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
	};
	namespace BroadPhaseLayers {
		static constexpr JPH::BroadPhaseLayer NON_MOVING {0};
		static constexpr JPH::BroadPhaseLayer MOVING {1};
		static constexpr JPH::uint NUM_LAYERS = 2;
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
		virtual JPH::uint GetNumBroadPhaseLayers() const override {
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

	[[maybe_unused]]
	static thread_local rnd_gamerand_t prng;

	[[maybe_unused]]
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
        JPH::AlignedAllocate = +[](const std::size_t size, const std::size_t align) -> void* {
            return mi_malloc_aligned(size, align);
        };
        JPH::AlignedFree = +[](void* ptr) -> void {
            mi_free(ptr);
        };
        JPH::Trace = &trace_proc;
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
        m_temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(
			scripting_subsystem::get_config_table()["Physics"]["tempAllocatorBufferSize"].cast<std::size_t>().valueOr(32ull << 20)
        );
    	const auto num_threads = scripting_subsystem::get_config_table()["Threads"]["physicsThreads"].cast<std::uint32_t>().valueOr(1);
        m_job_system = std::make_unique<JPH::JobSystemThreadPool>(
        	JPH::cMaxPhysicsJobs,
        	JPH::cMaxPhysicsBarriers,
        	num_threads
        );
    	m_broad_phase = std::make_unique<BPLayerInterfaceImpl>();
		m_broad_phase_filter = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
    	m_object_layer_pair_filter = std::make_unique<ObjectLayerPairFilterImpl>();
    	m_contact_listener = std::make_unique<ContactListenerImpl>();

    	m_physics_system.Init(
    		scripting_subsystem::get_config_table()["Physics"]["maxRigidBodies"].cast<std::uint32_t>().valueOr(0x1000),
    		scripting_subsystem::get_config_table()["Physics"]["numMutexes"].cast<std::uint32_t>().valueOr(0x1000),
    		scripting_subsystem::get_config_table()["Physics"]["maxBodyPairs"].cast<std::uint32_t>().valueOr(0x1000),
    		scripting_subsystem::get_config_table()["Physics"]["maxContacts"].cast<std::uint32_t>().valueOr(0x1000),
    		*m_broad_phase,
    		*m_broad_phase_filter,
    		*m_object_layer_pair_filter
    	);
    	m_physics_system.SetGravity(JPH::Vec3{0.0f, -9.81f, 0.0f}); // set gravity to earth's gravity
    	m_physics_system.SetContactListener(&*m_contact_listener);
    	rnd_gamerand_seed(&prng, 0xdeadbeef);
    	m_debug_renderer = std::make_unique<debug_renderer>();
    }

    physics_subsystem::~physics_subsystem() {
    	m_debug_renderer.reset();
    	m_contact_listener.reset();
    	m_object_layer_pair_filter.reset();
    	m_broad_phase_filter.reset();
    	m_broad_phase.reset();
        m_job_system.reset();
        m_temp_allocator.reset();
    	JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
    }

    auto physics_subsystem::on_start(scene& scene) -> void {
    	auto& bi = m_physics_system.GetBodyInterface();

    	scene.observer<com::character_controller>().event(flecs::OnAdd).each([&](com::character_controller& cc) {
			static constexpr float cCharacterHeightStanding = 1.35f;
			static constexpr float cCharacterRadiusStanding = 0.3f;
			const JPH::Ref<JPH::Shape> capsule = JPH::RotatedTranslatedShapeSettings{
				JPH::Vec3{},
				JPH::Quat::sIdentity(),
				new JPH::CapsuleShape(0.5f * cCharacterHeightStanding, cCharacterRadiusStanding)
			}.Create().Get();
			JPH::Ref<JPH::CharacterSettings> settings = new JPH::CharacterSettings();
			settings->mMaxSlopeAngle = JPH::DegreesToRadians(55.0f);
    		settings->mMass = 80.0f;
    		settings->mGravityFactor = 2.0f;
			settings->mLayer = Layers::MOVING;
			settings->mShape = capsule;
			settings->mFriction = 0.5f;
			settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -cCharacterRadiusStanding); // Accept contacts that touch the lower sphere of the capsule
			JPH::Ref<JPH::Character> character = new JPH::Character(settings, {}, JPH::Quat::sIdentity(), 0, &m_physics_system);
			character->AddToPhysicsSystem(JPH::EActivation::Activate);
			cc.characer = character;
		});

    	scene.observer<com::character_controller>().event(flecs::OnRemove).each([&](com::character_controller& cc) {
    		cc.characer->RemoveFromPhysicsSystem();
		});

		log_info("Creating static colliders...");

    	static constexpr auto create_static_body = [](const com::transform& transform, const com::mesh_renderer& renderer) -> std::optional<JPH::BodyCreationSettings> {
    		if (renderer.meshes.empty()) [[unlikely]] {
				return std::nullopt;
    		}
    		const auto& mesh = renderer.meshes.front();
    		JPH::Shape::ShapeResult result {};
    		const JPH::Vec3 pos {
    			transform.position.x,
				transform.position.y,
				transform.position.z
			};
    		const JPH::Quat rot {
    			transform.rotation.x,
				transform.rotation.y,
				transform.rotation.z,
				transform.rotation.w,
			};
    		auto verts = mesh->verts;
    		const auto scale = DirectX::XMLoadFloat3(&transform.scale);
    		for (JPH::Float3& vert : verts) {
    			static_assert(sizeof(JPH::Float3) == sizeof(DirectX::XMFLOAT3));
    			static_assert(alignof(JPH::Float3) == alignof(DirectX::XMFLOAT3));
    			auto* punned = reinterpret_cast<DirectX::XMFLOAT3*>(&vert);
    			DirectX::XMStoreFloat3(punned, DirectX::XMVectorMultiply(DirectX::XMLoadFloat3(punned), scale));
    		}
    		JPH::BodyCreationSettings static_object {
    			new JPH::MeshShape(JPH::MeshShapeSettings{verts, mesh->triangles}, result),
				pos,
				rot,
				JPH::EMotionType::Static,
				Layers::NON_MOVING
			};
    		// make static objects concrete like:
    		static_object.mFriction = 0.9f;
    		static_object.mRestitution = 0.0f;
    		return static_object;
    	};

    	auto filter = scene.filter<const com::transform, const com::mesh_renderer>();
		std::vector<std::pair<std::span<const com::transform>, std::span<const com::mesh_renderer>>> targets {};
    	std::size_t total = 0;
    	filter.iter([&](flecs::iter i, const com::transform* transform, const com::mesh_renderer* renderer) {
    		const std::size_t n = i.count();
    		total += n;
    		targets.emplace_back(std::span{transform, n}, std::span{renderer, n});
		});
    	std::vector<std::optional<JPH::BodyCreationSettings>> bodies {};
    	bodies.resize(total);
    	for (std::size_t base_idx = 0; auto&& [transforms, renderers] : targets) {
			passert(transforms.size() == renderers.size());
    		std::for_each(std::execution::par_unseq, std::begin(transforms), std::end(transforms), [&](const com::transform& transform) {
    			const auto index = &transform - &transforms.front();
    			const auto& renderer = renderers[index];
    			const auto body = create_static_body(transform, renderer);
    			if (body) [[likely]] {
					bodies[base_idx + index] = body;
				}
    		});
    	}
    	for (const auto& body : bodies) {
			if (body) [[likely]] {
				bi.CreateAndAddBody(*body, JPH::EActivation::DontActivate);
			}
    	}

    	m_physics_system.OptimizeBroadPhase();
    }

    HOTPROC auto physics_subsystem::on_post_tick() -> void {
    	auto& active = scene::get_active();
    	auto& bi = m_physics_system.GetBodyInterface();

    	const double delta = kernel::get().get_delta_time();
    	const int n_steps = 1;
    	m_physics_system.Update(static_cast<float>(delta), n_steps, &*m_temp_allocator, &*m_job_system);

    	// sync loop 1 (rigidbody <-> transform)
    	active.filter<com::rigidbody, com::transform>().each([&](com::rigidbody& rb, com::transform& transform) {
		    const JPH::BodyID body_id = rb.body_id;
		    const JPH::Vec3 pos = bi.GetPosition(body_id);
			transform.position.x = pos.GetX();
    		transform.position.y = pos.GetY();
    		transform.position.z = pos.GetZ();
    		transform.rotation = std::bit_cast<DirectX::XMFLOAT4>(bi.GetRotation(body_id));
		});

    	// sync loop 2 (character controller <-> transform)
    	active.filter<com::character_controller, com::transform>().each([&](com::character_controller& cc, com::transform& transform) {
    		cc.characer->PostSimulation(0.05f);
			const JPH::Vec3 pos = cc.characer->GetPosition();
			transform.position.x = pos.GetX();
			transform.position.y = pos.GetY();
			transform.position.z = pos.GetZ();
			transform.rotation = std::bit_cast<DirectX::XMFLOAT4>(cc.characer->GetRotation());
		});
    }
}
