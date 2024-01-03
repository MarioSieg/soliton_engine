-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

-- Some Lua extensions to make life easier.

function string.startsWith(str, start)
    return string.sub(str, 1, string.len(start)) == start
end

function string.endsWith(str, ending)
    return ending == '' or string.sub(str, -string.len(ending)) == ending
end

function table.copyDeep(orig, copies)
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

function table.copyShallow(orig)
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
