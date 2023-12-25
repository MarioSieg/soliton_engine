-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
ffi.cdef [[
    void __lu_script_editor_render(const char* title);
    void __lu_script_editor_set_text(const char* text);
    const char* __lu_script_editor_get_text(void);
    size_t __lu_script_editor_get_text_len(void);
    void __lu_script_editor_redo(void);
    void __lu_script_editor_undo(void);
    void __lu_script_editor_set_read_only(bool read_only);
]]

local NativeScriptEditor = {}

function NativeScriptEditor.render(title)
    ffi.C.__lu_script_editor_render(title)
end

function NativeScriptEditor.setText(text)
    ffi.C.__lu_script_editor_set_text(text)
end

function NativeScriptEditor.getText()
    local len = ffi.C.__lu_script_editor_get_text_len()
    return ffi.string(ffi.C.__lu_script_editor_get_text(), len)
end

local gui = require 'imgui.gui'

local MAX_SCRIPT_SIZE = 1024 * 128 -- 128 KiB
local SCRIPT_TEMPLATE = [[
local M = {}

function M:__onStart() -- This function is called when the world is loaded.

end

function M:__onTick() -- This function is called every frame.

end

return M

]]

function newScript(str, name)
    assert(type(str) == 'string')
    assert(type(name) == 'string')
    assert(#str+1 < MAX_SCRIPT_SIZE)

    local script = {
        name = name,
        textBuf = ffi.new('char[?]', #str+1),
    }

    ffi.copy(script.textBuf, str)

    function script:syncToEditor()
        NativeScriptEditor.setText(self.textBuf)
    end
    
    function script:syncFromEditor()
        local text = NativeScriptEditor.getText()
        assert(#text < MAX_SCRIPT_SIZE)
        ffi.copy(self.textBuf, text)
    end
    
    function script:exec()
        local chunk, err = loadstring(ffi.string(self.textBuf))
        if chunk then
            local ok, err = chunk()
            if not ok then
                print(err)
                return false
            end
            return true
        else
            print(err)
            return false
        end
    end
    return script
end

local M = {
    name = 'Script Editor',
    isVisible = ffi.new('bool[1]', true),
    scripts = {
        ['New'] = (
            function()
                local script = newScript(SCRIPT_TEMPLATE, 'New')
                script:syncToEditor()
                -- test sync
                --assert(NativeScriptEditor.getText() == SCRIPT_TEMPLATE)
                --script:syncFromEditor()
                --assert(ffi.string(script.textBuf) == SCRIPT_TEMPLATE)
                return script
            end
        )(),
    },
    activeScriptName = 'New',
}

function M:render()
    gui.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if gui.Begin(self.name, self.isVisible, ffi.C.ImGuiWindowFlags_MenuBar) then
        if gui.BeginMenuBar() then
            if gui.BeginMenu('File') then
                gui.EndMenu()
            end
            if gui.BeginMenu('Edit') then
                gui.EndMenu()
            end
            if gui.BeginMenu('View') then
                gui.EndMenu()
            end
            if gui.BeginMenu('Help') then
                gui.EndMenu()
            end
            gui.Separator()
            if gui.Button('Run') then
                local script = self.scripts[self.activeScriptName]
                assert(script)
                print('Executing live script: '..script.name)
                script:syncFromEditor()
                script:exec()
            end
            if gui.Button('Run Selected') then
                
            end
            if gui.Button('Hot-Reload') then
                
            end
            gui.EndMenuBar()
        end
        gui.Separator()
        if gui.BeginTabBar('##ScriptEditorTabs') then
            for name, script in pairs(self.scripts) do
                if gui.BeginTabItem(name) then
                    NativeScriptEditor.render(name)
                    gui.EndTabItem()
                end
            end
            gui.EndTabBar()
        end
    end
    gui.End()
end

return M
