-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

local ICONS = require 'editor.icons'
local UI = require 'editor.imgui'

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

local NativeEditor = {}

function NativeEditor.render(title)
    ffi.C.__lu_script_editor_render(title)
end

function NativeEditor.setText(text)
    ffi.C.__lu_script_editor_set_text(text)
end

function NativeEditor.getText()
    local len = ffi.C.__lu_script_editor_get_text_len()
    local str = ffi.string(ffi.C.__lu_script_editor_get_text(), len)
    assert(str ~= nil)
    assert(#str == len)
    return str
end

function NativeEditor.redo()
    ffi.C.__lu_script_editor_redo()
end

function NativeEditor.undo()
    ffi.C.__lu_script_editor_undo()
end

function NativeEditor.setReadOnly(readOnly)
    ffi.C.__lu_script_editor_set_read_only(readOnly)
end

function NativeEditor.hasTextChanged()
    return ffi.C.__lu_script_editor_has_text_changed()
end

function readAllText(file)
    local f = assert(io.open(file, "rb"))
    local content = f:read("*all")
    f:close()
    return content
end

local SCRIPT_TEMPLATE = readAllText('assets/scripts/editor/tools/script_template.lua')

function newScript(str, name)
    local script = {
        name = name,
        textBuf = ffi.new('char[?]', #str+1),
    }

    ffi.copy(script.textBuf, str)

    function script:syncToEditor()
        NativeEditor.setText(self.textBuf)
    end
    
    function script:syncFromEditor()
        local text = NativeEditor.getText()
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

local ScriptEditor = {
    name = ICONS.CODE..' Script Editor',
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

function ScriptEditor:render()
    UI.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if UI.Begin(ScriptEditor.name, ScriptEditor.isVisible, ffi.C.ImGuiWindowFlags_MenuBar) then
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
            UI.PushStyleColor_U32(ffi.C.ImGuiCol_Button, 0xff000088)
            if UI.Button(ICONS.PLAY_CIRCLE..' Restart') then
                local script = ScriptEditor.scripts[ScriptEditor.activeScriptName]
                assert(script)
                script:exec()
            end
            UI.PopStyleColor(1)
            UI.EndMenuBar()
        end
        UI.Separator()
        if UI.BeginTabBar('##ScriptEditorTabs') then
            for name, script in pairs(ScriptEditor.scripts) do
                if UI.BeginTabItem(name) then
                    NativeEditor.render(name)
                    UI.EndTabItem()
                end
            end
            UI.EndTabBar()
        end
    end
    UI.End()
end

return ScriptEditor