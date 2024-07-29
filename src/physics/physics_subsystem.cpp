// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "physics_subsystem.hpp"
#include "../core/kernel.hpp"

#if USE_MIMALLOC
#include <mimalloc.h>
#endif

#define RND_IMPLEMENTATION
#include <execution>
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
#include "../scripting/convar.hpp"
#include "Jolt/Physics/Collision/Shape/ScaledShape.h"

namespace lu::physics {
    using scripting::scripting_subsystem;

	namespace Layers {
		static constexpr JPH::ObjectLayer NON_MOVING = 0;
		static constexpr JPH::ObjectLayer MOVING = 1;
		static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
	}
	namespace BroadPhaseLayers {
		static constexpr JPH::BroadPhaseLayer NON_MOVING {0};
		static constexpr JPH::BroadPhaseLayer MOVING {1};
		static constexpr JPH::uint NUM_LAYERS = 2;
	}

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

	class ContactListenerImpl : public JPH::ContactListener {
		virtual auto OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
		                            const JPH::ContactManifold& inManifold,
		                            JPH::ContactSettings& ioSettings) -> void override {
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
        log_info("[Physics]: {}", buf);
    }

    static convar<std::uint64_t> cv_tmp_allocator_buffer_size {"Physics.tempAllocatorBufferSize", 32ull << 20, convar_flags::read_only};
    static convar<std::uint32_t> cv_num_physics_threads {"Threads.physicsThreads", 1u, convar_flags::read_only};
    static convar<std::uint32_t> cv_max_rigid_bodies {"Physics.maxRigidBodies", 0x1000u, convar_flags::read_only};
    static convar<std::uint32_t> cv_num_mutexes {"Physics.numMutexes", 0x1000u, convar_flags::read_only};
    static convar<std::uint32_t> cv_max_body_pairs {"Physics.maxBodyPairs", 0x1000u, convar_flags::read_only};
    static convar<std::uint32_t> cv_max_contacts {"Physics.maxContacts", 0x1000u, convar_flags::read_only};

