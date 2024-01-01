-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef [[
    double __lu_get_delta_time(void);
]]

time = {
    deltaTime = 0.0, -- in seconds
    time = 0.0, -- in seconds
    frameTime = 16.0, -- in milliseconds
    timeScale = 1.0, -- 1.0 = realtime, 0.0 = paused
    frame = 0, -- frame counter
    fps = 0.0, -- frames per second
    fpsMin = 10.0^5, -- minimum frames per second
    fpsMax = -10.0^5, -- maximum frames per second
    fpsAvg = 0.0, -- average frames per second over N samples
    fpsAvgMin = 10.0^5, -- minimum avg frames per second
    fpsAvgMax = -10.0^5, -- maximum avg frames per second
    fpsHistogram = {}, -- frames per second histogram
    fpsHistogramSamples = 256
}

local prev = 0.0
local idx = 1 -- ring buffer index

function time:__onTick()
    self.deltaTime = ffi.C.__lu_get_delta_time()
    self.time = self.time + self.deltaTime
    self.frameTime = self.deltaTime * 1000.0
    self.frame = self.frame + 1
    self.fps = 1000.0 / self.frameTime
    self.fpsMin = math.min(self.fpsMin, self.fps)
    self.fpsMax = math.max(self.fpsMax, self.fps)
    self.fpsAvgMin = math.min(self.fpsAvgMin, self.fpsAvg)
    self.fpsAvgMax = math.max(self.fpsAvgMax, self.fpsAvg)

    -- Update circular buffer with the current fps
    self.fpsHistogram[idx] = self.fps

    -- Calculate the average fps over N samples
    local sum = 0.0
    for i = 1, self.fpsHistogramSamples do
        if self.fpsHistogram[i] then
            sum = sum + self.fpsHistogram[i]
        end
    end
    self.fpsAvg = sum / self.fpsHistogramSamples
    idx = idx + 1
    idx = ((idx - 1) % self.fpsHistogramSamples) + 1
end

return time
