-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef [[
    double __lu_get_delta_time(void);
]]

time = {
    delta_time = 0.0, -- in seconds
    time = 0.0, -- in seconds
    frame_time = 0.0, -- in milliseconds
    time_scale = 1.0, -- 1.0 = realtime, 0.0 = paused
    frame = 0, -- frame counter
    fps = 0.0, -- frames per second
    fps_min = 10.0^5, -- minimum frames per second
    fps_max = -10.0^5, -- maximum frames per second
    fps_avg = 0.0, -- average frames per second over N samples
    fps_avg_min = 10.0^5, -- minimum avg frames per second
    fps_avg_max = -10.0^5, -- maximum avg frames per second
    fps_histogram = {}, -- frames per second histogram
    fps_histogram_samples = 256
}

local prev = 0.0
local idx = 1 -- ring buffer index

function time._on_tick_()
    time.delta_time = ffi.C.__lu_get_delta_time()
    time.time = time.time + time.delta_time
    time.frame_time = time.delta_time * 1000.0
    time.frame = time.frame + 1
    time.fps = 1000.0 / time.frame_time
    time.fps_min = math.min(time.fps_min, time.fps)
    time.fps_max = math.max(time.fps_max, time.fps)
    time.fps_avg_min = math.min(time.fps_avg_min, time.fps_avg)
    time.fps_avg_max = math.max(time.fps_avg_max, time.fps_avg)

    -- Update circular buffer with the current fps
    time.fps_histogram[idx] = time.fps

    -- Calculate the average fps over N samples
    local sum_fps = 0.0
    for i = 1, time.fps_histogram_samples do
        if time.fps_histogram[i] then
            sum_fps = sum_fps + time.fps_histogram[i]
        end
    end
    time.fps_avg = sum_fps / time.fps_histogram_samples
    idx = idx + 1
    idx = ((idx - 1) % time.fps_histogram_samples) + 1
end

return time
