-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
-- input Module - Functions for input handling.
-- @module input
------------------------------------------------------------------------------

local ffi = require 'ffi'

ffi.cdef[[
    bool __lu_input_is_key_pressed(int key_code);
    bool __lu_input_is_key_released(int key_code);
    bool __lu_input_is_mouse_button_pressed(int mb_code);
    bool __lu_input_is_mouse_button_released(int mb_code);
    lua_vec2 __lu_input_get_mouse_pos(void);
]]

local C = ffi.C

--- input Module
local input = {
    --- Keyboard key codes
    KEYS = {
        SPACE = 32,
        APOSTROPHE = 39,
        COMMA = 44,
        MINUS = 45,
        PERIOD = 46,
        SLASH = 47,
        KEY0 = 48,
        KEY1 = 49,
        KEY2 = 50,
        KEY3 = 51,
        KEY4 = 52,
        KEY5 = 53,
        KEY6 = 54,
        KEY7 = 55,
        KEY8 = 56,
        KEY9 = 57,
        SEMICOLON = 59,
        EQUAL = 61,
        A = 65,
        B = 66,
        C = 67,
        D = 68,
        E = 69,
        F = 70,
        G = 71,
        H = 72,
        I = 73,
        J = 74,
        K = 75,
        L = 76,
        M = 77,
        N = 78,
        O = 79,
        P = 80,
        Q = 81,
        R = 82,
        S = 83,
        T = 84,
        U = 85,
        V = 86,
        W = 87,
        X = 88,
        Y = 89,
        Z = 90,
        LEFT_BRACKET = 91,
        BACKSLASH = 92,
        RIGHT_BRACKET = 93,
        GRAVEACCENT = 96,
        WORLD1 = 161,
        WORLD2 = 162,
        ESCAPE = 256,
        ENTER = 257,
        TAB = 258,
        BACKSPACE = 259,
        INSERT = 260,
        DELETE = 261,
        RIGHT = 262,
        LEFT = 263,
        DOWN = 264,
        UP = 265,
        PAGEUP = 266,
        PAGEDOWN = 267,
        HOME = 268,
        END = 269,
        CAPSLOCK = 280,
        SCROLLLOCK = 281,
        NUMLOCK = 282,
        PRINTSCREEN = 283,
        PAUSE = 284,
        F1 = 290,
        F2 = 291,
        F3 = 292,
        F4 = 293,
        F5 = 294,
        F6 = 295,
        F7 = 296,
        F8 = 297,
        F9 = 298,
        F10 = 299,
        F11 = 300,
        F12 = 301,
        F13 = 302,
        F14 = 303,
        F15 = 304,
        F16 = 305,
        F17 = 306,
        F18 = 307,
        F19 = 308,
        F20 = 309,
        F21 = 310,
        F22 = 311,
        F23 = 312,
        F24 = 313,
        F25 = 314,
        KP0 = 320,
        KP1 = 321,
        KP2 = 322,
        KP3 = 323,
        KP4 = 324,
        KP5 = 325,
        KP6 = 326,
        KP7 = 327,
        KP8 = 328,
        KP9 = 329,
        KEYPAD_DECIMAL = 330,
        KEYPAD_DIVIDE = 331,
        KEYPAD_MULTIPLY = 332,
        KEYPAD_SUBTRACT = 333,
        KEYPAD_ADD = 334,
        KEYPAD_ENTER = 335,
        KEYPAD_EQUAL = 336,
        LEFT_SHIFT = 340,
        LEFT_CONTROL = 341,
        LEFT_ALT = 342,
        LEFT_SUPER = 343,
        RIGHT_SHIFT = 344,
        RIGHT_CONTROL = 345,
        RIGHT_ALT = 346,
        RIGHT_SUPER = 347,
        MENU = 348
    },
    --- Mouse button codes
    MOUSE_BUTTONS = {
        MB1 = 0,
        MB2 = 1,
        MB3 = 2,
        MB4 = 3,
        MB5 = 4,
        MB6 = 5,
        MB7 = 6,
        MB8 = 7,
        LEFT = 0,
        RIGHT = 1,
        MIDDLE = 2
    }
}

--- Checks if the specified key is pressed.
-- @tparam input.KEYS key The key code
-- @treturn bool True if the key is pressed
function input.isKeyPressed(key)
    return C.__lu_input_is_key_pressed(key)
end

--- Checks if the specified key is released.
-- @tparam input.KEYS key The key code
-- @treturn bool True if the key is released
function input.isKeyReleased(key)
    return C.__lu_input_is_key_released(key)
end

--- Checks if the specified mouse button is pressed.
-- @tparam input.MOUSE_BUTTONS mb The mouse button code
-- @treturn bool True if the mouse button is pressed
function input.isMouseButtonPressed(mb)
    return C.__lu_input_is_mouse_button_pressed(mb)
end

--- Checks if the specified mouse button is released.
-- @tparam input.MOUSE_BUTTONS mb The mouse button code
-- @treturn bool True if the mouse button is released
function input.isMouseButtonReleased(mb)
    return C.__lu_input_is_mouse_button_released(mb)
end

--- Gets the current mouse position.
-- @treturn gmath.vec2 The current mouse position
function input.getMousePos()
    return C.__lu_input_get_mouse_pos()
end

return input
