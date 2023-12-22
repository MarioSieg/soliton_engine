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
