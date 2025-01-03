// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "physics_subsystem.hpp"
#include "../core/kernel.hpp"
#include "../scene/scene_mgr.hpp"
#include "layerdef.hpp"
#include "broad_phase_layer.hpp"
#include "broad_phase_layer_filter.hpp"
#include "contact_listener.hpp"
#include "object_layer_filter.hpp"

#if USE_MIMALLOC
#include <mimalloc.h>
#endif

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
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>

#include "../graphics/graphics_subsystem.hpp"
#include "../core/system_variable.hpp"

namespace soliton::physics {
    static auto trace_proc(const char* msg, ...) -> void {
        va_list list;
        va_start(list, msg);
        eastl::array<char, 0x1000> tmp {};
        vsnprintf(tmp.data(), sizeof(tmp), msg, list);
        va_end(list);
        log_info("[Physics]: {}", tmp.data());
    }

    static const system_variable<std::int64_t> sv_tmp_allocator_buffer_size {"physics.temp_allocator_buffer_size", {32ll << 20}};
    static const system_variable<std::int64_t> sv_num_physics_threads {"cpu.physics_threads", {1}};
    static const system_variable<std::int64_t> sv_max_rigid_bodies {"physics.max_rigid_bodies", {0x1000}};
    static const system_variable<std::int64_t> sv_num_mutexes {"physics.mutex_count", {0x1000}};
    static const system_variable<std::int64_t> sv_max_body_pairs {"physics.max_body_pairs", {0x1000}};
    static const system_variable<std::int64_t> sv_max_contacts {"physics.max_contacts", {0x1000}};

    physics_subsystem::physics_subsystem() : subsystem{"Physics"} {
#if USE_MIMALLOC
        JPH::Allocate = +[](const std::size_t size) -> void* {
              return mi_malloc(size);
        };
        JPH::Reallocate = +[](void* ptr, const std::size_t old, const std::size_t size) -> void* {
            return mi_realloc(ptr, size);
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
        m_temp_allocator = eastl::make_unique<JPH::TempAllocatorImpl>(sv_tmp_allocator_buffer_size());
        m_job_system = eastl::make_unique<JPH::JobSystemThreadPool>(
        	JPH::cMaxPhysicsJobs,
        	JPH::cMaxPhysicsBarriers,
            sv_num_physics_threads()
        );
    	m_broad_phase = eastl::make_unique<broad_phase_layer>();
		m_broad_phase_filter = eastl::make_unique<broad_phase_layer_filter>();
    	m_object_layer_pair_filter = eastl::make_unique<object_layer_filter>();
    	m_contact_listener = eastl::make_unique<contact_listener>();
    	m_physics_system.Init(
            sv_max_rigid_bodies(),
            sv_num_mutexes(),
            sv_max_body_pairs(),
            sv_max_contacts(),
    		*m_broad_phase,
    		*m_broad_phase_filter,
    		*m_object_layer_pair_filter
    	);
    	m_physics_system.SetGravity(JPH::Vec3{0.0f, -9.81f, 0.0f}); // set gravity to earth's gravity
    	m_physics_system.SetContactListener(&*m_contact_listener);
    	m_debug_renderer = eastl::make_unique<debug_renderer>();
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
			settings->mLayer = layers::k_dynamic;
			settings->mShape = capsule;
			settings->mFriction = 0.5f;
			settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -cCharacterRadiusStanding); // Accept contacts that touch the lower sphere of the capsule
			JPH::Ref<JPH::Character> character = new JPH::Character(settings, {}, JPH::Quat::sIdentity(), 0, &m_physics_system);
			character->AddToPhysicsSystem(JPH::EActivation::Activate);
    		if (cc.phys_character) {
    			cc.phys_character->RemoveFromPhysicsSystem();
    			cc.phys_character = nullptr;
    		}
			cc.phys_character = character;
		});

    	scene.observer<com::character_controller>().event(flecs::OnRemove).each([&](com::character_controller& cc) {
    		if (!cc.phys_character) return;
			cc.phys_character->RemoveFromPhysicsSystem();
    		cc.phys_character = nullptr;
		});

		log_info("Creating static colliders...");

    	auto filter = scene.query<const com::transform, const com::mesh_renderer>();
		eastl::vector<eastl::pair<eastl::span<const com::transform>, eastl::span<const com::mesh_renderer>>> targets {};
    	std::size_t total = 0;
        for (auto old_body : m_static_bodies) {
            bi.RemoveBody(old_body);
        }
        m_static_bodies.clear();
    	filter.each([&](const com::transform& transform, const com::mesh_renderer& renderer) {
            if (!renderer.meshes.empty()) {
                JPH::BodyCreationSettings ci {};
                create_static_body(ci, transform, renderer);
                m_static_bodies.emplace_back(bi.CreateAndAddBody(ci, JPH::EActivation::DontActivate));
            }
		});
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
    	auto& active = scene_mgr::active();
    	JPH::BodyInterface& bi = m_physics_system.GetBodyInterface();

    	// sync loop 1 rigidbody => transform
    	active.query<const com::rigidbody, com::transform>().each([&](const com::rigidbody& rb, com::transform& transform) {
			const JPH::BodyID body_id = rb.phys_body;
			transform.position = eastl::bit_cast<XMFLOAT4>(bi.GetPosition(body_id));
			transform.rotation = eastl::bit_cast<XMFLOAT4>(bi.GetRotation(body_id));
		});

    	// sync loop 2 character controller => transform
    	active.query<const com::character_controller, com::transform>().each([&](const com::character_controller& cc, com::transform& transform) {
			cc.phys_character->PostSimulation(0.05f);
    		transform.position = eastl::bit_cast<XMFLOAT4>(cc.phys_character->GetPosition());
			transform.rotation = eastl::bit_cast<XMFLOAT4>(cc.phys_character->GetPosition());
		});
    }

    auto physics_subsystem::create_static_body(JPH::BodyCreationSettings& ci, const com::transform& transform, const com::mesh_renderer& renderer) -> void {
        if (renderer.meshes.empty()) {
            log_warn("Mesh renderer has no meshes");
            return;
        }
        const auto* const mesh = renderer.meshes.front();
        if (!mesh->get_collision_mesh()) {
            log_warn("Mesh has no collision mesh");
            return;
        }
        JPH::Shape::ShapeResult result {};
        JPH::Vec3 pos;
        static_assert(sizeof(pos) == sizeof(XMFLOAT3A));
        XMStoreFloat3(reinterpret_cast<XMFLOAT3A*>(&pos), XMLoadFloat4(&transform.position));
        JPH::Quat rot;
        static_assert(sizeof(rot) == sizeof(XMFLOAT4));
        XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&rot), XMQuaternionNormalize(XMLoadFloat4(&transform.rotation)));
        XMFLOAT3A scale {};
        XMStoreFloat3A(&scale, XMLoadFloat4(&transform.scale));
        ci = {
            new JPH::ScaledShape{mesh->get_collision_mesh()->data(), eastl::bit_cast<JPH::Vec3>(scale)},
            pos,
            rot,
            JPH::EMotionType::Static,
            layers::k_static
        };
    }
}
