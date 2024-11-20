// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "backend.hpp"
#include "../core/core.hpp"

#include <spdlog/fmt/bin_to_hex.h>
#include <GLFW/glfw3.h>
#include <nfd.hpp>

#if USE_MIMALLOC
#   include <mimalloc.h>
#endif

namespace soliton::platform::backend {
    static constinit bool s_is_initialized;
    static constinit bool s_is_nfd_online;

    auto is_online() noexcept -> bool {
        return s_is_initialized;
    }

    auto init() -> void {
        if (s_is_initialized) return;
        log_info("Initializing platform backend");

        glfwSetErrorCallback(+[](const int code, const char* const desc) -> void {
            log_error("Platform backend error: {} ({:#X})", desc, code);
        });

#if USE_MIMALLOC
        static GLFWallocator s_alloc {};
        s_alloc.allocate = +[](const std::size_t size, [[maybe_unused]] void* usr) -> void* {
            return mi_malloc(size);
        };
        s_alloc.deallocate = +[](void* ptr, [[maybe_unused]] void* usr) -> void {
            mi_free(ptr);
        };
        s_alloc.reallocate = +[](void* ptr, const std::size_t size, [[maybe_unused]] void* usr) -> void* {
            return mi_realloc(ptr, size);
        };
        glfwInitAllocator(&s_alloc);
#endif

        const bool glfw_online = glfwInit() == GLFW_TRUE;
        if (!glfw_online) [[unlikely]] {
            const char* desc = nullptr;
            glfwGetError(&desc);
            if (desc) log_error("Failed to initialize GLFW: {}", desc);
            panic_assert(glfw_online);
        }

        if (NFD_Init() != NFD_OKAY) [[unlikely]] {
            log_error("Failed to initialize file dialog system: {}", NFD_GetError());
            s_is_nfd_online = false;
        } else {
            s_is_nfd_online = true;
        }

        // print full monitor info
        int n;
        if (GLFWmonitor** mons = glfwGetMonitors(&n); mons)
            for (int i = 0; i < n; ++i)
                backend::print_monitor_info(mons[i]);

        s_is_initialized = true;
    }

    auto shutdown() -> void {
        if (!s_is_initialized) return;
        NFD_Quit();
        glfwTerminate();
        s_is_initialized = s_is_nfd_online = false;
    }

    auto print_monitor_info(GLFWmonitor* const mon) -> void {
        if (!mon) return;
        if (const char* name = glfwGetMonitorName(mon); name) {
            log_info("Monitor name: {}", name);
        }
        if (const GLFWvidmode* mode = glfwGetVideoMode(mon); mode) {
            log_info("Monitor: {}x{}@{}Hz", mode->width, mode->height, mode->refreshRate);
        }
        int n;
        if (const GLFWvidmode* modes = glfwGetVideoModes(mon, &n); modes) {
            log_info("-------- Available Video Modes --------");
            for (int i = 0; i < n; ++i) {
                const GLFWvidmode& mode = modes[i];
                log_info("    {}x{}@{}Hz", mode.width, mode.height, mode.refreshRate);
            }
        }
        const GLFWgammaramp* const ramp = glfwGetGammaRamp(mon);
        log_info("-------- Gamma Ramp --------");
        eastl::vector<std::uint8_t> gramp {};
        gramp.reserve(ramp->size * 3);
        for (int i = 0; i < ramp->size; ++i) {
            gramp.emplace_back(ramp->red[i]);
            gramp.emplace_back(ramp->green[i]);
            gramp.emplace_back(ramp->blue[i]);
        }
        log_info("{}", spdlog::to_hex(gramp));
    }

    auto poll_events() -> void {
        glfwPollEvents();
    }
}
