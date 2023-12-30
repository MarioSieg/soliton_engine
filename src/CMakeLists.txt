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
