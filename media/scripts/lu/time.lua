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

function time:_on_tick_()
    self.delta_time = ffi.C.__lu_get_delta_time()
    self.time = self.time + self.delta_time
    self.frame_time = self.delta_time * 1000.0
    self.frame = self.frame + 1
    self.fps = 1000.0 / self.frame_time
    self.fps_min = math.min(self.fps_min, self.fps)
    self.fps_max = math.max(self.fps_max, self.fps)
    self.fps_avg_min = math.min(self.fps_avg_min, self.fps_avg)
    self.fps_avg_max = math.max(self.fps_avg_max, self.fps_avg)

    -- Update circular buffer with the current fps
    self.fps_histogram[idx] = self.fps

    -- Calculate the average fps over N samples
    local sum_fps = 0.0
    for i = 1, self.fps_histogram_samples do
        if self.fps_histogram[i] then
            sum_fps = sum_fps + self.fps_histogram[i]
        end
    end
    self.fps_avg = sum_fps / self.fps_histogram_samples
    idx = idx + 1
    idx = ((idx - 1) % self.fps_histogram_samples) + 1
end

return time
