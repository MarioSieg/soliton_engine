// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "contact_listener.hpp"

namespace soliton::physics {
    auto contact_listener::OnContactAdded(
        const JPH::Body& body1,
        const JPH::Body& body2,
        const JPH::ContactManifold& manifold,
        JPH::ContactSettings& io
    ) -> void {
        io.mCombinedFriction = std::sqrt(body1.GetFriction() * body2.GetFriction());
        io.mCombinedRestitution = std::max(body1.GetRestitution(), body2.GetRestitution());
    }
    auto contact_listener::OnContactPersisted(
        const JPH::Body& body1,
        const JPH::Body& body2,
        const JPH::ContactManifold& manifold,
        JPH::ContactSettings& io
    ) -> void {
        // Meow
    }
}
