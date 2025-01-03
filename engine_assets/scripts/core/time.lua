----------------------------------------------------------------------------
-- Soliton Engine time Module
--
-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.
------------------------------------------------------------------------------

local ffi = require 'ffi'
local cpp = ffi.C

ffi.cdef [[
    double __lu_get_delta_time(void);
    double __lu_get_hpc_clock_now_ns(void);
]]

local min, max = math.min, math.max

local samples = 256

local time = {
    delta_time = 0.0, -- in seconds
    time = 0.0, -- in seconds
    frame_time = 16.0, -- in milliseconds
    time_scale = 1.0, -- 1.0 = realtime, 0.0 = paused
    frame = 0, -- frame counter
    fps = 0.0, -- frames per second
    fps_min = math.huge, -- minimum frames per second
    fps_max = -math.huge, -- maximum frames per second
    fps_avg = 0.0, -- average frames per second over N samples
    fps_avg_min = math.huge, -- minimum avg frames per second
    fps_avg_max = -math.huge, -- maximum avg frames per second
    fps_histogram = ffi.new('float[?]', samples), -- frames per second histogram
    samples = samples
}

for i = 0, samples - 1 do time.fps_histogram[i] = 0.0 end

local offset = 0 -- ring buffer index

function time:_update()
    self.delta_time = cpp.__lu_get_delta_time()
    self.time = self.time + self.delta_time
    self.frame_time = self.delta_time * 1000.0
    self.frame = self.frame + 1
    self.fps = 1000.0 / self.frame_time
    self.fps_min = min(self.fps_min, self.fps)
    self.fps_max = max(self.fps_max, self.fps)

    -- Update circular buffer with the current fps
    self.fps_histogram[offset] = self.fps
    offset = (offset + 1) % samples

    -- Calculate the average fps over N samples
    local sum = 0.0
    for i = 0, samples - 1 do
        sum = sum + self.fps_histogram[i]
    end
    self.fps_avg = sum / samples
    self.fps_avg_min = min(self.fps_avg_min, self.fps_avg)
    self.fps_avg_max = max(self.fps_avg_max, self.fps_avg)
end

function time.hpc_clock_now()
    return cpp.__lu_get_hpc_clock_now_ns()
end

function time.hpc_clock_elapsed_ns(start)
    return time.hpc_clock_now() - start
end

function time.hpc_clock_elapsed_us(start)
    return time.hpc_clock_elapsed_ns(start) / 1000
end

function time.hpc_clock_elapsed_ms(start)
    return time.hpc_clock_elapsed_ns(start) / 1000000
end

function time.hpc_clock_elapsed_s(start)
    return time.hpc_clock_elapsed_ns(start) / 1000000000
end

return time
