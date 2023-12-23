-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef [[
    typedef struct { int v[2]; } __lu_ivec2;
    void __lu_panic(const char* msg);
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

local m = {
    name = 'Untitled App',
    version = '0.0.1',
    author = 'Anonymous',
    copyright = '',
    website = '',
    company = '',
    type = 'Game',
    window = {
        is_maximized = false,
        is_fullscreen = false,
        is_visible = true,
    }
}

function m.panic(msg)
    assert(type(msg) == 'string')
    ffi.C.__lu_panic(msg)
end

function m.window.maximize()
    ffi.C.__lu_window_maximize()
    m.window.is_maximized = true
end

function m.window.minimize()
    ffi.C.__lu_window_minimize()
    m.window.is_maximized = false
end

function m.window.enter_fullscreen()
    ffi.C.__lu_window_enter_fullscreen()
    m.window.is_fullscreen = true
end

function m.window.leave_fullscreen()
    ffi.C.__lu_window_leave_fullscreen()
    m.window.is_fullscreen = false
end

function m.window.set_title(title)
    assert(type(title) == 'string')
    ffi.C.__lu_window_set_title(title)
end

function m.window.set_size(width, height)
    assert(type(width) == 'number')
    assert(type(height) == 'number')
    ffi.C.__lu_window_set_size(width, height)
end

function m.window.set_pos(x, y)
    assert(type(x) == 'number')
    assert(type(y) == 'number')
    ffi.C.__lu_window_set_pos(x, y)
end

function m.window.show()
    ffi.C.__lu_window_show()
    m.window.is_visible = true
end

function m.window.hide()
    ffi.C.__lu_window_hide()
    m.window.is_visible = false
end

function m.window.allow_resize(allow)
    assert(type(allow) == 'boolean')
    ffi.C.__lu_window_allow_resize(allow)
end

function m.window.get_size()
    local v = ffi.C.__lu_window_get_size()
    return v.v[0], v.v[1]
end

function m.window.get_pos()
    local v = ffi.C.__lu_window_get_pos()
    return v.v[0], v.v[1]
end

return m
