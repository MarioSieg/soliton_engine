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
    #set(MACOSX_BUNDLE_ICON_FILE logo.icns)
    #set(MACOSX_BUNDLE_BUNDLE_NAME "Soliton Engine")
    #set(APP_ICON_MACOSX assets/icons/logo.icns)
    #set_source_files_properties(${APP_ICON_MACOSX} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    #add_executable(soliton_engine MACOSX_BUNDLE ${SOURCES} ${APP_ICON_MACOSX})
    add_executable(soliton_engine ${SOURCES})
else ()
    add_executable(soliton_engine WIN32 ${SOURCES} ${APP_ICON_RESOURCE_WINDOWS})
endif ()
