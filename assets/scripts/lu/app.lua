-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local jit = require 'jit'
local ffi = require 'ffi'

ffi.cdef [[
    bool __lu_app_is_focused(void);
    bool __lu_app_is_ui_hovered(void);
    void __lu_window_maximize(void);
    void __lu_window_minimize(void);
    void __lu_window_enter_fullscreen(void);
    void __lu_window_leave_fullscreen(void);
    void __lu_window_set_title(const char* title);
    void __lu_window_set_size(int width, int height);
    void __lu_window_set_pos(int x, int y);
    void __lu_window_show(void);
    void __lu_window_hide(void);
    void __lu_window_allow_resize(bool allow);
    lua_vec2 __lu_window_get_size(void);
    lua_vec2 __lu_window_get_framebuf_size(void);
    lua_vec2 __lu_window_get_pos(void);
    void __lu_window_enable_cursor(bool enable);
    void __lu_app_exit(void);
    const char* __lu_app_host_get_cpu_name(void);
    const char* __lu_app_host_get_gpu_name(void);
    const char* __lu_app_host_get_gapi_name(void);
    const char* __lu_app_open_file_dialog(const char *file_type, const char* filters, const char* default_path);
    const char* __lu_app_open_folder_dialog(const char* default_path);
    uint32_t __lu_app_host_get_num_cpus(void);
]]

local C = ffi.C

local App = {
    name = 'Untitled App',
    appVersion = '0.1',
    engineVersion = '0.1',
    author = 'Anonymous',
    copyright = '',
    website = '',
    company = '',
    type = 'Game',
    Window = {
        isMaximized = false,
        isFullscreen = false,
        isVisible = true,
    },
    Host = {
        CPU_NAME = ffi.string(C.__lu_app_host_get_cpu_name()),
        GPU_NAME = ffi.string(C.__lu_app_host_get_gpu_name()),
        GRAPHICS_API = ffi.string(C.__lu_app_host_get_gapi_name()),
        HOST = jit.os..' '..jit.arch,
        NUM_CPUS = math.max(1, C.__lu_app_host_get_num_cpus()),
    },
    Utils = {}
}

function App.isFocused()
    return C.__lu_app_is_focused()
end

function App.isUIHovered()
    return C.__lu_app_is_ui_hovered()
end

function App.Window.maximize()
    C.__lu_window_maximize()
    App.Window.isMaximized = true
end

function App.Window.minimize()
    C.__lu_window_minimize()
    App.Window.isMaximized = false
end

function App.Window.enterFullscreen()
    C.__lu_window_enter_fullscreen()
    App.Window.isFullscreen = true
end

function App.Window.leaveFullscreen()
    C.__lu_window_leave_fullscreen()
    App.Window.isFullscreen = false
end

function App.Window.setTitle(title)
    C.__lu_window_set_title(title)
end

function App.Window.setSize(width, height)
    C.__lu_window_set_size(width, height)
end

function App.Window.setPos(x, y)
    C.__lu_window_set_pos(x, y)
end

function App.Window.show()
    C.__lu_window_show()
    App.Window.isVisible = true
end

function App.Window.hide()
    C.__lu_window_hide()
    App.Window.isVisible = false
end

function App.Window.allowResize(allow)
    C.__lu_window_allow_resize(allow)
end

function App.Window.getSize()
    return C.__lu_window_get_size()
end

function App.Window.getFrameBufSize()
    return C.__lu_window_get_framebuf_size()
end

function App.Window.getPos()
    return C.__lu_window_get_pos()
end

function App.Window.setPlatformTitle(suffix)
    if suffix and type(suffix) == 'string' then
        App.Window.setTitle(string.format('Lunam Engine v.%s - %s %s - %s', App.engineVersion, jit.os, jit.arch:upper(), suffix))
    else
        App.Window.setTitle(string.format('Lunam Engine v.%s - %s %s', App.engineVersion, jit.os, jit.arch:upper()))
    end
end

function App.Window.enableCursor(enable)
    C.__lu_window_enable_cursor(enable)
end

function App.exit()
    C.__lu_app_exit()
end

function App.Utils.openFileDialog(fileTypes, filters, defaultPath)
    filters = filters or ''
    defaultPath = defaultPath or ''
    return ffi.string(C.__lu_app_open_file_dialog(fileTypes, filters, defaultPath))
end

function App.Utils.openFolderDialog(defaultPath)
    defaultPath = defaultPath or ''
    return ffi.string(C.__lu_app_open_folder_dialog(defaultPath))
end

App.Window.setPlatformTitle()

return App
