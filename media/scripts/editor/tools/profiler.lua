-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'
local gui = require 'editor.imgui'

local profiler = {
    name = ICONS.CLOCK..' Profiler',
    isVisible = ffi.new('bool[1]', true),
    isProfilerRunning = false,
}

local profileDataRoutines = {}
local MAX_TIME = 60.0 * 5.0 -- 5 minutes
local timeLimit = 30
local startTime = 0.0
local startTimePtr = ffi.new('double[1]', 0.0)
local fileProxy = {}

function fileProxy:write(str)
    assert(type(str) == 'string')
    table.insert(profileDataRoutines, str)
end
function fileProxy:close() end

function profiler:render()
    if gui.Begin(self.name, self.isVisible) then
        if gui.BeginTabBar('##profiler_tabs') then
            if gui.BeginTabItem(ICONS.ALARM_CLOCK..' General') then
                if gui.CollapsingHeader(ICONS.CHART_AREA..' Histogram', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
                    local plot_size = gui.ImVec2(WINDOW_SIZE.x, 200.0)
                    gui.PlotHistogram_FloatPtr('##frame_times', time.fpsHistogram, time.HISTOGRAM_SAMPLES, 0, nil, 0.0, time.fpsAvg * 2.0, plot_size)
                end
                if gui.CollapsingHeader(ICONS.INFO..' Stats', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
                    gui.Text(string.format('FPS: %d', time.fpsAvg))
                    gui.Text(string.format('FPS avg: %d', time.fpsAvg))
                    gui.Text(string.format('FPS min: %d', time.fpsMin))
                    gui.Text(string.format('FPS max: %d', time.fpsMax))
                    gui.Text(string.format('Time: %.3f s', time.time))
                    gui.Text(string.format('Delta time: %.3f s', time.deltaTime))
                    gui.Text(string.format('Frame time: %.3f ms', time.frameTime))
                    gui.Text(string.format('Frame: %d', time.frame))
                end
                gui.EndTabItem()
            end
            if gui.BeginTabItem(ICONS.CODE..' Scripting') then
                if self.isProfilerRunning then
                    startTime = startTime + time.deltaTime
                    if startTime >= timeLimit then
                        print('Stopped profiling')
                        profile.stop()
                        self.isProfilerRunning = false
                    end
                end
                local title = self.isProfilerRunning and ICONS.STOP_CIRCLE..' Stop' or ICONS.PLAY_CIRCLE..' Record'
                gui.PushStyleColor_U32(ffi.C.ImGuiCol_Button, self.isProfilerRunning and 0xff000088 or 0xff008800)
                if gui.Button(title) then
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
                gui.PopStyleColor()
                gui.SameLine()
                gui.Text(string.format('Recorded: %.1f s', startTime))
                gui.SameLine()
                if gui.RadioButton('30 s', timeLimit ==30) then timeLimit = 30 end
                gui.SameLine()
                if gui.RadioButton('1 m', timeLimit == 60) then timeLimit = 60 end
                gui.SameLine()
                if gui.RadioButton('5 m', timeLimit == 60*5) then timeLimit = 60*5 end
                gui.SameLine()
                if gui.RadioButton('10 m', timeLimit == 60*10) then timeLimit = 60*10 end
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

return profiler
