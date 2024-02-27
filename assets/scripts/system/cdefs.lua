-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- Common CDEFS used by the private and public API

local ffi = require 'ffi'

ffi.cdef[[
    typedef uint64_t lua_entity_id;

    typedef struct {
        double x;
        double y;
    } lua_vec2;

    typedef struct {
        double x;
        double y;
        double z;
    } lua_vec3;

    typedef struct {
        double x;
        double y;
        double z;
        double w;
    } lua_vec4;
]]
