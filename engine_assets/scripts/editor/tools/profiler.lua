-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local bit = require 'bit'
local profile = require 'jit.p'
local utils = require 'editor.utils'
local app = require 'app'
local ui = require 'imgui.imgui'
local icons = require 'imgui.icons'
local time = require 'time'
local utility = require 'utility'

local max = math.max
local lshift = bit.lshift

local plot_samples = 1024
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
local subsys_count = 0
local subsys_ring_buffer_size = 32
local subsys_max_count = 16
local subsys_show_avg_times = ffi.new('bool[1]', true)
local subsys_show_us_times = ffi.new('bool[1]', false)
local subsys_names = ffi.new('const char*[?]', subsys_max_count)
local subsys_pretick_times = ffi.new('double[?]', subsys_max_count)
local subsys_tick_times = ffi.new('double[?]', subsys_max_count)
local subsys_post_tick_times = ffi.new('double[?]', subsys_max_count)
local subsys_pre_tick_times_avg = {}
local subsys_tick_times_avg = {}
local subsys_post_tick_times_avg = {}
local subsys_percent_avg = {}
local subsys_total_tick_times = {}
local subsys_all_systems_total = utility.ring_buffer:new(subsys_ring_buffer_size)
local autofit_flags = lshift(1, 5) + lshift(1, 3) -- ui.ImPlotFlags_NoBoxSelect | ui.ImPlotFlags_NoInput, these are not defined for some reason. TODO: fix

local profiler = {
    name = icons.i_clock .. ' Profiler',
    is_visible = ffi.new('bool[1]', true),
    is_profiler_running = false
}

subsys_count = app._get_subsystem_names(subsys_names, subsys_max_count)
for i = 0, subsys_count - 1 do
    local name = ffi.string(subsys_names[i])
    subsys_pre_tick_times_avg[name] = utility.ring_buffer:new(subsys_ring_buffer_size)
    subsys_tick_times_avg[name] = utility.ring_buffer:new(subsys_ring_buffer_size)
    subsys_post_tick_times_avg[name] = utility.ring_buffer:new(subsys_ring_buffer_size)
    subsys_percent_avg[name] = utility.ring_buffer:new(subsys_ring_buffer_size)
    subsys_total_tick_times[name] = ffi.new('double[?]', plot_samples)
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
        app._get_subsystem_pre_tick_times(subsys_pretick_times, subsys_max_count)
        app._get_subsystem_tick_times(subsys_tick_times, subsys_max_count)
        app._get_subsystem_post_tick_times(subsys_post_tick_times, subsys_max_count)
        ui.Checkbox(string.format('Avg. Timings (%d Samples)', subsys_ring_buffer_size), subsys_show_avg_times)
        ui.SameLine()
        ui.Checkbox('Use Microseconds', subsys_show_us_times)
        ui.SameLine()
        ui.Separator()
        ui.TextUnformatted(string.format('All Subsystems Avg. Total: %.03f ms', subsys_all_systems_total:avg()))
        local avg_times = subsys_show_avg_times[0]
        local max_time_needed = 0
        if ui.BeginTable('##subsystems_table', 6, ffi.C.ImGuiTableFlags_Borders) then
            ui.TableSetupColumn('Subsystem')
            ui.TableSetupColumn('Pre-Tick')
            ui.TableSetupColumn('Tick')
            ui.TableSetupColumn('Post-Tick')
            ui.TableSetupColumn('Subsystem Total Time')
            ui.TableSetupColumn('Frame Total Percent')
            ui.TableHeadersRow()
            local total_ms_all_subsystems = 0
            local unit = subsys_show_us_times[0] and 'us' or 'ms'
            local unit_div = subsys_show_us_times[0] and 1000 or 1
            for i = 0, subsys_count - 1 do
                local name = ffi.string(subsys_names[i])
                local pre_tick_ms = subsys_pretick_times[i]
                local tick_ms = subsys_tick_times[i]
                local post_tick_ms = subsys_post_tick_times[i]
                local total_ms = pre_tick_ms + tick_ms + post_tick_ms
                max_time_needed = max(max_time_needed, total_ms)
                subsys_pre_tick_times_avg[name]:push(pre_tick_ms)
                subsys_tick_times_avg[name]:push(tick_ms)
                subsys_post_tick_times_avg[name]:push(post_tick_ms)
                total_ms_all_subsystems = total_ms_all_subsystems + total_ms
                subsys_total_tick_times[name][per_subsystem_sample_idx] = total_ms
            end
            subsys_all_systems_total:push(total_ms_all_subsystems)
            for i = 0, subsys_count - 1 do
                local name = ffi.string(subsys_names[i])
                local pre_tick_ms = avg_times and subsys_pre_tick_times_avg[name]:avg() or subsys_pretick_times[i]
                local tick_ms = avg_times and subsys_tick_times_avg[name]:avg() or subsys_tick_times[i]
                local post_tick_ms = avg_times and subsys_post_tick_times_avg[name]:avg() or subsys_post_tick_times[i]
                local total_ms = pre_tick_ms + tick_ms + post_tick_ms
                local percent_of_all = subsys_total_tick_times[name][per_subsystem_sample_idx] / total_ms_all_subsystems * 100.0
                ui.TableNextRow()
                ui.TableSetColumnIndex(0)
                ui.TextUnformatted(name)
                ui.TableSetColumnIndex(1)
                ui.TextUnformatted(string.format('%.03f %s', pre_tick_ms * unit_div, unit))
                ui.TableSetColumnIndex(2)
                ui.TextUnformatted(string.format('%.03f %s', tick_ms * unit_div, unit))
                ui.TableSetColumnIndex(3)
                ui.TextUnformatted(string.format('%.03f %s', post_tick_ms * unit_div, unit))
                ui.TableSetColumnIndex(4)
                ui.TextUnformatted(string.format('%.03f %s', total_ms * unit_div, unit))
                ui.TableSetColumnIndex(5)
                ui.TextUnformatted(string.format('%.2f%%', percent_of_all))
            end
        end
        ui.EndTable()
        ui.Separator()
        ui.Separator()
        ui.ImPlot_SetNextAxesToFit()
        if ui.ImPlot_BeginPlot("Subsystem Total Times", nil, autofit_flags) then
            ui.ImPlot_SetupAxes("Subsystems", "Total Time (ms)", ui.ImPlotAxisFlags_NoTickLabels, ui.ImPlotAxisFlags_NoTickLabels)
            local indices = {}
            for i = 0, subsys_count - 1 do
                table.insert(indices, i)
            end
            table.sort(indices, function(a, b)
                return subsys_total_tick_times[ffi.string(subsys_names[a])][per_subsystem_sample_idx]
                    > subsys_total_tick_times[ffi.string(subsys_names[b])][per_subsystem_sample_idx]
            end)
            for i = 1, #indices do
                local name = ffi.string(subsys_names[indices[i]])
                ui.ImPlot_PlotBars(
                    name,
                    subsys_total_tick_times[name],
                    plot_samples
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
