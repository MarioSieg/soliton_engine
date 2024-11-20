-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- input Module - Functions for input handling.
--- @module input
------------------------------------------------------------------------------

local ffi = require 'ffi'
local cpp = ffi.C

ffi.cdef [[
    bool __lu_input_is_key_pressed(int key_code);
    bool __lu_input_is_key_released(int key_code);
    bool __lu_input_is_mouse_button_pressed(int mb_code);
    bool __lu_input_is_mouse_button_released(int mb_code);
    __vec2 __lu_input_get_mouse_pos(void);
]]

--- input Module
local input = {
    --- keyboard key codes
    keys = {
        space = 32,
        apostrophe = 39,
        comma = 44,
        minus = 45,
        period = 46,
        slash = 47,
        key0 = 48,
        key1 = 49,
        key2 = 50,
        key3 = 51,
        key4 = 52,
        key5 = 53,
        key6 = 54,
        key7 = 55,
        key8 = 56,
        key9 = 57,
        semicolon = 59,
        equal = 61,
        a = 65,
        b = 66,
        cpp = 67,
        d = 68,
        e = 69,
        f = 70,
        g = 71,
        h = 72,
        i = 73,
        j = 74,
        k = 75,
        l = 76,
        m = 77,
        n = 78,
        o = 79,
        p = 80,
        q = 81,
        r = 82,
        s = 83,
        t = 84,
        u = 85,
        v = 86,
        w = 87,
        x = 88,
        y = 89,
        z = 90,
        left_bracket = 91,
        backslash = 92,
        right_bracket = 93,
        graveaccent = 96,
        world1 = 161,
        world2 = 162,
        escape = 256,
        enter = 257,
        tab = 258,
        backspace = 259,
        insert = 260,
        delete = 261,
        right = 262,
        left = 263,
        down = 264,
        up = 265,
        pageup = 266,
        pagedown = 267,
        home = 268,
        kend = 269,
        capslock = 280,
        scrolllock = 281,
        numlock = 282,
        printscreen = 283,
        pause = 284,
        f1 = 290,
        f2 = 291,
        f3 = 292,
        f4 = 293,
        f5 = 294,
        f6 = 295,
        f7 = 296,
        f8 = 297,
        f9 = 298,
        f10 = 299,
        f11 = 300,
        f12 = 301,
        f13 = 302,
        f14 = 303,
        f15 = 304,
        f16 = 305,
        f17 = 306,
        f18 = 307,
        f19 = 308,
        f20 = 309,
        f21 = 310,
        f22 = 311,
        f23 = 312,
        f24 = 313,
        f25 = 314,
        kp0 = 320,
        kp1 = 321,
        kp2 = 322,
        kp3 = 323,
        kp4 = 324,
        kp5 = 325,
        kp6 = 326,
        kp7 = 327,
        kp8 = 328,
        kp9 = 329,
        keypad_decimal = 330,
        keypad_divide = 331,
        keypad_multiply = 332,
        keypad_subtract = 333,
        keypad_add = 334,
        keypad_enter = 335,
        keypad_equal = 336,
        left_shift = 340,
        left_control = 341,
        left_alt = 342,
        left_super = 343,
        right_shift = 344,
        right_control = 345,
        right_alt = 346,
        right_super = 347,
        menu = 348
    },
    --- mouse button codes
    mouse_buttons = {
        mb1 = 0,
        mb2 = 1,
        mb3 = 2,
        mb4 = 3,
        mb5 = 4,
        mb6 = 5,
        mb7 = 6,
        mb8 = 7,
        left = 0,
        right = 1,
        middle = 2
    }
}

--- Checks if the specified key is pressed.
-- @tparam input.KEYS key The key code
-- @treturn bool True if the key is pressed
function input.is_key_pressed(key)
    return cpp.__lu_input_is_key_pressed(key)
end

--- Checks if the specified key is released.
-- @tparam input.KEYS key The key code
-- @treturn bool True if the key is released
function input.is_key_released(key)
    return cpp.__lu_input_is_key_released(key)
end

--- Checks if the specified mouse button is pressed.
-- @tparam input.MOUSE_BUTTONS mb The mouse button code
-- @treturn bool True if the mouse button is pressed
function input.is_mouse_button_pressed(mb)
    return cpp.__lu_input_is_mouse_button_pressed(mb)
end

--- Checks if the specified mouse button is released.
-- @tparam input.MOUSE_BUTTONS mb The mouse button code
-- @treturn bool True if the mouse button is released
function input.is_mouse_button_released(mb)
    return cpp.__lu_input_is_mouse_button_released(mb)
end

--- Gets the current mouse position.
-- @treturn gmath.vec2 The current mouse position
function input.get_mouse_position()
    return cpp.__lu_input_get_mouse_pos()
end

return input
