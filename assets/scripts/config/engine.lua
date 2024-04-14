-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

-- Default internal engine config, hidden from end user

local jit = require 'jit'
local ffi = require 'ffi'
ffi.cdef[[
    uint32_t __lu_app_host_get_num_cpus(void);
]]

print('Loading internal engine config...')

-- Default engine config:
-- Do NOT rename any variables in here, as some these are accessed from C++ code.
ENGINE_CONFIG = {
    General = {
        enableEditor = true, -- Enable the editor.
        enableDebug = true, -- Enable debug mode.
        enableJit = true, -- Enable Just-In-Time compilation.
        enableJitAssemblyDump = false, -- Enable JIT assembly dump to jit.log output file.
        enableFilesystemValidation = true, -- Enable filesystem validation.
        loadLuaStdlibExtensions = true, -- Load Lua standard library extensions.
        smartFramerateDependentGCStepping = false, -- Enable smart garbage collection stepping based on framerate.
        smartFramerateDependentGCSteppingCollectionLimit = 0.01, -- Variable number of seconds to reserve for other things that won’t be caught in diff (experimented with everywhere from 0.002 to 0.01).
        targetFramerate = 0, -- Target framerate. Set to 0 to set to the display refresh rate.
    },
    Threads = {
        -- Lunam gives 1/4 of the CPU threads to rendering, 1/4 to physics and 1/4 to simulation (ECS).
        -- The rest is used for audio and scene loading threads and for other software running on the system.
        -- The total amount of threads is capped at MAX_TOTAL_ENGINE_THREADS (if ~= 0)

        autoPartitionEngineThreadCount = true, -- Automatically partition the engine thread count.
        maxTotalEngineThreads = 16, -- Maximum total engine threads for ecs, rendering and physics. (Except audio and scene loading threads, which are created on demand). Set to 0 to use the number of logical cores.
        partitionRatio = 4, -- The ratio of threads to use for rendering, physics and ECS. The rest will be used for audio and scene loading threads.
        maxRenderThreads = 8, -- Maximum render threads.
        maxPhysicsThreads = 8, -- Maximum physics threads.
        maxSimThreads = 8, -- Maximum simulation threads.
        renderThreads = 8, -- Number of render threads.
        physicsThreads = 8, -- Number of physics threads.
        simThreads = 8, -- Number of simulation threads.
    },
    Window = {
        defaultWidth = 1920, -- Default window width.
        defaultHeight = 1080, -- Default window height.
        minWidth = 640, -- Minimum window width.
        minHeight = 480, -- Minimum window height.
        icon = 'logo.png' -- Window icon image file.
    },
    Renderer = {
        enableVulkanValidationLayers = true, -- Enable Vulkan validation layers.
        maxDebugDrawVertices = 1000000, -- Maximum amount of debug draw vertices.
        uiFontSize = 18, -- UI font size (ImGui).
    },
    Physics = {
        tempAllocatorBufferSize = 1024 * 1024 * 64, -- 64 MB
        maxRigidBodies = 65535, -- Max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
        numMutexes = 0, -- how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.

        -- This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
        -- body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
        -- too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
        maxBodyPairs = 65535,

        -- This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
        -- number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
        maxContacts = 65535, 
    }
}

if jit.os == 'OSX' then
    print('Detected macOS, increasing UI font size...')
    ENGINE_CONFIG.Renderer.uiFontSize = ENGINE_CONFIG.Renderer.uiFontSize * 2
end

if ENGINE_CONFIG.Threads.autoPartitionEngineThreadCount then
    local function clamp(x, min, max)
        return math.min(math.max(x, min), max)
    end
    local cpus = math.max(1, ffi.C.__lu_app_host_get_num_cpus())
    print('Detected ' .. cpus .. ' logical cores.')
    if ENGINE_CONFIG.Threads.maxTotalEngineThreads > 0 then
        cpus = math.min(cpus, ENGINE_CONFIG.Threads.maxTotalEngineThreads)
    end
    local ratio = math.max(1, ENGINE_CONFIG.Threads.partitionRatio)
    ENGINE_CONFIG.Threads.renderThreads = clamp(math.ceil(cpus / ratio), 1, ENGINE_CONFIG.Threads.maxRenderThreads)
    ENGINE_CONFIG.Threads.physicsThreads = clamp(math.ceil(cpus / ratio), 1, ENGINE_CONFIG.Threads.maxPhysicsThreads)
    ENGINE_CONFIG.Threads.simThreads = clamp(math.ceil(cpus / ratio), 1, ENGINE_CONFIG.Threads.maxSimThreads)
    print('Auto-partitioned engine thread count: ' .. cpus .. ' threads, ' .. ENGINE_CONFIG.Threads.renderThreads .. ' render threads, ' .. ENGINE_CONFIG.Threads.physicsThreads .. ' physics threads, ' .. ENGINE_CONFIG.Threads.simThreads .. ' ECS threads.')
end
