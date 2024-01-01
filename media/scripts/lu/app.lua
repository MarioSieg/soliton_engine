-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef [[
    typedef struct { int v[2]; } __lu_ivec2;
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
    __lu_ivec2 __lu_window_get_size(void);
    __lu_ivec2 __lu_window_get_pos(void);
    void __lu_app_exit(void);
]]

app = {
    name = 'Untitled App',
    appVersion = '0.1',
    engineVersion = '0.1',
    author = 'Anonymous',
    copyright = '',
    website = '',
    company = '',
    type = 'Game',
    window = {
        isMaximized = false,
        isFullscreen = false,
        isVisible = true,
    }
}

function app.window.maximize()
    ffi.C.__lu_window_maximize()
    app.window.isMaximized = true
end

function app.window.minimize()
    ffi.C.__lu_window_minimize()
    app.window.isMaximized = false
end

function app.window.enterFullscreen()
    ffi.C.__lu_window_enter_fullscreen()
    app.window.isFullscreen = true
end

function app.window.leaveFullscreen()
    ffi.C.__lu_window_leave_fullscreen()
    app.window.isFullscreen = false
end

function app.window.setTitle(title)
    assert(type(title) == 'string')
    ffi.C.__lu_window_set_title(title)
end

function app.window.setSize(width, height)
    assert(type(width) == 'number')
    assert(type(height) == 'number')
    ffi.C.__lu_window_set_size(width, height)
end

function app.window.setPos(x, y)
    assert(type(x) == 'number')
    assert(type(y) == 'number')
    ffi.C.__lu_window_set_pos(x, y)
end

function app.window.show()
    ffi.C.__lu_window_show()
    app.window.isVisible = true
end

function app.window.hide()
    ffi.C.__lu_window_hide()
    app.window.isVisible = false
end

function app.window.allowResize(allow)
    assert(type(allow) == 'boolean')
    ffi.C.__lu_window_allow_resize(allow)
end

function app.window.getSize()
    local v = ffi.C.__lu_window_get_size()
    return v.v[0], v.v[1]
end

function app.window.getPos()
    local v = ffi.C.__lu_window_get_pos()
    return v.v[0], v.v[1]
end

function app.exit()
    ffi.C.__lu_app_exit()
end

app.window.setTitle(string.format('Lunam Engine v.%s - %s %s', app.engineVersion, jit.os, jit.arch))

return app
