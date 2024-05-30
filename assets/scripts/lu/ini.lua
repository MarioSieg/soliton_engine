-- Copyright (c) 2022-2023 Mario 'Neo' Sieg. All Rights Reserved.
-- Simple INI file parser and writer.

local ini = {}

--- Deserialize INI file to table.
--@tparam string fileName File path of the INI file to load.
--@treturn table Table containing all INI data.
function ini.deserialize(fileName)
	assert(type(fileName) == 'string')
	local file = assert(io.open(fileName, 'r'), 'Failed to open INI file: '..fileName)
	local data = {}
	local section
	for line in file:lines() do
		local tempSection = line:match('^%[([^%[%]]+)%]$')
		if tempSection then
			section = tonumber(tempSection) and tonumber(tempSection) or tempSection
			data[section] = data[section] or {}
		end
		local param, value = line:match('^([%w|_]+)%s-=%s-(.+)$')
		if param and value ~= nil then
			if tonumber(value) then
				value = tonumber(value)
			elseif value == 'true' then
				value = true
			elseif value == 'false' then
				value = false
			end
			if tonumber(param) then
				param = tonumber(param)
			end
			data[section][param] = value
		end
	end
	file:close()
	return data
end

--- Serialize table to INI file.
--@tparam string fileName File path of the INI file to save.
--@tparam data Table containing all data to be stored in the INI file.
function ini.serialize(fileName, data)
	assert(type(fileName) == 'string' and type(data) == 'table')
	local file = assert(io.open(fileName, 'w+b'), 'Error loading file :' .. fileName)
	local contents = ''
	for section, param in pairs(data) do
		contents = contents..('[%s]\n'):format(section)
		for key, value in pairs(param) do
			contents = contents..('%s=%s\n'):format(key, tostring(value))
		end
		contents = contents..'\n'
	end
	file:write(contents)
	file:close()
end

return ini