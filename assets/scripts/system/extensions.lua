-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.
-- Lua library extensions and utility functions.

-- Execute one full GC cycle and stop automatic GC afterwards, because collection restarts the automatic GC.
-- GC will be called every frame anyway.
function collectgarbage_full_cycle()
    collectgarbage('collect')
    collectgarbage('stop')
end

function string.starts_with(str, start)
    return string.sub(str, 1, string.len(start)) == start
end

function string.ends_with(str, ending)
    return ending == '' or string.sub(str, -string.len(ending)) == ending
end

function table.copy_deep(orig, copies)
    assert(type(orig) == 'table')
    copies = copies or {}
    local otype = type(orig)
    local copy
    if otype == 'table' then
        if copies[orig] then
            copy = copies[orig]
        else
            copy = {}
            copies[orig] = copy
            for k, v in next, orig, nil do
                copy[table.copyDeep(k, copies)] = table.copyDeep(v, copies)
            end
            setmetatable(copy, table.copyDeep(getmetatable(orig), copies))
        end
    else -- number, string, boolean, etc
        copy = orig
    end
    return copy
end

function table.copy_shallow(orig)
    local otype = type(orig)
    local copy
    if otype == 'table' then
        copy = {}
        for k, v in pairs(orig) do
            copy[k] = v
        end
    else -- number, string, boolean, etc
        copy = orig
    end
    return copy
end

-- Init protocol logger.
protocol = {}
protocol_errs = 0

local print_proxy = _G.print
function _G.print(...)
    local args = {...}
    local str = string.format('[%s] ', os.date('%H:%M:%S'))
    for i, arg in ipairs(args) do
        if i > 1 then
            str = str..' '
        end
        str = str..tostring(arg)
    end
    table.insert(protocol, {str, false})
    print_proxy(str)
end

function eprint(...)
    local args = {...}
    local str = string.format('[%s] ', os.date('%H:%M:%S'))
    for i, arg in ipairs(args) do
        if i > 1 then
            str = str..' '
        end
        str = str..tostring(arg)
    end
    table.insert(protocol, {str, true})
    protocol_errs = protocol_errs + 1
end

local error_proxy = _G.error
function _G.error(...)
    local args = {...}
    local str = string.format('[%s] ', os.date('%H:%M:%S'))
    for i, arg in ipairs(args) do
        if i > 1 then
            str = str..' '
        end
        str = str..tostring(arg)
    end
    table.insert(protocol, {str, true})
    protocol_errs = protocol_errs + 1
    error_proxy(str)
end

function printsep()
    print('------------------------------------------------------------')
end
