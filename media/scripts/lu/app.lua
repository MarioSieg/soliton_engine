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
]]

App = {
    name = 'Untitled App',
    version = '0.0.1',
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

function App.window.maximize()
    ffi.C.__lu_window_maximize()
    App.window.isMaximized = true
end

function App.window.minimize()
    ffi.C.__lu_window_minimize()
    App.window.isMaximized = false
end

function App.window.enter_fullscreen()
    ffi.C.__lu_window_enter_fullscreen()
    App.window.isFullscreen = true
end

function App.window.leave_fullscreen()
    ffi.C.__lu_window_leave_fullscreen()
    App.window.isFullscreen = false
end

function App.window.set_title(title)
    assert(type(title) == 'string')
    ffi.C.__lu_window_set_title(title)
end

function App.window.set_size(width, height)
    assert(type(width) == 'number')
    assert(type(height) == 'number')
    ffi.C.__lu_window_set_size(width, height)
end

function App.window.set_pos(x, y)
    assert(type(x) == 'number')
    assert(type(y) == 'number')
    ffi.C.__lu_window_set_pos(x, y)
end

function App.window.show()
    ffi.C.__lu_window_show()
    App.window.isVisible = true
end

function App.window.hide()
    ffi.C.__lu_window_hide()
    App.window.isVisible = false
end

function App.window.allow_resize(allow)
    assert(type(allow) == 'boolean')
    ffi.C.__lu_window_allow_resize(allow)
end

function App.window.get_size()
    local v = ffi.C.__lu_window_get_size()
    return v.v[0], v.v[1]
end

function App.window.get_pos()
    local v = ffi.C.__lu_window_get_pos()
    return v.v[0], v.v[1]
end

App.window.maximize()

return App
