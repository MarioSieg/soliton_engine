-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'
local gui = require 'editor.imgui'

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

function fileProxy:write(str)
    assert(type(str) == 'string')
    table.insert(profileDataRoutines, str)
end
function fileProxy:close() end

function Profiler:render()
    if gui.Begin(Profiler.name, Profiler.isVisible) then
        if gui.BeginTabBar('##profiler_tabs') then
            if gui.BeginTabItem(ICONS.ALARM_CLOCK..' General') then
                if gui.CollapsingHeader(ICONS.CHART_AREA..' Histogram', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
                    local plot_size = gui.ImVec2(WINDOW_SIZE.x, 200.0)
                    gui.PlotHistogram_FloatPtr('##frame_times', Time.fpsHistogram, Time.HISTOGRAM_SAMPLES, 0, nil, 0.0, Time.fpsAvg * 2.0, plot_size)
                end
                if gui.CollapsingHeader(ICONS.INFO..' Stats', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
                    gui.Text(string.format('FPS: %d', Time.fpsAvg))
                    gui.Text(string.format('FPS avg: %d', Time.fpsAvg))
                    gui.Text(string.format('FPS min: %d', Time.fpsMin))
                    gui.Text(string.format('FPS max: %d', Time.fpsMax))
                    gui.Text(string.format('Time: %.3f s', Time.time))
                    gui.Text(string.format('Delta time: %.3f s', Time.deltaTime))
                    gui.Text(string.format('Frame time: %.3f ms', Time.frameTime))
                    gui.Text(string.format('Frame: %d', Time.frame))
                end
                gui.EndTabItem()
            end
            if gui.BeginTabItem(ICONS.CODE..' Scripting') then
                if Profiler.isProfilerRunning then
                    startTime = startTime + Time.deltaTime
                    if startTime >= timeLimit then
                        print('Stopped profiling')
                        profile.stop()
                        Profiler.isProfilerRunning = false
                    end
                end
                local title = Profiler.isProfilerRunning and ICONS.STOP_CIRCLE..' Stop' or ICONS.PLAY_CIRCLE..' Record'
                gui.PushStyleColor_U32(ffi.C.ImGuiCol_Button, Profiler.isProfilerRunning and 0xff000088 or 0xff008800)
                if gui.Button(title) then
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
                gui.PopStyleColor()
                gui.SameLine()
                gui.Text(string.format(ICONS.STOPWATCH..' %d s', startTime))
                gui.SameLine()
                gui.PushItemWidth(MAX_WIDTH)
                if gui.BeginCombo('##profiler_time_limit', TIMINGS[timeLimit] or '?', ffi.C.ImGuiComboFlags_HeightSmall) then
                    if gui.Selectable('15 Seconds', timeLimit == 15) then timeLimit = 15 end
                    if gui.Selectable('30 Seconds', timeLimit == 30) then timeLimit = 30 end
                    if gui.Selectable('1 Minute', timeLimit == 60) then timeLimit = 60 end
                    if gui.Selectable('5 Minute', timeLimit == 60*5) then timeLimit = 60*5 end
                    if gui.Selectable('10 Minute', timeLimit == 60*10) then timeLimit = 60*10 end
                    if gui.Selectable('1 Hour', timeLimit == 60^2) then timeLimit = 60^2 end
                    gui.EndCombo()
                end
                gui.PopItemWidth()
                gui.SameLine()
                gui.ProgressBar(startTime / timeLimit)
                gui.Separator()
                if #profileDataRoutines == 0 then
                    gui.TextUnformatted('No data recorded')
                else
                    if gui.BeginChild('##profiler_callstack') then
                        for _, routine in ipairs(profileDataRoutines) do
                            gui.TextUnformatted(routine)
                            gui.Separator()
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

return Profiler
