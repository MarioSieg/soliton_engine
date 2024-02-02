// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "physics_subsystem.hpp"
#include "../core/kernel.hpp"

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

    void physics_subsystem::on_start(scene& scene) {
    	m_physics_system.OptimizeBroadPhase();
    }

    void physics_subsystem::on_post_tick() {
    	const double delta = kernel::get().get_delta_time();
    	const int n_steps = 1;
    	m_physics_system.Update(delta, n_steps, &*m_temp_allocator, &*m_job_system);
    }
}
