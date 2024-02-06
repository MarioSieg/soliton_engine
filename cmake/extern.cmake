add_subdirectory(extern/spdlog)
target_include_directories(lunam PRIVATE extern/spdlog/include)
target_link_libraries(lunam spdlog)

add_subdirectory(extern/Boxer)
target_include_directories(lunam PRIVATE extern/Boxer/include)
target_link_libraries(lunam Boxer)

add_subdirectory(extern/glfw)
target_include_directories(lunam PRIVATE extern/glfw/include)
target_link_libraries(lunam glfw)

add_subdirectory(extern/freetype)
target_include_directories(lunam PRIVATE extern/freetype/include)
target_link_libraries(lunam freetype)

add_subdirectory(extern/mimalloc)
target_include_directories(lunam PRIVATE extern/mimalloc/include)
target_link_libraries(lunam mimalloc-static)

add_subdirectory(extern/infoware)
target_include_directories(lunam PRIVATE extern/infoware/include)
target_link_libraries(lunam infoware)

add_subdirectory(extern/unordered_dense)
target_include_directories(lunam PRIVATE extern/unordered_dense/include)
target_link_libraries(lunam unordered_dense)

add_subdirectory(extern/flecs)
target_include_directories(lunam PRIVATE extern/flecs/include)
target_link_libraries(lunam flecs_static)

add_subdirectory(extern/DirectXMath)
target_include_directories(lunam PRIVATE extern/DirectXMath/Inc)

target_include_directories(lunam PRIVATE extern/salieri)
target_include_directories(lunam PRIVATE extern/stb)

add_subdirectory(extern/draco)
target_include_directories(lunam PRIVATE extern/draco/src)
target_link_libraries(lunam draco_static)

add_subdirectory(extern/Vulkan-Headers)
target_include_directories(lunam PRIVATE extern/Vulkan-Headers/include)
add_subdirectory(extern/Vulkan-Loader)
add_subdirectory(extern/SPIRV-Headers)
add_subdirectory(extern/SPIRV-Tools)
add_subdirectory(extern/glslang)
add_subdirectory(extern/shaderc)
target_include_directories(lunam PRIVATE extern/shaderc/libshaderc/include)
target_link_libraries(lunam vulkan)
target_link_libraries(lunam shaderc)
target_link_libraries(lunam vulkan)

add_subdirectory(extern/bgfx)
target_include_directories(lunam PRIVATE extern/bgfx/include)
target_link_libraries(lunam bx bimg bimg_decode) # we only borrow bgfx's bx and bimg

add_subdirectory(extern/JoltPhysics/Build)
target_include_directories(lunam PRIVATE extern/JoltPhysics/Source)
target_link_libraries(lunam Jolt)

target_include_directories(lunam PRIVATE extern/libs)

add_subdirectory(extern/nativefiledialog-extended)
target_include_directories(lunam PRIVATE extern/nativefiledialog-extended/src/include)
target_link_libraries(lunam nfd)

# Assimp must be last
add_subdirectory(extern/assimp)
target_include_directories(lunam PRIVATE extern/assimp/include)
target_link_libraries(lunam assimp)
target_compile_options(assimp PRIVATE -fexceptions -frtti) # fuck u assimp
