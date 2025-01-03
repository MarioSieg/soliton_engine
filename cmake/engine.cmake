if (APPLE)
    file(GLOB_RECURSE SOURCES
        src/*.cpp
        src/*.mm
        src/*.c
        src/*.hpp
    )
else()
    file(GLOB_RECURSE SOURCES
        src/*.cpp
        src/*.c
        src/*.hpp
    )
endif()

if (WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "WinRes.rc")
else()
    set(APP_ICON_RESOURCE_WINDOWS "")
endif()

if (APPLE)
    add_executable(soliton_engine ${SOURCES})
else ()
    add_executable(soliton_engine WIN32 ${SOURCES} ${APP_ICON_RESOURCE_WINDOWS})
endif ()
