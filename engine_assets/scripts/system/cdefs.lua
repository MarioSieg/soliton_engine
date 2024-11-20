-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- Common CDEFS used by the private and public API

local ffi = require 'ffi'

-- Define the CDEFS for the Lua API
-- Must match the C++ struct definitions
ffi.cdef [[
    void __lu_panic(const char* msg);
    uint32_t __lu_ffi_cookie(void);

    typedef uint64_t __entity_id;
    typedef uint32_t __asset_id;

    typedef struct {
        double x;
        double y;
    } __vec2;

    typedef struct {
        double x;
        double y;
        double z;
    } __vec3;

    typedef struct {
        double x;
        double y;
        double z;
        double w;
    } __vec4;
]]
