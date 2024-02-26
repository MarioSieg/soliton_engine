file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/*.c
    src/*.hpp
)

if (WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "WinRes.rc")
else()
    set(APP_ICON_RESOURCE_WINDOWS "")
endif()

add_executable(lunam WIN32 ${SOURCES} ${APP_ICON_RESOURCE_WINDOWS})

if (WIN32)
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/win_amd64/lua51.lib)
elseif(APPLE)
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/mac_aarch64/libluajit.a)
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/mac_aarch64/libMoltenVK.dylib)
else()
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/linux_amd64/lua51.a)
    target_link_libraries(lunam tbb)
endif()
