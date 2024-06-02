if (WIN32)
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/win_amd64/libluajit.lib)
elseif(APPLE)
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/mac_aarch64/libluajit.a)
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/mac_aarch64/libMoltenVK.dylib)
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/mac_aarch64/Noesis.dylib)
else()
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/linux_amd64/libluajit.a)
    target_link_libraries(lunam ${CMAKE_CURRENT_SOURCE_DIR}/lib/linux_amd64/libNoesis.so)
    target_link_libraries(lunam tbb)
endif()

add_subdirectory(extern/spdlog)
target_include_directories(lunam PRIVATE extern/spdlog/include)
target_link_libraries(lunam spdlog::spdlog_header_only)

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
if (WIN32)
    target_link_libraries(lunam mimalloc-static)
else()
    target_link_libraries(lunam mimalloc)
endif()

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

if (NOT WIN32)
    target_include_directories(lunam PRIVATE extern/salieri)
endif()

target_include_directories(lunam PRIVATE extern/stb)

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

add_subdirectory(extern/bgfx)
target_include_directories(lunam PRIVATE extern/bgfx/include)
target_link_libraries(lunam bx bimg bimg_decode) # we only borrow bgfx's bx and bimg

add_compile_definitions(JPH_DEBUG_RENDERER=1)
add_subdirectory(extern/JoltPhysics/Build)
target_include_directories(lunam PRIVATE extern/JoltPhysics/Source)
target_link_libraries(lunam Jolt)

target_include_directories(lunam PRIVATE extern/libs)

add_subdirectory(extern/nativefiledialog-extended)
target_include_directories(lunam PRIVATE extern/nativefiledialog-extended/src/include)
target_link_libraries(lunam nfd)

add_subdirectory(extern/Vulkan-Utility-Libraries)
target_include_directories(lunam PRIVATE extern/Vulkan-Utility-Libraries/include)

target_include_directories(lunam PRIVATE extern/VulkanMemoryAllocator/include)

target_include_directories(lunam PRIVATE extern/noesis/Include)
target_include_directories(lunam PRIVATE src/graphics/noesis/Providers/Include)
target_include_directories(lunam PRIVATE src/graphics/noesis/Interactivity/Include)
target_include_directories(lunam PRIVATE src/graphics/noesis/Theme/Include)
target_include_directories(lunam PRIVATE src/graphics/noesis/MediaElement/Include)
target_include_directories(lunam PRIVATE src/graphics/noesis/VKRenderDevice/Include)
target_include_directories(lunam PRIVATE src/graphics/noesis/App/Include)

add_subdirectory(extern/glm)
target_include_directories(lunam PRIVATE extern/glm)

add_subdirectory(extern/simdutf)
target_include_directories(lunam PRIVATE extern/simdutf/include)
target_link_libraries(lunam simdutf)

target_include_directories(lunam PRIVATE extern/mINI/src)

add_subdirectory(extern/LuaBridge3)
target_include_directories(lunam PRIVATE extern/LuaBridge3/Source)

target_include_directories(lunam PRIVATE extern/lunam_jit/src)

##################################################################################################
# Libraries, which sadly requires C++ exceptions and have no way to disable them
##################################################################################################

add_subdirectory(extern/Simd/prj/cmake)
target_include_directories(lunam PRIVATE extern/Simd/src)
target_link_libraries(lunam Simd)
if (NOT WIN32)
    target_compile_options(Simd PRIVATE -fexceptions) # Simd uses exceptions
endif()

add_subdirectory(extern/assimp)
target_include_directories(lunam PRIVATE extern/assimp/include)
target_link_libraries(lunam assimp)
if (NOT WIN32)
    target_compile_options(assimp PRIVATE -fexceptions) # Assimp uses exceptions
endif()
