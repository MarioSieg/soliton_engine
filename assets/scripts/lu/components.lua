-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
-- components Module - Bundles all components for the entity-component-system.
-- @module components
------------------------------------------------------------------------------

--- components Module
local components = {
    transform = require 'lu.components.transform', -- transform component
    camera = require 'lu.components.camera', -- camera component
    character_controller = require 'lu.components.character_controller', -- Character controller component
}

return components
