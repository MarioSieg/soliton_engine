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

if (WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "WinRes.rc")
else()
    set(APP_ICON_RESOURCE_WINDOWS "")
endif()

add_executable(lunam WIN32 ${SOURCES} ${APP_ICON_RESOURCE_WINDOWS})

if (WIN32)
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/win_amd64/lua51.lib)
else()
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/linux_amd64/lua51.a)
    target_link_libraries(lunam tbb)
endif()
