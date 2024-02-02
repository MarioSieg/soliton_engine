-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local jit = require 'jit'
local ffi = require 'ffi'

ffi.cdef [[
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
    void __lu_app_exit(void);
    const char* __lu_app_host_get_cpu_name(void);
    const char* __lu_app_host_get_gpu_name(void);
    const char* __lu_app_host_get_gapi_name(void);
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
        HOST = jit.os..' '..jit.arch
    }
}

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

function App.exit()
    C.__lu_app_exit()
end

App.Window.setTitle(string.format('Lunam Engine v.%s - %s %s', App.engineVersion, jit.os, jit.arch))
-- App.Window.maximize()

return App
