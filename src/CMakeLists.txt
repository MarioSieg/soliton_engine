file(GLOB_RECURSE SOURCES
    src/boot/*.cpp
    src/boot/*.c
    src/boot/*.hpp

    src/assetmgr/*.cpp
    src/assetmgr/*.c
    src/assetmgr/*.hpp

    src/core/*.cpp
    src/core/*.c
    src/core/*.hpp

    src/dense/*.cpp
    src/dense/*.c
    src/dense/*.hpp

    src/ecs/*.cpp
    src/ecs/*.c
    src/ecs/*.hpp

    src/graphics/*.cpp
    src/graphics/*.c
    src/graphics/*.hpp

    src/math/*.cpp
    src/math/*.c
    src/math/*.hpp

    src/platform/*.cpp
    src/platform/*.c
    src/platform/*.hpp

    src/scene/*.cpp
    src/scene/*.c
    src/scene/*.hpp

    src/scripting/*.cpp
    src/scripting/*.c
    src/scripting/*.hpp

    src/stb/*.cpp
    src/stb/*.c
    src/stb/*.hpp

    src/log/*.cpp
    src/log/*.c
    src/log/*.hpp

    src/vulkancore/*.cpp
    src/vulkancore/*.c
    src/vulkancore/*.hpp
)

add_compile_definitions(SPDLOG_COMPILED_LIB)

include_directories(src/log/include)

if (WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "WinRes.rc")
else()
    set(APP_ICON_RESOURCE_WINDOWS "")
endif()

add_executable(lunam WIN32 ${SOURCES} ${APP_ICON_RESOURCE_WINDOWS})

add_subdirectory(src/msgbox)
include_directories(src/msgbox/include)
target_link_libraries(lunam Boxer)

include_directories(src/sal)

add_subdirectory(src/glfw)
include_directories(src/glfw/include)
target_link_libraries(lunam glfw)

add_subdirectory(src/freetype)
include_directories(src/freetype/include)
target_link_libraries(lunam freetype)

add_subdirectory(src/allocator)
include_directories(src/allocator/include)
target_link_libraries(lunam mimalloc-static)

add_subdirectory(src/machineinfo)
include_directories(src/machineinfo/include)
target_link_libraries(lunam infoware)

add_subdirectory(src/meshio)
include_directories(src/meshio/include)
target_link_libraries(lunam assimp)

add_subdirectory(src/vulkanloader)
target_link_libraries(lunam vulkan)

add_subdirectory(src/spirvheaders)
add_subdirectory(src/spirvtools)
add_subdirectory(src/glslang)
add_subdirectory(src/shaderc)
include_directories(src/shaderc/libshaderc/include)
target_link_libraries(lunam shaderc)

if (WIN32)
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/win_amd64/lua51.lib)
else()
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/linux_amd64/lua51.a)
    target_link_libraries(lunam tbb)
endif()
