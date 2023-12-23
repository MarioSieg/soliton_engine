add_subdirectory(extern/spdlog)
include_directories(extern/spdlog/include)

add_subdirectory(extern/mimalloc)
include_directories(extern/mimalloc/include)
target_link_libraries(lunam mimalloc-static)

add_subdirectory(extern/infoware)
include_directories(extern/infoware/include)
target_link_libraries(lunam infoware)

add_subdirectory(extern/glfw)
include_directories(extern/glfw/include)
target_link_libraries(lunam glfw)

add_subdirectory(extern/boxer)
include_directories(extern/boxer/include)
target_link_libraries(lunam Boxer)

include_directories(extern/stb)

add_subdirectory(extern/bgfx.cmake)
include_directories(extern/bgfx.cmake/bgfx/include)
target_link_libraries(lunam bgfx bx bimg)

add_subdirectory(extern/imgui)
include_directories(extern/imgui)
target_link_libraries(lunam imgui)

add_subdirectory(extern/implot)
include_directories(extern/implot)
target_link_libraries(lunam implot)

include_directories(extern/LuaJIT/src)
target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/extern/__bin/win_amd64/lua51.lib)

include_directories(extern/luafilesystem/src)
include_directories(extern/LuaBridge3/Source)
