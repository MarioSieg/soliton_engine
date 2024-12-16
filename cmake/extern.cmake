if (WIN32)
    target_link_libraries(soliton_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/amd64/windows/libluajit.lib)
elseif(APPLE)
    target_link_libraries(soliton_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm64/osx/libluajit.a)
    target_link_libraries(soliton_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm64/osx/libfmod.dylib)
    target_link_libraries(soliton_engine /Users/mariosieg/VulkanSDK/1.3.296.0/macOS/lib/libvulkan.1.dylib)
    target_include_directories(soliton_engine PRIVATE extern/fmod/osx/api/core/inc)
else()
    target_link_libraries(soliton_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/amd64/linux/libluajit.a)
    target_link_libraries(soliton_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/amd64/linux/libfmod.so)
    target_include_directories(soliton_engine PRIVATE extern/fmod/linux/api/core/inc)
    target_link_libraries(soliton_engine tbb)
endif()

target_include_directories(soliton_engine PRIVATE extern/eabase/include/Common)

add_subdirectory(extern/eaassert)
target_include_directories(soliton_engine PRIVATE extern/eaassert/include)
target_link_libraries(soliton_engine EAAssert)

add_subdirectory(extern/eathread)
target_include_directories(soliton_engine PRIVATE extern/eathread/include)
target_link_libraries(soliton_engine EAThread)

add_subdirectory(extern/eastdc)
target_include_directories(soliton_engine PRIVATE extern/eastdc/include)
target_link_libraries(soliton_engine EAStdC)

add_subdirectory(extern/eastl)
target_include_directories(soliton_engine PRIVATE extern/eastl/include)
target_link_libraries(soliton_engine EASTL)

add_subdirectory(extern/spdlog)
target_include_directories(soliton_engine PRIVATE extern/spdlog/include)
target_link_libraries(soliton_engine spdlog::spdlog_header_only)

add_subdirectory(extern/Boxer)
target_include_directories(soliton_engine PRIVATE extern/Boxer/include)
target_link_libraries(soliton_engine Boxer)

add_subdirectory(extern/glfw)
target_include_directories(soliton_engine PRIVATE extern/glfw/include)
target_link_libraries(soliton_engine glfw)

add_subdirectory(extern/freetype)
target_include_directories(soliton_engine PRIVATE extern/freetype/include)
target_link_libraries(soliton_engine freetype)

add_subdirectory(extern/mimalloc)
target_include_directories(soliton_engine PRIVATE extern/mimalloc/include)
if (WIN32)
    target_link_libraries(soliton_engine mimalloc-static)
else()
    target_link_libraries(soliton_engine mimalloc)
endif()

add_subdirectory(extern/infoware)
target_include_directories(soliton_engine PRIVATE extern/infoware/include)
target_link_libraries(soliton_engine infoware)

add_subdirectory(extern/unordered_dense)
target_include_directories(soliton_engine PRIVATE extern/unordered_dense/include)
target_link_libraries(soliton_engine unordered_dense)

add_subdirectory(extern/flecs)
target_include_directories(soliton_engine PRIVATE extern/flecs/include)
target_link_libraries(soliton_engine flecs_static)

add_subdirectory(extern/DirectXMath)
target_include_directories(soliton_engine PRIVATE extern/DirectXMath/Inc)

if (NOT WIN32)
    target_include_directories(soliton_engine PRIVATE extern/salieri)
endif()

target_include_directories(soliton_engine PRIVATE extern/stb)

add_subdirectory(extern/Vulkan-Headers)
target_include_directories(soliton_engine PRIVATE extern/Vulkan-Headers/include)
add_subdirectory(extern/SPIRV-Headers)
add_subdirectory(extern/SPIRV-Tools)
add_subdirectory(extern/glslang)
add_subdirectory(extern/shaderc)
target_include_directories(soliton_engine PRIVATE extern/shaderc/libshaderc/include)
target_include_directories(soliton_engine PRIVATE extern/shaderc/libshaderc_util/include)
if (NOT APPLE)
    target_link_libraries(soliton_engine vulkan)
endif()
target_link_libraries(soliton_engine shaderc)

add_subdirectory(extern/bgfx)
target_include_directories(soliton_engine PRIVATE extern/bgfx/include)
target_link_libraries(soliton_engine bx bimg bimg_decode) # we only borrow bgfx's bx and bimg

add_compile_definitions(JPH_DEBUG_RENDERER=1)
add_subdirectory(extern/JoltPhysics/Build)
target_include_directories(soliton_engine PRIVATE extern/JoltPhysics/Source)
target_link_libraries(soliton_engine Jolt)

target_include_directories(soliton_engine PRIVATE extern/libs)

add_subdirectory(extern/nativefiledialog-extended)
target_include_directories(soliton_engine PRIVATE extern/nativefiledialog-extended/src/include)
target_link_libraries(soliton_engine nfd)

add_subdirectory(extern/Vulkan-Utility-Libraries)
target_include_directories(soliton_engine PRIVATE extern/Vulkan-Utility-Libraries/include)

