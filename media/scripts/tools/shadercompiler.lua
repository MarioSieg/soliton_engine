-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local EXECUTABLE = 'tools/shaderc'
if jit.os == 'Windows' then
    EXECUTABLE = EXECUTABLE..'.exe'
end
