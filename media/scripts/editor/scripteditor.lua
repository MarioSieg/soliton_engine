-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local gui = require 'editor.imgui'

ffi.cdef [[
    void __lu_script_editor_render(const char* title);
    void __lu_script_editor_set_text(const char* text);
    const char* __lu_script_editor_get_text(void);
    size_t __lu_script_editor_get_text_len(void);
    void __lu_script_editor_redo(void);
    void __lu_script_editor_undo(void);
    void __lu_script_editor_set_read_only(bool read_only);
    bool __lu_script_editor_has_text_changed(void);
]]

local native_editor = {}

function native_editor.render(title)
    ffi.C.__lu_script_editor_render(title)
end

function native_editor.setText(text)
    ffi.C.__lu_script_editor_set_text(text)
end

function native_editor.getText()
    local len = ffi.C.__lu_script_editor_get_text_len()
    local str = ffi.string(ffi.C.__lu_script_editor_get_text(), len)
    assert(str ~= nil)
    assert(type(str) == 'string')
    assert(#str == len)
    return str
end

function native_editor.redo()
    ffi.C.__lu_script_editor_redo()
end

function native_editor.undo()
    ffi.C.__lu_script_editor_undo()
end

function native_editor.setReadOnly(read_only)
    assert(type(read_only) == 'boolean')
    ffi.C.__lu_script_editor_set_read_only(read_only)
end

function native_editor.hasTextChanged()
    return ffi.C.__lu_script_editor_has_text_changed()
end

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

    local script = {
        name = name,
        textBuf = ffi.new('char[?]', #str+1),
    }

    ffi.copy(script.textBuf, str)

    function script:syncToEditor()
        native_editor.setText(self.textBuf)
    end
    
    function script:syncFromEditor()
        local text = native_editor.getText()
        self.textBuf = ffi.new('char[?]', #text+1)
        ffi.copy(self.textBuf, text)
    end
    
    function script:exec()
        self:syncFromEditor()
        local chunk, loadErr = loadstring(ffi.string(self.textBuf))
        if chunk then
            local ok, err = pcall(chunk)
            if not ok then
                print('Error when executing script: '..self.name)
                print(err)
            end
        else
            print('Error when loading script: '..self.name)
            print(loadErr)
        end
    end

    return script
end

local m = {
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

function m:render()
    gui.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if gui.Begin(m.name, m.isVisible, ffi.C.ImGuiWindowFlags_MenuBar) then
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
                local script = m.scripts[m.activeScriptName]
                assert(script)
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
            for name, script in pairs(m.scripts) do
                if gui.BeginTabItem(name) then
                    native_editor.render(name)
                    gui.EndTabItem()
                end
            end
            gui.EndTabBar()
        end
    end
    gui.End()
end

return m