target_include_directories(soliton_engine PRIVATE extern/VulkanMemoryAllocator/include)

target_include_directories(soliton_engine PRIVATE extern/noesis/Include)
target_include_directories(soliton_engine PRIVATE src/graphics/noesis/Providers/Include)
target_include_directories(soliton_engine PRIVATE src/graphics/noesis/Interactivity/Include)
target_include_directories(soliton_engine PRIVATE src/graphics/noesis/Theme/Include)
target_include_directories(soliton_engine PRIVATE src/graphics/noesis/MediaElement/Include)
target_include_directories(soliton_engine PRIVATE src/graphics/noesis/VKRenderDevice/Include)
target_include_directories(soliton_engine PRIVATE src/graphics/noesis/App/Include)

add_subdirectory(extern/glm)
target_include_directories(soliton_engine PRIVATE extern/glm)

add_subdirectory(extern/simdutf)
target_include_directories(soliton_engine PRIVATE extern/simdutf/include)
target_link_libraries(soliton_engine simdutf)

target_include_directories(soliton_engine PRIVATE extern/mINI/src)

add_subdirectory(extern/LuaBridge3)
target_include_directories(soliton_engine PRIVATE extern/LuaBridge3/Source)

target_include_directories(soliton_engine PRIVATE extern/lunam_jit/src)

add_subdirectory(extern/stduuid)
target_include_directories(soliton_engine PRIVATE extern/stduuid/include)
target_link_libraries(soliton_engine stduuid)

add_subdirectory(extern/luv)
target_include_directories(soliton_engine PRIVATE extern/luv/src)
target_link_libraries(soliton_engine libluv_a)

add_subdirectory(extern/SPIRV-Reflect)
target_include_directories(soliton_engine PRIVATE extern/SPIRV-Reflect/include)
target_link_libraries(soliton_engine spirv-reflect-static)

add_subdirectory(extern/simpleini)
target_include_directories(soliton_engine PRIVATE extern/simpleini)
target_link_libraries(soliton_engine SimpleIni)

add_subdirectory(extern/efsw)
target_include_directories(soliton_engine PRIVATE extern/efsw/include)
target_link_libraries(soliton_engine efsw-static)

add_subdirectory(extern/regex)
target_include_directories(soliton_engine PRIVATE extern/regex/include)
target_link_libraries(soliton_engine boost_regex)

target_include_directories(soliton_engine PRIVATE src/graphics/imgui/imgui_collections)
target_include_directories(soliton_engine PRIVATE src/graphics/imgui/imgui_collections/imgui)
target_include_directories(soliton_engine PRIVATE src/graphics/imgui/imgui_collections/cimgui)
target_include_directories(soliton_engine PRIVATE src/graphics/imgui/imgui_collections/implot)
target_include_directories(soliton_engine PRIVATE src/graphics/imgui/imgui_collections/cimplot)
target_include_directories(soliton_engine PRIVATE src/graphics/imgui/imgui_collections/imtexteditor)

add_subdirectory(extern/fast_float)
target_include_directories(soliton_engine PRIVATE extern/fast_float/include)

add_subdirectory(extern/utfcpp)
target_include_directories(soliton_engine PRIVATE extern/utfcpp/source)

add_subdirectory(extern/nameof)
target_include_directories(soliton_engine PRIVATE extern/nameof/include)

add_subdirectory(extern/cereal)
target_include_directories(soliton_engine PRIVATE extern/cereal/include)

add_subdirectory(extern/lz4/build/cmake)
target_include_directories(soliton_engine PRIVATE extern/lz4/lib)
target_link_libraries(soliton_engine lz4)

# Allow to include GLSL common headers
target_include_directories(soliton_engine PRIVATE engine_assets/shaders2)

add_subdirectory(extern/json)
target_include_directories(soliton_engine PRIVATE extern/json/single_include/nlohmann)

add_subdirectory(extern/tinygltf)
target_include_directories(soliton_engine PRIVATE extern/tinygltf)

add_subdirectory(extern/inipp)
target_include_directories(soliton_engine PRIVATE extern/inipp)


##################################################################################################
# Libraries, which requires C++ exceptions and have no way to disable them
##################################################################################################

add_subdirectory(extern/Simd/prj/cmake)
target_include_directories(soliton_engine PRIVATE extern/Simd/src)
target_link_libraries(soliton_engine Simd)
if (NOT WIN32)
    target_compile_options(Simd PRIVATE -fexceptions) # Simd uses exceptions
endif()

add_subdirectory(extern/assimp)
target_include_directories(soliton_engine PRIVATE extern/assimp/include)
target_link_libraries(soliton_engine assimp)
if (NOT WIN32)
    target_compile_options(assimp PRIVATE -fexceptions) # Assimp uses exceptions
endif()

add_subdirectory(extern/rttr)
target_include_directories(soliton_engine PRIVATE extern/rttr/src)
target_link_libraries(soliton_engine rttr_core)
if (NOT WIN32)
    target_compile_options(rttr_core PRIVATE -fexceptions) # rttr uses exceptions
endif()
