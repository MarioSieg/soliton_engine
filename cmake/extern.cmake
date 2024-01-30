add_subdirectory(extern/spdlog)
target_include_directories(lunam PRIVATE extern/spdlog/include)
target_link_libraries(lunam spdlog)

add_subdirectory(extern/Boxer)
include_directories(extern/Boxer/include)
target_link_libraries(lunam Boxer)

add_subdirectory(extern/glfw)
include_directories(extern/glfw/include)
target_link_libraries(lunam glfw)

add_subdirectory(extern/freetype)
include_directories(extern/freetype/include)
target_link_libraries(lunam freetype)

add_subdirectory(extern/mimalloc)
include_directories(extern/mimalloc/include)
target_link_libraries(lunam mimalloc-static)

add_subdirectory(extern/infoware)
include_directories(extern/infoware/include)
target_link_libraries(lunam infoware)

add_subdirectory(extern/unordered_dense)
include_directories(extern/unordered_dense/include)
target_link_libraries(lunam unordered_dense)

add_subdirectory(extern/flecs)
include_directories(extern/flecs/include)
target_link_libraries(lunam flecs_static)

add_subdirectory(extern/tinygltf)
include_directories(extern/tinygltf/include)
target_link_libraries(lunam tinygltf)

add_subdirectory(extern/DirectXMath)
include_directories(extern/DirectXMath/Inc)

include_directories(extern/salieri)
include_directories(extern/stb)

add_subdirectory(extern/draco)
include_directories(extern/draco/src)
target_link_libraries(lunam draco_static)

add_subdirectory(extern/Vulkan-Loader)
add_subdirectory(extern/SPIRV-Headers)
add_subdirectory(extern/SPIRV-Tools)
add_subdirectory(extern/glslang)
add_subdirectory(extern/shaderc)
include_directories(extern/shaderc/libshaderc/include)
target_link_libraries(lunam vulkan)
target_link_libraries(lunam shaderc)
