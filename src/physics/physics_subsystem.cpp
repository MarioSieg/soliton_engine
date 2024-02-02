// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "physics_subsystem.hpp"

#include <mimalloc.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

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

	// An example contact listener
	class MyContactListener : public JPH::ContactListener {
	public:
		// See: ContactListener
		virtual JPH::ValidateResult	OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override {
			// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
			return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
		}
		virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {

		}
		virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {

		}
		virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override {

		}
	};

	// An example activation listener
	class MyBodyActivationListener : public JPH::BodyActivationListener {
	public:
		virtual void OnBodyActivated(const JPH::BodyID &inBodyID, std::uint64_t inBodyUserData) override {
		}
		virtual void OnBodyDeactivated(const JPH::BodyID &inBodyID, std::uint64_t inBodyUserData) override {
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
		m_contact_listener = std::make_unique<MyContactListener>();
		m_body_activation_listener = std::make_unique<MyBodyActivationListener>();
    }

    physics_subsystem::~physics_subsystem() {
    	m_body_activation_listener.reset();
    	m_contact_listener.reset();
    	m_broad_phase_filter.reset();
    	m_broad_phase.reset();
        m_job_system.reset();
        m_temp_allocator.reset();
        delete JPH::Factory::sInstance;
    }

    void physics_subsystem::on_start(scene& scene) {
        subsystem::on_start(scene);
    }

    void physics_subsystem::on_post_tick() {

    }
}
