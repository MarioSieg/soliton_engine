-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local gui = require 'imgui.gui'

local M = {
    name = 'Profiler',
    isVisible = ffi.new('bool[1]', true),
    frameTimes = ffi.new('float[?]', Time.fpsHistogramSamples),
}

function M:render()
    if gui.Begin(M.name, M.isVisible) then
        for i = 1, Time.fpsHistogramSamples do -- copy fps histogram to frame times
            M.frameTimes[i-1] = Time.fpsHistogram[i] or 0.0
        end
        local plot_size = gui.ImVec2(WINDOW_SIZE.x, 200.0)
        gui.PlotHistogram_FloatPtr('Frame times', M.frameTimes, Time.fpsHistogramSamples, 0, nil, 0.0, Time.fpsAvg * 2.0, plot_size)
        gui.Text(string.format('FPS: %d', Time.fpsAvg))
        gui.Text(string.format('FPS avg: %d', Time.fpsAvg))
        gui.Text(string.format('FPS min: %d', Time.fpsMin))
        gui.Text(string.format('FPS max: %d', Time.fpsMax))
        gui.Text(string.format('Time: %.3f s', Time.time))
        gui.Text(string.format('Delta time: %.3f s', Time.deltaTime))
        gui.Text(string.format('Frame time: %.3f ms', Time.frameTime))
    end
    gui.End()
end

return M
