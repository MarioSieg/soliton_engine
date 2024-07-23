-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'

local ui = require 'imgui.imgui'
local icons = require 'imgui.icons'
local time = require('time')

local profiler = {
    name = icons.i_clock .. ' Profiler',
    is_visible = ffi.new('bool[1]', true),
    is_profiler_running = false,
}

local profiled_callstack = {}
local timings = {
    [15]    = '15 Seconds',
    [30]    = '30 Seconds',
    [60]    = '1 Minute',
    [60 * 5]  = '5 Minute',
    [60 * 10] = '10 Minute',
    [60 ^ 2]  = '1 Hour'
}
local width = #timings[30] * 12.0

local time_limit = 30
local start_time = 0.0
local file_proxy = {}
local fps_plot = {}
local fps_avg = 0.0

function file_proxy:write(str)
    table.insert(profiled_callstack, str)
end
function file_proxy:close() end

function profiler:render()
    if ui.Begin(self.name, self.is_visible) then
        if ui.BeginTabBar('##profiler_tabs') then
            if ui.BeginTabItem(icons.i_bezier_curve .. ' Histogram') then
                local plot_size = ui.ImVec2(default_window_size.x, 200.0)
                ui.PlotHistogram_FloatPtr(
                    '##frame_times',
                    time.fps_histogram,
                    time.samples,
                    0,
                    nil,
                    0.0,
                    time.fps_avg * 1.5,
                    plot_size
                )
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
                if self.is_profiler_running then
                    table.insert(fps_plot, time.fps)
                    start_time = start_time + time.delta_time
                    if start_time >= time_limit then
                        print('Stopped profiling')
                        local sum = 0.0
                        for i = 1, #fps_plot do
                            sum = sum + fps_plot[i]
                        end
                        fps_avg = sum / #fps_plot
                        fps_plot = {}
                        profile.stop()
                        self.is_profiler_running = false
                    end
                end
                local title = self.is_profiler_running and icons.i_stop_circle .. ' Stop' or icons.i_play_circle .. ' Record'
                ui.PushStyleColor_U32(ffi.C.ImGuiCol_Button, self.is_profiler_running and 0xff000088 or 0xff008800)
                if ui.Button(title) then
                    if self.is_profiler_running then
                        print('Stopped profiling')
                        profile.stop()
                    else
                        profile.start('G', file_proxy)
                        start_time = 0.0
                        profiled_callstack = {}
                        print('Started profiling')
                    end
                    self.is_profiler_running = not self.is_profiler_running
                end
                ui.PopStyleColor()
                ui.SameLine()
                ui.Text(string.format(icons.i_stopwatch .. ' %d s', start_time))
                ui.SameLine()
                ui.PushItemWidth(width)
                if ui.BeginCombo('##profiler_time_limit', timings[time_limit] or '?', ffi.C.ImGuiComboFlags_HeightSmall) then
                    if ui.Selectable('15 Seconds', time_limit == 15) then time_limit = 15 end
                    if ui.Selectable('30 Seconds', time_limit == 30) then time_limit = 30 end
                    if ui.Selectable('1 Minute', time_limit == 60) then time_limit = 60 end
                    if ui.Selectable('5 Minute', time_limit == 60*5) then time_limit = 60*5 end
                    if ui.Selectable('10 Minute', time_limit == 60*10) then time_limit = 60*10 end
                    if ui.Selectable('1 Hour', time_limit == 60^2) then time_limit = 60^2 end
                    ui.EndCombo()
                end
                ui.PopItemWidth()
                ui.SameLine()
                ui.ProgressBar(start_time / time_limit)
                ui.Separator()
                if #profiled_callstack == 0 then
                    ui.TextUnformatted('No data recorded')
                else
                    ui.Text(string.format('AVG: %.03fHz', fps_avg))
                    ui.Separator()
                    if ui.BeginChild('##profiler_callstack') then
                        for i=1, #profiled_callstack do
                            ui.TextUnformatted(profiled_callstack[i])
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
