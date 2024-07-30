// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"
#include "../../graphics/imgui/text_editor.hpp"

[[nodiscard]] static auto get_editor() -> TextEditor& { // lazy init
    static const eastl::unique_ptr<TextEditor> g_editor = [] {
        auto editor = eastl::make_unique<TextEditor>();
        editor->SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        return editor;
    }();
    return *g_editor;
}
static eastl::string g_editor_text;

LUA_INTEROP_API auto __lu_script_editor_render(const char* title) -> void {
    title = title ? title : "Script Editor";
    get_editor().Render(title, {}, true);
}

LUA_INTEROP_API auto __lu_script_editor_set_text(const char* text) -> void {
    text = text ? text : "?!";
    get_editor().SetText(text);
}

LUA_INTEROP_API auto __lu_script_editor_get_text() -> const char* {
    g_editor_text = get_editor().GetText().c_str();
    return g_editor_text.c_str();
}

LUA_INTEROP_API auto __lu_script_editor_get_text_len() -> std::size_t {
    g_editor_text = get_editor().GetText().c_str();
    return g_editor_text.size();
}

LUA_INTEROP_API auto __lu_script_editor_redo() -> void {
    if (get_editor().CanRedo()) {
        get_editor().Redo();
    }
}

LUA_INTEROP_API auto __lu_script_editor_undo() -> void {
    if (get_editor().CanUndo()) {
        get_editor().Undo();
    }
}

LUA_INTEROP_API auto __lu_script_editor_set_readonly(const bool readonly) -> void {
    get_editor().SetReadOnly(readonly);
}

LUA_INTEROP_API auto __lu_script_editor_has_text_changed() -> bool {
    return get_editor().IsTextChanged();
}
