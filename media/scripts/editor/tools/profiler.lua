-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'
local gui = require 'editor.imgui'

local m = {
    name = ICONS.CLOCK..' Profiler',
    isVisible = ffi.new('bool[1]', true),
}

local isProfilerRunning = false
local profileData = {}
local fileProxy = {}

function fileProxy:write(str)
    assert(type(str) == 'string')
    table.insert(profileData, str)
end
function fileProxy:close() end

function m:render()
    if gui.Begin(m.name, m.isVisible) then
        if gui.BeginTabBar('##profiler_tabs') then
            if gui.BeginTabItem(ICONS.ALARM_CLOCK..' General') then
                if gui.CollapsingHeader(ICONS.CHART_AREA..' Histogram', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
                    local plot_size = gui.ImVec2(WINDOW_SIZE.x, 200.0)
                    gui.PlotHistogram_FloatPtr('Frame times', time.fpsHistogram, time.HISTOGRAM_SAMPLES, 0, nil, 0.0, time.fpsAvg * 2.0, plot_size)
                end
                if gui.CollapsingHeader(ICONS.INFO..' Stats', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
                    gui.Text(string.format('FPS: %d', time.fpsAvg))
                    gui.Text(string.format('FPS avg: %d', time.fpsAvg))
                    gui.Text(string.format('FPS min: %d', time.fpsMin))
                    gui.Text(string.format('FPS max: %d', time.fpsMax))
                    gui.Text(string.format('Time: %.3f s', time.time))
                    gui.Text(string.format('Delta time: %.3f s', time.deltaTime))
                    gui.Text(string.format('Frame time: %.3f ms', time.frameTime))
                end
                gui.EndTabItem()
            end
            if gui.BeginTabItem(ICONS.CODE..' Scripting') then
                local title = isProfilerRunning and ICONS.STOP_CIRCLE or ICONS.PLAY_CIRCLE
                if gui.Button(title) then
                    if isProfilerRunning then
                        print('Stopped profiling')
                        profile.stop()
                    else
                        profile.start('Fva', fileProxy)
                        print('Started profiling')
                    end
                    isProfilerRunning = not isProfilerRunning
                end
                if profileData then
                    if gui.BeginChild('##profiler_callstack') then
                        if gui.CollapsingHeader('Callstack', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
                            for _, code in ipairs(profileData) do
                                gui.TextUnformatted(code)
                                gui.Separator()
                            end
                        end
                    end
                    gui.EndChild()
                end
                gui.EndTabItem()
            end
            gui.EndTabBar()
        end
    end
    gui.End()
end

return m
