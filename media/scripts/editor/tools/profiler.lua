-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local gui = require 'editor.imgui'

local m = {
    name = ICONS.CLOCK..' Profiler',
    isVisible = ffi.new('bool[1]', true),
    frameTimes = ffi.new('float[?]', time.fpsHistogramSamples),
}

function m:render()
    if gui.Begin(m.name, m.isVisible) then
        for i = 1, time.fpsHistogramSamples do -- copy fps histogram to frame times
            m.frameTimes[i-1] = time.fpsHistogram[i] or 0.0
        end
        local plot_size = gui.ImVec2(WINDOW_SIZE.x, 200.0)
        gui.PlotHistogram_FloatPtr('Frame times', m.frameTimes, time.fpsHistogramSamples, 0, nil, 0.0, time.fpsAvg * 2.0, plot_size)
        gui.Text(string.format('FPS: %d', time.fpsAvg))
        gui.Text(string.format('FPS avg: %d', time.fpsAvg))
        gui.Text(string.format('FPS min: %d', time.fpsMin))
        gui.Text(string.format('FPS max: %d', time.fpsMax))
        gui.Text(string.format('Time: %.3f s', time.time))
        gui.Text(string.format('Delta time: %.3f s', time.deltaTime))
        gui.Text(string.format('Frame time: %.3f ms', time.frameTime))
    end
    gui.End()
end

return m
