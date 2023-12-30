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

add_subdirectory(extern/Boxer)
include_directories(extern/boxer/include)
target_link_libraries(lunam Boxer)

include_directories(extern/stb)

add_subdirectory(extern/bgfx.cmake)
include_directories(extern/bgfx.cmake/bgfx/include)
target_link_libraries(lunam bgfx bx bimg)

add_subdirectory(extern/freetype)
include_directories(extern/freetype/include)
target_link_libraries(lunam freetype)

include_directories(extern/LuaJIT/src)
if (WIN32)
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/extern/__bin/win_amd64/lua51.lib)
else()
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/extern/__bin/linux_amd64/lua51.a)
endif()

include_directories(extern/luafilesystem/src)
include_directories(extern/LuaBridge3/Source)

add_subdirectory(extern/flecs)
include_directories(extern/flecs/include)
target_link_libraries(lunam flecs_static)

include_directories(extern/sal)

add_subdirectory(extern/DirectXMath)
include_directories(extern/DirectXMath/Inc)
include_directories(extern/DirectXMath/Extensions)

add_subdirectory(extern/unordered_dense)
include_directories(extern/unordered_dense/include/ankerl)
target_link_directories(lunam PRIVATE unordered_dense)