    physics_subsystem::physics_subsystem() : subsystem{"Physics"} {
#if USE_MIMALLOC
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
#else
        JPH::RegisterDefaultAllocator();
#endif
        JPH::Trace = &trace_proc;
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
        m_temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(cv_tmp_allocator_buffer_size());
        m_job_system = std::make_unique<JPH::JobSystemThreadPool>(
        	JPH::cMaxPhysicsJobs,
        	JPH::cMaxPhysicsBarriers,
            cv_num_physics_threads()
        );
    	m_broad_phase = std::make_unique<BPLayerInterfaceImpl>();
		m_broad_phase_filter = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
    	m_object_layer_pair_filter = std::make_unique<ObjectLayerPairFilterImpl>();
    	m_contact_listener = std::make_unique<ContactListenerImpl>();

    	m_physics_system.Init(
            cv_max_rigid_bodies(),
            cv_num_mutexes(),
            cv_max_body_pairs(),
            cv_max_contacts(),
    		*m_broad_phase,
    		*m_broad_phase_filter,
    		*m_object_layer_pair_filter
    	);
    	m_physics_system.SetGravity(JPH::Vec3{0.0f, -9.81f, 0.0f}); // set gravity to earth's gravity
    	m_physics_system.SetContactListener(&*m_contact_listener);
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
			cc.phys_character = character;
		});

    	scene.observer<com::character_controller>().event(flecs::OnRemove).each([&](com::character_controller& cc) {
    		cc.phys_character->RemoveFromPhysicsSystem();
		});

		log_info("Creating static colliders...");

    	auto filter = scene.filter<const com::transform, const com::mesh_renderer>();
		eastl::vector<std::pair<std::span<const com::transform>, std::span<const com::mesh_renderer>>> targets {};
    	std::size_t total = 0;
    	filter.iter([&](flecs::iter i, const com::transform* transform, const com::mesh_renderer* renderer) {
    		const std::size_t n = i.count();
    		total += n;
    		targets.emplace_back(std::span{transform, n}, std::span{renderer, n});
		});
    	eastl::vector<JPH::BodyID> bodies {};
    	bodies.resize(total);
    	for (std::size_t base_idx = 0; auto&& target : targets) {
    		const auto& transforms = target.first;
    		const auto& renderers = target.second;
			passert(transforms.size() == renderers.size());
            const auto exector = [&](const com::transform& transform) {
                const auto index = &transform - &transforms.front();
                const auto& renderer = renderers[index];
                if (!renderer.meshes.empty()) [[likely]] {
                    JPH::BodyCreationSettings ci {};
                    create_static_body(ci, transform, renderer);
                    bodies[base_idx + index] = bi.CreateAndAddBody(ci, JPH::EActivation::DontActivate);
                }
            };
#if PLATFORM_OSX
            std::for_each(std::begin(transforms), std::end(transforms), exector);
#else
            std::for_each(std::execution::par_unseq, std::begin(transforms), std::end(transforms), exector);
#endif
    	}
    	for (auto old_body : m_static_bodies) {
    		bi.RemoveBody(old_body);
    	}
        m_static_bodies = std::move(bodies);

        log_info("Optimizing broadphase...");
    	m_physics_system.OptimizeBroadPhase();
    }

    HOTPROC auto physics_subsystem::on_post_tick() -> void {
    	const double delta = kernel::get().get_delta_time();
    	const int n_steps = 1;
    	m_physics_system.Update(static_cast<float>(delta), n_steps, &*m_temp_allocator, &*m_job_system);
    	post_sync();
    }

	// Sync transforms from physics to game objects
    auto physics_subsystem::post_sync() const -> void {
    	auto& active = scene::get_active();
    	JPH::BodyInterface& bi = m_physics_system.GetBodyInterface();

    	// sync loop 1 rigidbody => transform
    	active.filter<const com::rigidbody, com::transform>().each([&](const com::rigidbody& rb, com::transform& transform) {
			const JPH::BodyID body_id = rb.phys_body;
			transform.position = std::bit_cast<DirectX::XMFLOAT4>(bi.GetPosition(body_id));
			transform.rotation = std::bit_cast<DirectX::XMFLOAT4>(bi.GetRotation(body_id));
		});

    	// sync loop 2 character controller => transform
    	active.filter<const com::character_controller, com::transform>().each([&](const com::character_controller& cc, com::transform& transform) {
			cc.phys_character->PostSimulation(0.05f);
    		transform.position = std::bit_cast<DirectX::XMFLOAT4>(cc.phys_character->GetPosition());
			transform.rotation = std::bit_cast<DirectX::XMFLOAT4>(cc.phys_character->GetPosition());
		});
    }

    auto physics_subsystem::create_static_body(JPH::BodyCreationSettings& ci, const com::transform& transform, const com::mesh_renderer& renderer) -> void {
        passert(!renderer.meshes.empty());
        const auto& mesh = renderer.meshes.front();
        JPH::Shape::ShapeResult result {};
        JPH::Vec3 pos;
        static_assert(sizeof(pos) == sizeof(DirectX::XMFLOAT3A));
        DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3A*>(&pos), DirectX::XMLoadFloat4(&transform.position));
        JPH::Quat rot;
        static_assert(sizeof(rot) == sizeof(DirectX::XMFLOAT4));
        DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(&rot), DirectX::XMQuaternionNormalize(DirectX::XMLoadFloat4(&transform.rotation)));
        auto shape = new JPH::MeshShape{JPH::MeshShapeSettings{mesh->verts, mesh->triangles}, result};
        DirectX::XMFLOAT3A scale {};
        DirectX::XMStoreFloat3A(&scale, DirectX::XMLoadFloat4(&transform.scale));
        ci = {
            new JPH::ScaledShape{shape, std::bit_cast<JPH::Vec3>(scale)},
            pos,
            rot,
            JPH::EMotionType::Static,
            Layers::NON_MOVING
        };
    }
}
