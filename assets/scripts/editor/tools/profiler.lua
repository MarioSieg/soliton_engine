-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'

local UI = require 'editor.imgui'
local ICONS = require 'editor.icons'
local time = require('time')

local Profiler = {
    name = ICONS.CLOCK..' Profiler',
    is_visible = ffi.new('bool[1]', true),
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
    if UI.Begin(self.name, self.is_visible) then
        if UI.BeginTabBar('##profiler_tabs') then
            if UI.BeginTabItem(ICONS.BEZIER_CURVE..' Histogram') then
                local plot_size = UI.ImVec2(WINDOW_SIZE.x, 200.0)
                UI.PlotHistogram_FloatPtr('##frame_times', time.fps_histogram, time.samples, 0, nil, 0.0, time.fps_avg * 1.5, plot_size)
                UI.EndTabItem()
            end
            if UI.BeginTabItem(ICONS.ALARM_CLOCK..' General') then
                UI.Text(string.format('FPS: %d', time.fps))
                UI.Text(string.format('FPS avg: %d', time.fps_avg))
                UI.Text(string.format('FPS min: %d', time.fps_min))
                UI.Text(string.format('FPS max: %d', time.fps_max))
                UI.Text(string.format('time: %.3f s', time.time))
                UI.Text(string.format('Delta time: %f s', time.delta_time))
                UI.Text(string.format('Frame time: %f ms', time.frame_time))
                UI.Text(string.format('Frame: %d', time.frame))
                UI.EndTabItem()
            end
            if UI.BeginTabItem(ICONS.CODE..' Scripting') then
                if self.isProfilerRunning then
                    table.insert(fpsPlot, time.fps)
                    startTime = startTime + time.deltaTime
                    if startTime >= timeLimit then
                        print('Stopped profiling')
                        local sum = 0.0
                        for i=1, #fpsPlot do
                            sum = sum + fpsPlot[i]
                        end
                        fpsAvg = sum / #fpsPlot
                        fpsPlot = {}
                        profile.stop()
                        self.isProfilerRunning = false
                    end
                end
                local title = self.isProfilerRunning and ICONS.STOP_CIRCLE..' Stop' or ICONS.PLAY_CIRCLE..' Record'
                UI.PushStyleColor_U32(ffi.C.ImGuiCol_Button, self.isProfilerRunning and 0xff000088 or 0xff008800)
                if UI.Button(title) then
                    if self.isProfilerRunning then
                        print('Stopped profiling')
                        profile.stop()
                    else
                        profile.start('G', fileProxy)
                        startTime = 0.0
                        profileDataRoutines = {}
                        print('Started profiling')
                    end
                    self.isProfilerRunning = not self.isProfilerRunning
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
                        for i=1, #profileDataRoutines do
                            UI.TextUnformatted(profileDataRoutines[i])
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
