-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

-- Default internal engine config, hidden from end user

local jit = require 'jit'
local ffi = require 'ffi'
local min, max, ceil = math.min, math.max, math.ceil
ffi.cdef[[
    uint32_t __lu_app_host_get_num_cpus(void);
]]

print('Loading internal engine config...')

-- Default render flags for the in-game UI
local ui_render_flags = {
    wireframe = 1, -- Toggles wireframe mode when rendering triangles
    color_batches = 2, -- Each batch submitted to the GPU is given a unique solid color
    overdraw = 4, -- Displays pixel overdraw using blending layers. Different colors are used for each type of triangles. 'Green' for normal ones, 'Red' for opacities and 'Blue' for clipping masks
    flip_y = 8, -- Inverts the render vertically
    ppaa = 16, -- Per-Primitive Antialiasing extrudes the contours of the geometry and smooths them. It is a 'cheap' antialiasing algorithm useful when GPU MSAA is not enabled
    lcd = 32, -- Enables subpixel rendering compatible with LCD displays
    show_glyphs = 64, -- Displays glyph atlas as a small overlay for debugging purposes
    show_ramps = 128, -- Displays ramp atlas as a small overlay for debugging purposes
    depth_testing = 256 -- Enables testing against the content of the depth buffer
}

-- Tesselation pixel error quality for the in-game UI
local ui_tesselation_error = {
    low = 0.7, -- low quality
    medium = 0.4,
    high = 0.2 -- high quality
}

