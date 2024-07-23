-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

local icons = require 'imgui.icons'
local UI = require 'imgui.imgui'

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
    assert(#str == len)
    return str
end

function native_editor.redo()
    ffi.C.__lu_script_editor_redo()
end

function native_editor.undo()
    ffi.C.__lu_script_editor_undo()
end

function native_editor.setReadOnly(readOnly)
    ffi.C.__lu_script_editor_set_read_only(readOnly)
end

function native_editor.hasTextChanged()
    return ffi.C.__lu_script_editor_has_text_changed()
end

local function read_all_text(file)
    local f = io.open(file, "rb")
    if not f then
        eprint('Failed to open file: '..file)
        return ''
    end
    local content = f:read("*all")
    f:close()
    return content
end

local new_script_template = read_all_text('templates/assets/script_template.lua')

local function create_script(str, name)
    local script = {
        name = name,
        nameWithExt = name..'.lua',
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

local script_editor = {
    name = icons.i_code .. ' Script Editor',
    is_visible = ffi.new('bool[1]', true),
    scripts = {
        ['New'] = (
            function()
                local script = create_script(new_script_template, 'New')
                script:syncToEditor()
                return script
            end
        )(),
    },
    activeScriptName = 'New',
}

function script_editor:render()
    UI.SetNextWindowSize(default_window_size, ffi.C.ImGuiCond_FirstUseEver)
    if UI.Begin(script_editor.name, script_editor.is_visible, ffi.C.ImGuiWindowFlags_MenuBar) then
        if UI.BeginMenuBar() then
            if UI.BeginMenu('File') then
                UI.EndMenu()
            end
            if UI.BeginMenu('Edit') then
                UI.EndMenu()
            end
            if UI.BeginMenu('View') then
                UI.EndMenu()
            end
            UI.Separator()
            if UI.Button(icons.i_play_circle .. ' Run') then
                local script = script_editor.scripts[script_editor.activeScriptName]
                assert(script)
                script:exec()
            end
            UI.EndMenuBar()
        end
        UI.Separator()
        if UI.BeginTabBar('##ScriptEditorTabs') then
            for name, script in pairs(script_editor.scripts) do
                if UI.BeginTabItem(script.nameWithExt) then
                    native_editor.render(name.nameWithExt)
                    UI.EndTabItem()
                end
            end
            UI.EndTabBar()
        end
    end
    UI.End()
end

return script_editor
