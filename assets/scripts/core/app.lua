-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- app Module - Functions for interacting with the host application, window and OS.
--- @module app
------------------------------------------------------------------------------

local jit = require 'jit'
local bit = require 'bit'
local ffi = require 'ffi'
local cpp = ffi.C

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

--- app Module
local app = {
    info = { -- User defined application information
        name = 'Untitled app', -- Name of the application
        appVersion = '0.1', -- Version of the application
        author = 'Anonymous', -- Author of the application
        copyright = '', -- Copyright of the application
        website = '', -- Website of the application
        company = '', -- Company of the application
        type = 'Game', -- Type of the application
    },
    window = { -- window related functions and properties
        _is_maximized = false, -- Is the window maximized
        _is_fullscreen = false, -- Is the window fullscreen
        _is_visible = true, -- Is the window visible
    },
    host = { -- host machine related properties
        cpu_name = ffi.string(cpp.__lu_app_host_get_cpu_name()), -- Name of the CPU
        gpu_name = ffi.string(cpp.__lu_app_host_get_gpu_name()), --  Name of the GPU
        graphics_api = ffi.string(cpp.__lu_app_host_get_gapi_name()), -- Name of the graphics API
        host_string = jit.os..' '..jit.arch, -- host OS and architecture
        hardware_concurrency = math.max(1, cpp.__lu_app_host_get_num_cpus()), -- Number of CPUs
    },
    utils = {} -- Utility functions
}

--- Panic and terminate the application
-- @tparam string msg Panic message
function app.panic(msg)
    cpp.__lu_panic(msg)
end

--- Get the FFI cookie
-- @treturn number FFI cookie
function app.ffi_cookie()
    return cpp.__lu_ffi_cookie()
end

--- Get engine version as packed integer (major << 8 | minor)
-- @treturn number Engine version packed as integer
function app.engine_version_packed()
    return cpp.__lu_engine_version()
end

--- Get engine version as unpacked integers (major, minor)
-- @treturn {number, ...} Engine version unpacked as integers
function app.engine_version_unpacked()
    local packed = app.engine_version_packed()
    local major = bit.band(bit.rshift(packed, 8), 255)
    local minor = bit.band(packed, 255)
    return major, minor
end

--- Get engine version string
-- @treturn string Engine version string with format: v.major.minor
function app.engine_version_str()
    local major, minor = app.engine_version_unpacked()
    return string.format('v.%d.%d', major, minor)
end

--- Check if the application is user focused
-- @return (boolean): Is the application user focused
function app.is_focused()
    return cpp.__lu_app_is_focused()
end

--- Check if the application UI is hovered (editor UI and ingame UI)
-- @return (boolean): Is the application UI hovered
function app.is_any_ui_hovered()
    return cpp.__lu_app_is_ui_hovered()
end

--- Hot reload the ingame UI and update changes
-- @tparam boolean|nil enable_wireframe Enable UI wireframe debugdraw mode after reloading
function app.hot_reload_game_ui(enable_wireframe)
    cpp.__lu_app_hot_reload_ui(enable_wireframe or false)
end

--- Hot reload shaders and update changes
function app.hot_reload_shaders()
    cpp.__lu_app_hot_reload_shaders()
end

--- Maximize the window
function app.window.maximize()
    cpp.__lu_window_maximize()
    app.window._is_maximized = true
end

--- Minimize the window
function app.window.minimize()
    cpp.__lu_window_minimize()
    app.window._is_maximized = false
end

--- Enter fullscreen mode
function app.window.fillscreen_enter()
    cpp.__lu_window_enter_fullscreen()
    app.window._is_fullscreen = true
end

--- Leave fullscreen mode
function app.window.fillscreen_exit()
    cpp.__lu_window_leave_fullscreen()
    app.window._is_fullscreen = false
end

--- Set window title
-- @tparam string title Title of the window
function app.window.set_title(title)
    cpp.__lu_window_set_title(title)
end

--- Set window size
-- @tparam number width Width of the window
-- @tparam number height Height of the window
function app.window.set_size(width, height)
    cpp.__lu_window_set_size(width, height)
end

--- Set window position
-- @tparam number x X position of the window (from top left corner of the screen)
-- @tparam number y Y position of the window (from top left corner of the screen)
function app.window.set_position(x, y)
    cpp.__lu_window_set_pos(x, y)
end

--- Show the window
function app.window.show()
    cpp.__lu_window_show()
    app.window._is_visible = true
end

--- Hide the window
function app.window.hide()
    cpp.__lu_window_hide()
    app.window._is_visible = false
end

--- Allow window resize
-- @tparam boolean allow Allow window resize
function app.window.allow_resize(allow)
    cpp.__lu_window_allow_resize(allow)
end

--- Get window size
-- @treturn {number, ...} Width and height of the window
function app.window.get_size()
    return cpp.__lu_window_get_size()
end

--- Get window frame buffer size
-- @treturn {number, number} Width and height of the window frame buffer
function app.window.get_frame_buffer_size()
    return cpp.__lu_window_get_framebuf_size()
end

--- Get window position
-- @treturn {number, ...} X and Y position of the window
function app.window.get_position()
    return cpp.__lu_window_get_pos()
end

--- Set window title to include platform information
-- @tparam string suffix Suffix to add to the title
function app.window.set_platform_title(suffix)
    if suffix and type(suffix) == 'string' then
        app.window.set_title(string.format('Lunam Engine %s - %s %s - %s', app.engine_version_str(), jit.os, jit.arch, suffix))
    else
        app.window.set_title(string.format('Lunam Engine %s - %s %s', app.engine_version_str(), jit.os, jit.arch))
    end
end

--- Enable or disable cursor
-- @tparam boolean enable Enable or disable cursor
function app.window.enable_cursor(enable)
    cpp.__lu_window_enable_cursor(enable)
end

--- Exit the application
function app.exit()
    cpp.__lu_app_exit()
end

--- Open native file dialog
-- @tparam string fileTypes File types to open
-- @tparam string filters Filters for the file dialog
-- @tparam string defaultPath Default path to open the file dialog
function app.utils.open_file_dialog(file_types, filters, default_ath)
    filters = filters or ''
    default_ath = default_ath or ''
    return ffi.string(cpp.__lu_app_open_file_dialog(file_types, filters, default_ath))
end

--- Open native folder dialog
-- @tparam string defaultPath Default path to open the folder dialog
function app.utils.open_folder_dialog(default_path)
    default_path = default_path or ''
    return ffi.string(cpp.__lu_app_open_folder_dialog(default_path))
end

app.window.set_platform_title()

return app
