-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
-- Components Module - Bundles all components for the entity-component-system.
-- @module Components
------------------------------------------------------------------------------

local Components = {
    Transform = require 'lu.components.transform', -- Transform component
    Camera = require 'lu.components.camera', -- Camera component
    CharacterController = require 'lu.components.character_controller', -- Character controller component
}

return Components
