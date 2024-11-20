----------------------------------------------------------------------------
-- Soliton Engine Utility Module
--
-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.
------------------------------------------------------------------------------

local ffi = require 'ffi'

local utils = {}

utils.ring_buffer = {
    data = nil,
    size = 0,
    idx = 0
}

function utils.ring_buffer:new(size)
    local o = {}
    setmetatable(o, self)
    self.__index = self
    o.data = ffi.new('double[?]', size)
    o.size = size
    o.idx = 0
    o:clear()
    return o
end

function utils.ring_buffer:push(value)
    self.data[self.idx] = value
    self.idx = (self.idx + 1) % self.size
end

function utils.ring_buffer:avg()
    local sum = 0.0
    for i = 0, self.size - 1 do
        sum = sum + self.data[i]
    end
    return sum / self.size
end

function utils.ring_buffer:clear()
    for i = 0, self.size - 1 do
        self.data[i] = 0.0
    end
end

function utils.ring_buffer:min_max_avg(data, size)
    local min = math.huge
    local max = -math.huge
    local sum = 0.0
    for i = 0, size - 1 do
        local value = data[i]
        if value < min then
            min = value
        end
        if value > max then
            max = value
        end
        sum = sum + value
    end
    return min, max, sum / size
end

function utils.ring_buffer:min()
    local min = math.huge
    for i = 0, self.size - 1 do
        local value = self.data[i]
        if value < min then
            min = value
        end
    end
    return min
end

function utils.ring_buffer:max()
    local max = -math.huge
    for i = 0, self.size - 1 do
        local value = self.data[i]
        if value > max then
            max = value
        end
    end
    return max
end

return utils
