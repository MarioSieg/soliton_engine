add_subdirectory(src/extern/spdlog)
include_directories(src/extern/spdlog/include)

add_subdirectory(src/extern/mimalloc)
include_directories(src/extern/mimalloc/include)
target_link_libraries(lunam mimalloc-static)

add_subdirectory(src/extern/infoware)
include_directories(src/extern/infoware/include)
target_link_libraries(lunam infoware)

add_subdirectory(src/extern/glfw)
include_directories(extern/glfw/include)
target_link_libraries(lunam glfw)

add_subdirectory(src/extern/Boxer)
include_directories(src/extern/boxer/include)
target_link_libraries(lunam Boxer)

add_subdirectory(src/extern/bgfx.cmake)
include_directories(src/extern/bgfx.cmake/bgfx/include)
target_link_libraries(lunam bgfx bx bimg)

add_subdirectory(src/extern/freetype)
include_directories(src/extern/freetype/include)
target_link_libraries(lunam freetype)

if (WIN32)
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/win_amd64/lua51.lib)
else()
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/linux_amd64/lua51.a)
endif()
