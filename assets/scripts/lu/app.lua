-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
-- app Module - Functions for interacting with the host application, window and OS.
-- @module app
------------------------------------------------------------------------------

local jit = require 'jit'
local bit = require 'bit'
local ffi = require 'ffi'

ffi.cdef [[
    void __lu_panic(const char* msg);
    uint32_t __lu_ffi_cookie(void);
    uint32_t __lu_engine_version(void);
    bool __lu_app_is_focused(void);
    bool __lu_app_is_ui_hovered(void);
    void __lu_app_hot_reload_ui(bool enable_wireframe);
    void __lu_app_hot_reload_shaders(void);
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

--- app Module
local app = {
    Info = { -- User defined application information
        name = 'Untitled app', -- Name of the application
        appVersion = '0.1', -- Version of the application
        author = 'Anonymous', -- Author of the application
        copyright = '', -- Copyright of the application
        website = '', -- Website of the application
        company = '', -- Company of the application
        type = 'Game', -- Type of the application
    },
    Window = { -- Window related functions and properties
        isMaximized = false, -- Is the window maximized
        isFullscreen = false, -- Is the window fullscreen
        isVisible = true, -- Is the window visible
    },
    Host = { -- Host machine related properties
        CPU_NAME = ffi.string(C.__lu_app_host_get_cpu_name()), -- Name of the CPU
        GPU_NAME = ffi.string(C.__lu_app_host_get_gpu_name()), --  Name of the GPU
        GRAPHICS_API = ffi.string(C.__lu_app_host_get_gapi_name()), -- Name of the graphics API
        HOST = jit.os..' '..jit.arch, -- Host OS and architecture
        NUM_CPUS = math.max(1, C.__lu_app_host_get_num_cpus()), -- Number of CPUs
    },
    Utils = {} -- Utility functions
}

--- Panic and terminate the application
-- @tparam string msg Panic message
function app.panic(msg)
    C.__lu_panic(msg)
end

--- Get the FFI cookie
-- @treturn number FFI cookie
function app.ffiCookie()
    return C.__lu_ffi_cookie()
end

--- Get engine version as packed integer (major << 8 | minor)
-- @treturn number Engine version packed as integer
function app.engineVersionPacked()
    return C.__lu_engine_version()
end

--- Get engine version as unpacked integers (major, minor)
-- @treturn {number, ...} Engine version unpacked as integers
function app.engineVersionUnpacked()
    local packed = app.engineVersionPacked()
    local major = bit.band(bit.rshift(packed, 8), 0xff)
    local minor = bit.band(packed, 0xff)
    return major, minor
end

--- Get engine version string
-- @treturn string Engine version string with format: v.major.minor
function app.engineVersionStr()
    local major, minor = app.engineVersionUnpacked()
    return string.format('v.%d.%d', major, minor)
end

--- Check if the application is user focused
-- @return (boolean): Is the application user focused
function app.isFocused()
    return C.__lu_app_is_focused()
end

--- Check if the application UI is hovered (editor UI and ingame UI)
-- @return (boolean): Is the application UI hovered
function app.isUIHovered()
    return C.__lu_app_is_ui_hovered()
end

--- Hot reload the ingame UI and update changes
-- @tparam boolean|nil enable_wireframe Enable UI wireframe debugdraw mode after reloading
function app.hotReloadUI(enable_wireframe)
    C.__lu_app_hot_reload_ui(enable_wireframe or false)
end

--- Hot reload shaders and update changes
function app.hotReloadShaders()
    C.__lu_app_hot_reload_shaders()
end

--- Maximize the window
function app.Window.maximize()
    C.__lu_window_maximize()
    app.Window.isMaximized = true
end

--- Minimize the window
function app.Window.minimize()
    C.__lu_window_minimize()
    app.Window.isMaximized = false
end

--- Enter fullscreen mode
function app.Window.enterFullscreen()
    C.__lu_window_enter_fullscreen()
    app.Window.isFullscreen = true
end

--- Leave fullscreen mode
function app.Window.leaveFullscreen()
    C.__lu_window_leave_fullscreen()
    app.Window.isFullscreen = false
end

--- Set window title
-- @tparam string title Title of the window
function app.Window.setTitle(title)
    C.__lu_window_set_title(title)
end

--- Set window size
-- @tparam number width Width of the window
-- @tparam number height Height of the window
function app.Window.setSize(width, height)
    C.__lu_window_set_size(width, height)
end

--- Set window position
-- @tparam number x X position of the window (from top left corner of the screen)
-- @tparam number y Y position of the window (from top left corner of the screen)
function app.Window.setPos(x, y)
    C.__lu_window_set_pos(x, y)
end

--- Show the window
function app.Window.show()
    C.__lu_window_show()
    app.Window.isVisible = true
end

--- Hide the window
function app.Window.hide()
    C.__lu_window_hide()
    app.Window.isVisible = false
end

--- Allow window resize
-- @tparam boolean allow Allow window resize
function app.Window.allowResize(allow)
    C.__lu_window_allow_resize(allow)
end

--- Get window size
-- @treturn {number, ...} Width and height of the window
function app.Window.getSize()
    return C.__lu_window_get_size()
end

--- Get window frame buffer size
-- @treturn {number, number} Width and height of the window frame buffer
function app.Window.getFrameBufSize()
    return C.__lu_window_get_framebuf_size()
end

--- Get window position
-- @treturn {number, ...} X and Y position of the window
function app.Window.getPos()
    return C.__lu_window_get_pos()
end

--- Set window title to include platform information
-- @tparam string suffix Suffix to add to the title
function app.Window.setPlatformTitle(suffix)
    if suffix and type(suffix) == 'string' then
        app.Window.setTitle(string.format('Lunam Engine %s - %s %s - %s', app.engineVersionStr(), jit.os, jit.arch, suffix))
    else
        app.Window.setTitle(string.format('Lunam Engine %s - %s %s', app.engineVersionStr(), jit.os, jit.arch))
    end
end

--- Enable or disable cursor
-- @tparam boolean enable Enable or disable cursor
function app.Window.enableCursor(enable)
    C.__lu_window_enable_cursor(enable)
end

--- Exit the application
function app.exit()
    C.__lu_app_exit()
end

--- Open native file dialog
-- @tparam string fileTypes File types to open
-- @tparam string filters Filters for the file dialog
-- @tparam string defaultPath Default path to open the file dialog
function app.Utils.openFileDialog(fileTypes, filters, defaultPath)
    filters = filters or ''
    defaultPath = defaultPath or ''
    return ffi.string(C.__lu_app_open_file_dialog(fileTypes, filters, defaultPath))
end

--- Open native folder dialog
-- @tparam string defaultPath Default path to open the folder dialog
function app.Utils.openFolderDialog(defaultPath)
    defaultPath = defaultPath or ''
    return ffi.string(C.__lu_app_open_folder_dialog(defaultPath))
end

app.Window.setPlatformTitle()

return app