-- Global engine config. Table might be extended at runtime.
-- Most of the variables are used on initialization and runtime changes might require a restart.
-- Do NOT rename any variables in here, as some these are accessed from C++ code.
engine_cfg = {
    ['system'] = {
        ['enable_editor'] = false, -- Enable the editor.
        ['enable_debug'] = true, -- Enable debugdraw mode.
        ['enable_jit'] = true, -- Enable Just-In-time compilation.
        ['enable_jit_disasm'] = false, -- Enable JIT assembly dump to jit.log output file.
        ['enable_fs_validation'] = true, -- Enable filesystem validation.
        ['auto_gc_time_stepping'] = false, -- Enable smart garbage collection stepping based on framerate.
        ['auto_gc_time_stepping_step_limit'] = 0.01, -- Variable number of seconds to reserve for other things that won’t be caught in diff (experimented with everywhere from 0.002 to 0.01).
        ['target_fps'] = 0, -- Target framerate. Set to 0 to set to the display refresh rate.
    },
    ['cpu'] = {
        -- Soliton gives 1/4 of the CPU threads to rendering, 1/4 to physics and 1/4 to simulation (ECS), if partition_denominator == 4 for example.
        -- The rest is used for audio and scene loading threads and for other software running on the system.
        -- The total amount of threads is capped at MAX_TOTAL_ENGINE_THREADS (if ~= 0)

        ['auto_thread_partitioning'] = true, -- Automatically partition the engine thread count.
        ['max_engine_threads'] = 16, -- Maximum total engine threads for ecs, rendering and physics. (Except audio and scene loading threads, which are created on demand). Set to 0 to use the number of logical cores.
        ['partition_denominator'] = 4, -- The ratio of threads to use for rendering, physics and ECS. The rest will be used for audio and scene loading threads.
        ['max_render_threads'] = 8, -- Maximum render threads.
        ['max_physics_threads'] = 8, -- Maximum physics threads.
        ['max_sim_threads'] = 8, -- Maximum simulation threads.
        ['render_threads'] = 8, -- Number of render threads.
        ['physics_threads'] = 8, -- Number of physics threads.
        ['simulation_threads'] = 8, -- Number of simulation threads.
    },
    ['window'] = {
        ['default_width'] = 1920, -- Default window width.
        ['default_height'] = 1080, -- Default window height.
        ['min_width'] = 640, -- Minimum window width.
        ['min_height'] = 480, -- Minimum window height.
        ['icon'] = '/RES/icons/logo.png' -- window icon image file.
    },
    ['renderer'] = {
        ['shader_dir'] = 'engine_assets/shaders/src', -- Shader directory.
        ['enable_parallel_shader_compilation'] = true, -- Enable shader compilation using multiple threads.
        ['enable_vulkan_validation_layers'] = true, -- Enable Vulkan validation layers, if available.
        ['max_debug_draw_vertices'] = 0x20000, -- Maximum amount of debugdraw draw vertices.
        ['error_texture'] = '/RES/textures/system/error.png', -- Fallback texture when a texture is not found.
        ['fallback_texture_w'] = '/RES/textures/system/fallback_white.png',
        ['fallback_texture_b'] = '/RES/textures/system/fallback_black.png',
        ['brdf_lut_dim'] = 512, -- Width and height of the PBR BRDF integration LUT texture: LUT x LUT.
        ['irradiance_cube_size'] = 64,
        ['prefiltered_cube_size'] = 512,
        ['prefiltered_cube_samples'] = 32,
        ['concurrent_frames'] = 3,
        ['force_vsync'] = false, -- Enable vertical synchronization.
        ['parallel_pipeline_creation'] = false
    },
    ['ui'] = { -- In-Game UI settings.
        ['render_flags'] = ui_render_flags.lcd + ui_render_flags.flip_y + ui_render_flags.ppaa, -- In-Game UI default render flags.
        ['tesselation_error'] = ui_tesselation_error.high, -- In-Game UI tesselation pixel error quality.
        ['license'] = {
            ['id'] = 'neo',
            ['key'] = 'Hayg7oXhoO5LHKuI/YPxXOK/Ghu5Mosoic3bbRrVZQmc/ovw'
        },
        ['xaml_root_path'] = 'assets/ui',
        ['font_root_path'] = 'assets/ui',
        ['texture_root_path'] = 'assets/ui',
        ['default_font'] = {
            ['family'] = 'Fonts/#PT Root UI',
            ['size'] = 15.0,
            ['weight'] = 400,
            ['stretch'] = 5
        }
    },
    ['physics'] = {
        ['temp_allocator_buffer_size'] = 64 * (1024 ^ 2), -- 64 MB
        ['max_rigid_bodies'] = 2 ^ 16, -- Max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
        ['mutex_count'] = 0, -- how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.

        -- This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
        -- body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
        -- too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
        ['max_body_pairs'] = 2 ^ 16,

        -- This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
        -- number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
        ['max_contacts'] = 2 ^ 16,
    },
    ['audio'] = {
        ['max_channels'] = 1024
    },
    ['editor'] = { -- Editor UI settings.
        ['font_size'] = 18, -- Font size
        ['style'] = {
            windowPadding            = { x = 8.00, y = 8.00 },
            framePadding             = { x = 5.00, y = 2.00 },
            cellPadding              = { x = 6.00, y = 6.00 },
            itemSpacing              = { x = 6.00, y = 6.00 },
            itemInnerSpacing         = { x = 6.00, y = 6.00 },
            touchExtraPadding        = { x = 0.00, y = 0.00 },
            indentSpacing            = 25,
            scrollbarSize            = 15,
            grabMinSize              = 10,
            windowBorderSize         = 1,
            childBorderSize          = 1,
            popupBorderSize          = 1,
            frameBorderSize          = 1,
            tabBorderSize            = 1,
            logSliderDeadzone        = 4,
            windowRounding           = 9,
            childRounding            = 9,
            frameRounding            = 9,
            popupRounding            = 9,
            scrollbarRounding        = 9,
            grabRounding             = 9,
            tabRounding              = 9,
            text                     = { a = 1.00, g = 1.00, b = 1.00, r = 1.00 },
            textDisabled             = { a = 0.50, g = 0.50, b = 0.50, r = 1.00 },
            windowBg                 = { a = 0.10, g = 0.10, b = 0.10, r = 1.00 },
            childBg                  = { a = 0.00, g = 0.00, b = 0.00, r = 0.00 },
            popupBg                  = { a = 0.19, g = 0.19, b = 0.19, r = 1.00 },
            border                   = { a = 0.28, g = 0.28, b = 0.28, r = 0.80 },
            borderShadow             = { a = 0.92, g = 0.91, b = 0.88, r = 0.00 },
            frameBg                  = { a = 0.05, g = 0.05, b = 0.05, r = 0.54 },
            frameBgHovered           = { a = 0.19, g = 0.19, b = 0.19, r = 0.54 },
            frameBgActive            = { a = 0.20, g = 0.22, b = 0.23, r = 1.00 },
            titleBg                  = { a = 0.00, g = 0.00, b = 0.00, r = 1.00 },
            titleBgActive            = { a = 0.06, g = 0.06, b = 0.06, r = 1.00 },
            titleBgCollapsed         = { a = 0.00, g = 0.00, b = 0.00, r = 1.00 },
            menuBarBg                = { a = 0.14, g = 0.14, b = 0.14, r = 1.00 },
            scrollbarBg              = { a = 0.05, g = 0.05, b = 0.05, r = 0.54 },
            scrollbarGrab            = { a = 0.34, g = 0.34, b = 0.34, r = 0.54 },
            scrollbarGrabHovered     = { a = 0.40, g = 0.40, b = 0.40, r = 0.54 },
            scrollbarGrabActive      = { a = 0.56, g = 0.56, b = 0.56, r = 0.54 },
            checkMark                = { a = 0.33, g = 0.67, b = 0.86, r = 1.00 },
            sliderGrab               = { a = 0.34, g = 0.34, b = 0.34, r = 0.54 },
            sliderGrabActive         = { a = 0.56, g = 0.56, b = 0.56, r = 0.54 },
            button                   = { a = 0.05, g = 0.05, b = 0.05, r = 0.54 },
            buttonHovered            = { a = 0.19, g = 0.19, b = 0.19, r = 0.54 },
            buttonActive             = { a = 0.20, g = 0.22, b = 0.23, r = 1.00 },
            header                   = { a = 0.00, g = 0.00, b = 0.00, r = 0.52 },
            headerHovered            = { a = 0.00, g = 0.00, b = 0.00, r = 0.36 },
            headerActive             = { a = 0.20, g = 0.22, b = 0.23, r = 0.33 },
            separator                = { a = 0.28, g = 0.28, b = 0.28, r = 0.29 },
            separatorHovered         = { a = 0.44, g = 0.44, b = 0.44, r = 0.29 },
            separatorActive          = { a = 0.40, g = 0.44, b = 0.47, r = 1.00 },
            resizeGrip               = { a = 0.28, g = 0.28, b = 0.28, r = 0.29 },
            resizeGripHovered        = { a = 0.44, g = 0.44, b = 0.44, r = 0.29 },
            resizeGripActive         = { a = 0.40, g = 0.44, b = 0.47, r = 1.00 },
            tab                      = { a = 0.00, g = 0.00, b = 0.00, r = 0.52 },
            tabHovered               = { a = 0.14, g = 0.14, b = 0.14, r = 1.00 },
            tabActive                = { a = 0.20, g = 0.20, b = 0.20, r = 0.36 },
            tabUnfocused             = { a = 0.00, g = 0.00, b = 0.00, r = 0.52 },
            tabUnfocusedActive       = { a = 0.14, g = 0.14, b = 0.14, r = 1.00 },
            dockingPreview           = { a = 0.33, g = 0.67, b = 0.86, r = 1.00 },
            dockingEmptyBg           = { a = 1.00, g = 0.00, b = 0.00, r = 1.00 },
            plotLines                = { a = 0.33, g = 0.67, b = 0.86, r = 1.00 },
            plotLinesHovered         = { a = 1.00, g = 0.00, b = 0.00, r = 1.00 },
            plotHistogram            = { a = 0.33, g = 0.67, b = 0.86, r = 1.00 },
            plotHistogramHovered     = { a = 1.00, g = 0.00, b = 0.00, r = 1.00 },
            tableHeaderBg            = { a = 0.00, g = 0.00, b = 0.00, r = 0.52 },
            tableBorderStrong        = { a = 0.00, g = 0.00, b = 0.00, r = 0.52 },
            tableBorderLight         = { a = 0.28, g = 0.28, b = 0.28, r = 0.29 },
            tableRowBg               = { a = 0.00, g = 0.00, b = 0.00, r = 0.00 },
            tableRowBgAlt            = { a = 1.00, g = 1.00, b = 1.00, r = 0.06 },
            textSelectedBg           = { a = 0.20, g = 0.22, b = 0.23, r = 1.00 },
            dragDropTarget           = { a = 0.33, g = 0.67, b = 0.86, r = 1.00 },
            navHighlight             = { a = 1.00, g = 0.00, b = 0.00, r = 1.00 },
            navWindowingHighlight    = { a = 1.00, g = 0.00, b = 0.00, r = 0.70 },
            navWindowingDimBg        = { a = 1.00, g = 0.00, b = 0.00, r = 0.20 },
            modalWindowDimBg         = { a = 0.35, g = 0.35, b = 0.35, r = 0.80 },
        },
    }
}

