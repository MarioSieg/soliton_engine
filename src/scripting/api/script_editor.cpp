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

LUA_INTEROP_API void __lu_script_editor_render(const char* title) {
    title = title ? title : "Script Editor";
    if (getEditor().GetTotalLines() > 0) {
        getEditor().Render(title);
    }
}

LUA_INTEROP_API void __lu_script_editor_set_text(const char* text) {
    text = text ? text : "?!";
    getEditor().SetText(text);
}

LUA_INTEROP_API const char* __lu_script_editor_get_text(void) {
    g_editor_text = getEditor().GetText();
    return g_editor_text.c_str();
}

LUA_INTEROP_API size_t __lu_script_editor_get_text_len(void) {
    g_editor_text = getEditor().GetText();
    return g_editor_text.size();
}

LUA_INTEROP_API void __lu_script_editor_redo() {
    if (getEditor().CanRedo()) {
        getEditor().Redo();
    }
}

LUA_INTEROP_API void __lu_script_editor_undo() {
    if (getEditor().CanUndo()) {
        getEditor().Undo();
    }
}

LUA_INTEROP_API void __lu_script_editor_set_readonly(bool readonly) {
    getEditor().SetReadOnly(readonly);
}
