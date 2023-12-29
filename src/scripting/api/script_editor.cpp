// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../api_prelude.hpp"
#include "../../graphics/imgui/text_editor.hpp"

[[nodiscard]] static auto getEditor() -> TextEditor& { // lazy init
    static const std::unique_ptr<TextEditor> g_editor = [] {
        auto editor = std::make_unique<TextEditor>();
        editor->SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        return editor;
    }();
    return *g_editor;
}
static std::string g_editor_text;

LUA_INTEROP_API auto __lu_script_editor_render(const char* title) -> void {
    title = title ? title : "Script Editor";
    getEditor().Render(title, {}, true);
}

LUA_INTEROP_API auto __lu_script_editor_set_text(const char* text) -> void {
    text = text ? text : "?!";
    getEditor().SetText(text);
}

LUA_INTEROP_API auto __lu_script_editor_get_text() -> const char* {
    g_editor_text = getEditor().GetText();
    return g_editor_text.c_str();
}

LUA_INTEROP_API auto __lu_script_editor_get_text_len() -> std::size_t {
    g_editor_text = getEditor().GetText();
    return g_editor_text.size();
}

LUA_INTEROP_API auto __lu_script_editor_redo() -> void {
    if (getEditor().CanRedo()) {
        getEditor().Redo();
    }
}

LUA_INTEROP_API auto __lu_script_editor_undo() -> void {
    if (getEditor().CanUndo()) {
        getEditor().Undo();
    }
}

LUA_INTEROP_API auto __lu_script_editor_set_readonly(bool readonly) -> void {
    getEditor().SetReadOnly(readonly);
}

LUA_INTEROP_API auto __lu_script_editor_has_text_changed() -> bool {
    return getEditor().IsTextChanged();
}
