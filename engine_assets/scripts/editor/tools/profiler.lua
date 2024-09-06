-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'
local utils = require 'editor.utils'
local app = require 'app'
local ui = require 'imgui.imgui'
local icons = require 'imgui.icons'
local time = require 'time'

local max_subsystem_count = 16
local plot_samples = 128
local per_subsystem_sample_idx = 0
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
local fps_plot = {}
local fps_avg = 0.0

local profiler = {
    name = icons.i_clock .. ' Profiler',
    is_visible = ffi.new('bool[1]', true),
    is_profiler_running = false,
    _subsystem_count = 0,
    _subsystem_names = ffi.new('const char*[?]', max_subsystem_count),
    _pre_tick_times = ffi.new('double[?]', max_subsystem_count),
    _tick_times = ffi.new('double[?]', max_subsystem_count),
    _post_tick_times = ffi.new('double[?]', max_subsystem_count),
    _total_tick_time_per_subsystem = {},
}

profiler._subsystem_count = app._get_subsystem_names(profiler._subsystem_names, max_subsystem_count)
for i = 0, profiler._subsystem_count - 1 do
    profiler._total_tick_time_per_subsystem[ffi.string(profiler._subsystem_names[i])] = ffi.new('double[?]', plot_samples)
end

function profiler:_render_histogram_tab()
    if ui.BeginTabItem(icons.i_bezier_curve .. ' Histogram') then
        local plot_size = ui.ImVec2(utils.default_window_size.x, 200.0)
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
end

function profiler:_render_general_tab()
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
end

function profiler:_render_subsystems_tab()
    per_subsystem_sample_idx = (per_subsystem_sample_idx + 1) % plot_samples
    if ui.BeginTabItem(icons.i_cogs .. ' Subsystems') then
        app._get_subsystem_pre_tick_times(self._pre_tick_times, max_subsystem_count)
        app._get_subsystem_tick_times(self._tick_times, max_subsystem_count)
        app._get_subsystem_post_tick_times(self._post_tick_times, max_subsystem_count)
        if ui.BeginTable('##subsystems_table', 6, ffi.C.ImGuiTableFlags_Borders) then
            ui.TableSetupColumn('Subsystem')
            ui.TableSetupColumn('Pre-Tick')
            ui.TableSetupColumn('Tick')
            ui.TableSetupColumn('Post-Tick')
            ui.TableSetupColumn('Total of Subsystem')
            ui.TableSetupColumn('Percent of All')
            ui.TableHeadersRow()
            local total_ms_all_subsystems = 0.0
            for i = 0, self._subsystem_count - 1 do
                local name = ffi.string(self._subsystem_names[i])
                local pre_tick_ms = self._pre_tick_times[i]
                local tick_ms = self._tick_times[i]
                local post_tick_ms = self._post_tick_times[i]
                local total_ms = pre_tick_ms + tick_ms + post_tick_ms
                total_ms_all_subsystems = total_ms_all_subsystems + total_ms
                self._total_tick_time_per_subsystem[name][per_subsystem_sample_idx] = total_ms
            end
            for i = 0, self._subsystem_count - 1 do
                local name = ffi.string(self._subsystem_names[i])
                local pre_tick_ms = self._pre_tick_times[i]
                local tick_ms = self._tick_times[i]
                local post_tick_ms = self._post_tick_times[i]
                local total_ms = pre_tick_ms + tick_ms + post_tick_ms
                local percent_of_all = total_ms / total_ms_all_subsystems * 100.0
                ui.TableNextRow()
                ui.TableSetColumnIndex(0)
                ui.Text(name)
                ui.TableSetColumnIndex(1)
                ui.Text(string.format('%.3f ms', pre_tick_ms))
                ui.TableSetColumnIndex(2)
                ui.Text(string.format('%.3f ms', tick_ms))
                ui.TableSetColumnIndex(3)
                ui.Text(string.format('%.3f ms', post_tick_ms))
                ui.TableSetColumnIndex(4)
                ui.Text(string.format('%.3f ms', total_ms))
                ui.TableSetColumnIndex(5)
                ui.Text(string.format('%.2f%%', percent_of_all))
            end
        end
        ui.EndTable()
        ui.Separator()
        ui.Separator()
        if ui.ImPlot_BeginPlot("Subsystem Total Times") then
            ui.ImPlot_SetupAxes("Subsystems", "Total Time (ms)", ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_NoTickLabels)
            for i = 0, self._subsystem_count - 1 do
                local name = ffi.string(self._subsystem_names[i])
                ui.ImPlot_PlotLine(
                    name,
                    self._total_tick_time_per_subsystem[name],
                    plot_samples,
                    per_subsystem_sample_idx,
                    0
                )
            end
            ui.ImPlot_EndPlot()
        end
        ui.EndTabItem()
    end
end

function profiler:_render_scripting_tab()
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
            if ui.Selectable('5 Minute', time_limit == 60 * 5) then time_limit = 60 * 5 end
            if ui.Selectable('10 Minute', time_limit == 60 * 10) then time_limit = 60 * 10 end
            if ui.Selectable('1 Hour', time_limit == 60 ^ 2) then time_limit = 60 ^ 2 end
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
                for i = 1, #profiled_callstack do
                    ui.TextUnformatted(profiled_callstack[i])
                    ui.Separator()
                end
            end
            ui.EndChild()
        end
        ui.EndTabItem()
    end
end

function profiler:render()
    ui.SetNextWindowSize(utils.default_window_size, ffi.C.ImGuiCond_FirstUseEver)
    if ui.Begin(self.name, self.is_visible) then
        if ui.BeginTabBar('##profiler_tabs') then
            self:_render_histogram_tab()
            self:_render_general_tab()
            self:_render_subsystems_tab()
            self:_render_scripting_tab()
            ui.EndTabBar()
        end
    end
    ui.End()
end

return profiler
