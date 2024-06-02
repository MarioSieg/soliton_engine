-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'

local ui = require 'editor.imgui'
local icons = require 'editor.icons'
local time = require('time')

local profiler = {
    name = icons.i_clock .. ' Profiler',
    is_visible = ffi.new('bool[1]', true),
    isProfilerRunning = false,
}

local profileDataRoutines = {}
local MAX_TIME = 60 ^ 2 -- 1 hour
local TIMINGS = {
    [15]    = '15 Seconds',
    [30]    = '30 Seconds',
    [60]    = '1 Minute',
    [60 * 5]  = '5 Minute',
    [60 * 10] = '10 Minute',
    [60 ^ 2]  = '1 Hour'
}
local MAX_WIDTH = #TIMINGS[30] * 12.0

local timeLimit = 30
local startTime = 0.0
local fileProxy = {}
local fpsPlot = {}
local fpsAvg = 0.0

function fileProxy:write(str)
    table.insert(profileDataRoutines, str)
end
function fileProxy:close() end

function profiler:render()
    if ui.Begin(self.name, self.is_visible) then
        if ui.BeginTabBar('##profiler_tabs') then
            if ui.BeginTabItem(icons.i_bezier_curve .. ' Histogram') then
                local plot_size = ui.ImVec2(default_window_size.x, 200.0)
                ui.PlotHistogram_FloatPtr('##frame_times', time.fps_histogram, time.samples, 0, nil, 0.0, time.fps_avg * 1.5, plot_size)
                ui.EndTabItem()
            end
            if ui.BeginTabItem(icons.i_alarm_clock .. ' General') then
                ui.Text(string.format('FPS: %d', time.fps))
                ui.Text(string.format('FPS avg: %d', time.fps_avg))
                ui.Text(string.format('FPS min: %d', time.fps_min))
                ui.Text(string.format('FPS max: %d', time.fps_max))
                ui.Text(string.format('time: %.3f s', time.time))
                ui.Text(string.format('Delta time: %f s', time.delta_time))
                ui.Text(string.format('Frame time: %f ms', time.frame_time))
                ui.Text(string.format('Frame: %d', time.frame))
                ui.EndTabItem()
            end
            if ui.BeginTabItem(icons.i_code .. ' Scripting') then
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
                local title = self.isProfilerRunning and icons.i_stop_circle .. ' Stop' or icons.i_play_circle .. ' Record'
                ui.PushStyleColor_U32(ffi.C.ImGuiCol_Button, self.isProfilerRunning and 0xff000088 or 0xff008800)
                if ui.Button(title) then
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
                ui.PopStyleColor()
                ui.SameLine()
                ui.Text(string.format(icons.i_stopwatch .. ' %d s', startTime))
                ui.SameLine()
                ui.PushItemWidth(MAX_WIDTH)
                if ui.BeginCombo('##profiler_time_limit', TIMINGS[timeLimit] or '?', ffi.C.ImGuiComboFlags_HeightSmall) then
                    if ui.Selectable('15 Seconds', timeLimit == 15) then timeLimit = 15 end
                    if ui.Selectable('30 Seconds', timeLimit == 30) then timeLimit = 30 end
                    if ui.Selectable('1 Minute', timeLimit == 60) then timeLimit = 60 end
                    if ui.Selectable('5 Minute', timeLimit == 60*5) then timeLimit = 60*5 end
                    if ui.Selectable('10 Minute', timeLimit == 60*10) then timeLimit = 60*10 end
                    if ui.Selectable('1 Hour', timeLimit == 60^2) then timeLimit = 60^2 end
                    ui.EndCombo()
                end
                ui.PopItemWidth()
                ui.SameLine()
                ui.ProgressBar(startTime / timeLimit)
                ui.Separator()
                if #profileDataRoutines == 0 then
                    ui.TextUnformatted('No data recorded')
                else
                    ui.Text(string.format('AVG: %.03fHz', fpsAvg))
                    ui.Separator()
                    if ui.BeginChild('##profiler_callstack') then
                        for i=1, #profileDataRoutines do
                            ui.TextUnformatted(profileDataRoutines[i])
                            ui.Separator()
                        end
                    end
                    ui.EndChild()
                end
                ui.EndTabItem()
            end
            ui.EndTabBar()
        end
    end
    ui.End()
end

return profiler
