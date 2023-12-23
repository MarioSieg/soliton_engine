-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef [[
    void __lu_panic(const char* msg);
]]

local m = {}

function m.panic(msg)
    ffi.C.__lu_panic(msg)
end

return m
