-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- components Module - Bundles all components for the entity-component-system.
--- @module components
------------------------------------------------------------------------------

--- components Module
local components = {
    transform = require 'components.transform', -- transform component
    camera = require 'components.camera', -- camera component
    mesh_renderer = require 'components.mesh_renderer', -- mesh renderer component
    character_controller = require 'components.character_controller', -- Character controller component
}

return components
