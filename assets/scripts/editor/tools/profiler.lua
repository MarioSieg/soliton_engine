-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'

local UI = require 'editor.imgui'
local ICONS = require 'editor.icons'
local Time = require('Time')

local Profiler = {
    name = ICONS.CLOCK..' Profiler',
    isVisible = ffi.new('bool[1]', true),
    isProfilerRunning = false,
}

local profileDataRoutines = {}
local MAX_TIME = 60^2 -- 1 hour
local TIMINGS = {
    [15]    = '15 Seconds',
    [30]    = '30 Seconds',
    [60]    = '1 Minute',
    [60*5]  = '5 Minute',
    [60*10] = '10 Minute',
    [60^2]  = '1 Hour'
}
local MAX_WIDTH = #TIMINGS[30] * 12.0

local timeLimit = 30
local startTime = 0.0
local startTimePtr = ffi.new('double[1]', 0.0)
local fileProxy = {}
local fpsPlot = {}
local fpsAvg = 0.0

function fileProxy:write(str)
    table.insert(profileDataRoutines, str)
end
function fileProxy:close() end

function Profiler:render()
    if UI.Begin(Profiler.name, Profiler.isVisible) then
        if UI.BeginTabBar('##profiler_tabs') then
            if UI.BeginTabItem(ICONS.ALARM_CLOCK..' General') then
                if UI.CollapsingHeader(ICONS.CHART_AREA..' Histogram', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
                    local plot_size = UI.ImVec2(WINDOW_SIZE.x, 200.0)
                    UI.PlotHistogram_FloatPtr('##frame_times', Time.fpsHistogram, Time.HISTOGRAM_SAMPLES, 0, nil, 0.0, Time.fpsAvg * 2.0, plot_size)
                end
                if UI.CollapsingHeader(ICONS.INFO..' Stats', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
                    UI.Text(string.format('FPS: %d', Time.fpsAvg))
                    UI.Text(string.format('FPS avg: %d', Time.fpsAvg))
                    UI.Text(string.format('FPS min: %d', Time.fpsMin))
                    UI.Text(string.format('FPS max: %d', Time.fpsMax))
                    UI.Text(string.format('Time: %.3f s', Time.time))
                    UI.Text(string.format('Delta time: %f s', Time.deltaTime))
                    UI.Text(string.format('Frame time: %f ms', Time.frameTime))
                    UI.Text(string.format('Frame: %d', Time.frame))
                end
                UI.EndTabItem()
            end
            if UI.BeginTabItem(ICONS.CODE..' Scripting') then
                if Profiler.isProfilerRunning then
                    table.insert(fpsPlot, Time.fps)
                    startTime = startTime + Time.deltaTime
                    if startTime >= timeLimit then
                        print('Stopped profiling')
                        local sum = 0.0
                        for i=1, #fpsPlot do
                            sum = sum + fpsPlot[i]
                        end
                        fpsAvg = sum / #fpsPlot
                        fpsPlot = {}
                        profile.stop()
                        Profiler.isProfilerRunning = false
                    end
                end
                local title = Profiler.isProfilerRunning and ICONS.STOP_CIRCLE..' Stop' or ICONS.PLAY_CIRCLE..' Record'
                UI.PushStyleColor_U32(ffi.C.ImGuiCol_Button, Profiler.isProfilerRunning and 0xff000088 or 0xff008800)
                if UI.Button(title) then
                    if Profiler.isProfilerRunning then
                        print('Stopped profiling')
                        profile.stop()
                    else
                        profile.start('G', fileProxy)
                        startTime = 0.0
                        profileDataRoutines = {}
                        print('Started profiling')
                    end
                    Profiler.isProfilerRunning = not Profiler.isProfilerRunning
                end
                UI.PopStyleColor()
                UI.SameLine()
                UI.Text(string.format(ICONS.STOPWATCH..' %d s', startTime))
                UI.SameLine()
                UI.PushItemWidth(MAX_WIDTH)
                if UI.BeginCombo('##profiler_time_limit', TIMINGS[timeLimit] or '?', ffi.C.ImGuiComboFlags_HeightSmall) then
                    if UI.Selectable('15 Seconds', timeLimit == 15) then timeLimit = 15 end
                    if UI.Selectable('30 Seconds', timeLimit == 30) then timeLimit = 30 end
                    if UI.Selectable('1 Minute', timeLimit == 60) then timeLimit = 60 end
                    if UI.Selectable('5 Minute', timeLimit == 60*5) then timeLimit = 60*5 end
                    if UI.Selectable('10 Minute', timeLimit == 60*10) then timeLimit = 60*10 end
                    if UI.Selectable('1 Hour', timeLimit == 60^2) then timeLimit = 60^2 end
                    UI.EndCombo()
                end
                UI.PopItemWidth()
                UI.SameLine()
                UI.ProgressBar(startTime / timeLimit)
                UI.Separator()
                if #profileDataRoutines == 0 then
                    UI.TextUnformatted('No data recorded')
                else
                    UI.Text(string.format('AVG: %.03fHz', fpsAvg))
                    UI.Separator()
                    if UI.BeginChild('##profiler_callstack') then
                        for _, routine in ipairs(profileDataRoutines) do
                            UI.TextUnformatted(routine)
                            UI.Separator()
                        end
                    end
                    UI.EndChild()
                end
                UI.EndTabItem()
            end
            UI.EndTabBar()
        end
    end
    UI.End()
end

return Profiler