local function clamp(x, min_x, max_x)
    return min(max(x, min_x), max_x)
end

local function adjust_config_for_local_machine()
    if jit.os == 'OSX' then
        engine_cfg['system']['enable_jit'] = false
        engine_cfg['editor']['font_size'] = 2 * engine_cfg['editor']['font_size']
    end

    if engine_cfg['cpu']['auto_thread_partitioning'] then
        local cpu = engine_cfg['cpu']
        local cpus = max(1, ffi.C.__lu_app_host_get_num_cpus())
        print(string.format('Detected %d logical cores.', cpus))
        if cpu['max_engine_threads'] > 0 then
            cpus = min(cpus, cpu['max_engine_threads'])
        end
        local ratio = max(1, cpu['partition_denominator'])
        cpu['render_threads'] = clamp(ceil(cpus / ratio), 1, cpu['max_render_threads'])
        cpu['physics_threads'] = clamp(ceil(cpus / ratio), 1, cpu['max_physics_threads'])
        cpu['simulation_threads'] = clamp(ceil(cpus / ratio), 1, cpu['max_sim_threads'])
        print(string.format(
            'Auto-partitioned engine thread count: Total %d threads, %d render threads, %d physics threads, %d ECS threads.',
            cpus, cpu['render_threads'], cpu['physics_threads'], cpu['simulation_threads']
        ))
    end
end

adjust_config_for_local_machine() -- Adjust some engine config values for the local machine the engine is running on
