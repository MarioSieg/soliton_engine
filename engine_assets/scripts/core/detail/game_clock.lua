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
local clock = {
    seasons = {
        spring = 1,
        summer = 2,
        autumn = 3,
        winter = 4
    },
    months = {
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
    debug_draw = true,
    time_cycle_scale = 1.0, -- Time cycle speed multiplier
    date = {
        day = 1,
        month = 6,
        year = 2024,
        time = 6,
    },
    daytime_coeff = 0.0, -- 0.0 = night, 1.0 = day
    _north_dir = vec3.unit_x,
    _sun_dir = -vec3.unit_y,
    _sun_dir_quat = quat.identity,
    _latitude = 50.0,
    _delta = 0.0,
    _ecliptic_obliquity = rad(23.44)
}

function clock:get_date_string() return string.format('%02d.%02d.%04d', self.date.day, self.date.month, self.date.year) end
function clock:get_time_string() return string.format('%02d:%02d', math.floor(self.date.time), (self.date.time % 1) * 60) end
function clock:get_date_time_string() return string.format('%s %s', self:get_date_string(), self:get_time_string()) end
function clock:is_hour_passed(hour) return self.date.time >= hour end
function clock:is_day_passed(day) return self.date.day >= day end
function clock:is_month_passed(month) return self.date.month >= month end
function clock:is_year_passed(year) return self.date.year >= year end
function clock:is_spring() return self.current_season == self.seasons.spring end
function clock:is_summer() return self.current_season == self.seasons.summer end
function clock:is_autumn() return self.current_season == self.seasons.autumn end
function clock:is_winter() return self.current_season == self.seasons.winter end
function clock:is_morning() return self.date.time >= 6 and self.date.time < 12 end
function clock:is_afternoon() return self.date.time >= 12 and self.date.time < 18 end
function clock:is_evening() return self.date.time >= 18 and self.date.time < 24 end
function clock:is_night() return self.date.time >= 0 and self.date.time < 6 end
function clock:set_time_scale(real_minutes_per_game_day)
    local real_seconds_per_game_day = real_minutes_per_game_day * 60
    self.time_cycle_scale = 24 / real_seconds_per_game_day
end

-- Calculate day-night factor based on time of day. 0.0 = night, 1.0 = day
local function daytime_coeff()
    local hour_angle = rad(clock.date.time / 24 * 360 - 180) -- Angle for the time of day
    clock.daytime_coeff = clamp((cos(hour_angle) + 1) / 2, 0, 1) -- Normalized day-night factor
end

local function compute_sun_orbit()
    local day = 30 * clock.date.month - 1 + 15
    local lambda = rad(280.46 + 0.9856474 * day) -- Ecliptic longitude
    clock._delta = asin(sin(clock._ecliptic_obliquity) * sin(lambda))
end

local function compute_sun_dir(hour)
    local lat = rad(clock._latitude)
    local lc = cos(lat)
    local ls = sin(lat)
    local hh = hour * pi / 12
    local hhc = cos(hh)
    local azimuth = atan2(sin(hh), hhc * ls - tan(clock._delta) * lc)
    local alt = asin(ls * sin(clock._delta) + lc * cos(clock._delta) * hhc)
    local rot0 = quat.from_axis_angle(vec3.up, azimuth)
    local dir = clock._north_dir * rot0
    local udx = vec3.cross(vec3.up, dir)
    local rot1 = quat.from_axis_angle(udx, alt)
    clock._sun_dir_quat = rot0 * rot1
    clock._sun_dir = dir * rot1
end

function clock:set_time(hour)
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

function clock:update()
    if self.debug_draw then
        debugdraw.draw_arrow_dir(vec3.zero, -self._sun_dir * 2.0, colors.yellow)
    end
    self:set_time(self.date.time)
    self.date.time = self.date.time + time.delta_time * self.time_cycle_scale
    self.date.time = self.date.time % 24.0
    self.date.day = self.date.day + (self.date.time == 0 and 1 or 0)
    self.date.month = self.date.month + (self.date.day > 30 and 1 or 0)
    self.date.day = (self.date.day > 30) and 1 or self.date.day
    self.date.year = self.date.year + (self.date.month > 12 and 1 or 0)
    self.date.month = (self.date.month > 12) and 1 or self.date.month
    self.current_season = self.seasons[season_map[self.date.month]]
end

clock:set_time_scale(60)

return clock
