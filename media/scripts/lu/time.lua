-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef [[
    double __lu_get_delta_time(void);
]]

local m = {
    delta_time = 0.0, -- in seconds
    time = 0.0, -- in seconds
    frame_time = 0.0, -- in milliseconds
    time_scale = 1.0, -- 1.0 = realtime, 0.0 = paused
    frame = 0, -- frame counter
    fps = 0.0, -- frames per second
    avg_fps = 0.0, -- average frames per second over N samples
    fps_histogram = {}, -- frames per second histogram
}

local samples = 1024 
local prev = 0.0
local buffer_size = samples
local idx = 1

function m._tick_()
    m.delta_time = ffi.C.__lu_get_delta_time()
    m.time = m.time + m.delta_time
    m.frame_time = m.delta_time * 1000.0
    m.frame = m.frame + 1
    m.fps = 1000.0 / m.frame_time

    -- Update circular buffer with the current fps
    m.fps_histogram[idx] = m.fps

    -- Calculate the average fps over N samples
    local sum_fps = 0.0
    for i = 1, buffer_size do
        if m.fps_histogram[i] then
            sum_fps = sum_fps + m.fps_histogram[i]
        end
    end
    m.avg_fps = sum_fps / buffer_size
    idx = idx + 1
    idx = ((idx - 1) % buffer_size) + 1
end

return m
