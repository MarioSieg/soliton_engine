-- Copyright (c) 2024 Mario 'Neo' Sieg. All Rights Reserved.

local vec3 = require 'vec3'
local quat = require 'quat'
local gmath = require 'gmath'
local time = require 'time'
local debugdraw = require 'debugdraw'
local rad, sin, cos, tan, atan2, asin, pi = math.rad, math.sin, math.cos, math.tan, math.atan2, math.asin, math.pi
local clamp = gmath.clamp

-- Simplified time cycle system:
-- 24 hour day
-- 30 days per month (12 months)
-- 30 * 12 = 360 days per year
local chrono = {
    seasons = { -- Season names and their index
        spring = 1,
        summer = 2,
        autumn = 3,
        winter = 4
    },
    months = { -- Month names and their index
        january = 1,
        february = 2,
        march = 3,
        april = 4,
        may = 5,
        june = 6,
        july = 7,
        august = 8,
        september = 9,
        october = 10,
        november = 11,
        december = 12
    },
    time_cycle_scale = 1.0, -- Time cycle speed multiplier
    date = { -- Current date and time
        day = 1,
        month = 6,
        year = 2024,
    },
    time = 4.45, -- Current time of day (0.0 - 24.0)
    debug_draw = false, -- Draw debug arrows for sun and moon
    sun_latitude = 50.0, -- Latitude of the sun
    daytime_coeff = 0.0, -- 0.0 = night, 1.0 = day

    _north_dir = vec3.unit_x,
    _sun_dir = -vec3.unit_y,
    _sun_dir_quat = quat.identity,
    _delta = 0.0,
    _ecliptic_obliquity = rad(23.44)
}

function chrono:get_date_string() return string.format('%02d.%02d.%04d', self.date.day, self.date.month, self.date.year) end
function chrono:get_time_string() return string.format('%02d:%02d', math.floor(self.time), (self.time % 1) * 60) end
function chrono:get_date_time_string() return string.format('%s %s', self:get_date_string(), self:get_time_string()) end
function chrono:is_hour_passed(hour) return self.time >= hour end
function chrono:is_day_passed(day) return self.date.day >= day end
function chrono:is_month_passed(month) return self.date.month >= month end
function chrono:is_year_passed(year) return self.date.year >= year end
function chrono:is_spring() return self.current_season == self.seasons.spring end
function chrono:is_summer() return self.current_season == self.seasons.summer end
function chrono:is_autumn() return self.current_season == self.seasons.autumn end
function chrono:is_winter() return self.current_season == self.seasons.winter end
function chrono:is_morning() return self.time >= 6 and self.time < 12 end
function chrono:is_afternoon() return self.time >= 12 and self.time < 18 end
function chrono:is_evening() return self.time >= 18 and self.time < 24 end
function chrono:is_night() return self.time >= 0 and self.time < 6 end
function chrono:set_time_scale(real_minutes_per_game_day)
    local real_seconds_per_game_day = real_minutes_per_game_day * 60
    self.time_cycle_scale = 24 / real_seconds_per_game_day
end

-- Calculate day-night factor based on time of day. 0.0 = night, 1.0 = day
local function daytime_coeff()
    local hour_angle = rad(chrono.time / 24 * 360 - 180) -- Angle for the time of day
    chrono.daytime_coeff = clamp((cos(hour_angle) + 1) / 2, 0, 1) -- Normalized day-night factor
end

local function compute_sun_orbit()
    local day = 30 * chrono.date.month - 1 + 15
    local lambda = rad(280.46 + 0.9856474 * day) -- Ecliptic longitude
    chrono._delta = asin(sin(chrono._ecliptic_obliquity) * sin(lambda))
end

local function compute_sun_dir(hour)
    local lat = rad(chrono.sun_latitude)
    local lc = cos(lat)
    local ls = sin(lat)
    local hh = hour * pi / 12
    local hhc = cos(hh)
    local azimuth = atan2(sin(hh), hhc * ls - tan(chrono._delta) * lc)
    local alt = asin(ls * sin(chrono._delta) + lc * cos(chrono._delta) * hhc)
    local rot0 = quat.from_axis_angle(vec3.up, azimuth)
    local dir = chrono._north_dir * rot0
    local udx = vec3.cross(vec3.up, dir)
    local rot1 = quat.from_axis_angle(udx, alt)
    chrono._sun_dir_quat = rot0 * rot1
    chrono._sun_dir = dir * rot1
end

function chrono:set_time(hour)
    compute_sun_orbit()
    compute_sun_dir(clamp(hour, 0, 24) - 12)
    daytime_coeff()
end

local season_map = {
    [1] = 'winter', [2] = 'winter', [3] = 'spring',
    [4] = 'spring', [5] = 'spring', [6] = 'summer',
    [7] = 'summer', [8] = 'summer', [9] = 'autumn',
    [10] = 'autumn', [11] = 'autumn', [12] = 'winter'
}

function chrono:update()
    if self.debug_draw then
        debugdraw.draw_arrow_dir(vec3.zero, -self._sun_dir * 2.0, colors.yellow)
    end
    self:set_time(self.time)
    self.time = self.time + time.delta_time * self.time_cycle_scale
    self.time = self.time % 24.0
    self.date.day = self.date.day + (self.time == 0 and 1 or 0)
    self.date.month = self.date.month + (self.date.day > 30 and 1 or 0)
    self.date.day = (self.date.day > 30) and 1 or self.date.day
    self.date.year = self.date.year + (self.date.month > 12 and 1 or 0)
    self.date.month = (self.date.month > 12) and 1 or self.date.month
    self.current_season = self.seasons[season_map[self.date.month]]
end

chrono:set_time_scale(60)

return chrono
