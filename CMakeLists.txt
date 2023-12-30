cmake_minimum_required(VERSION 3.10)

project(lunam LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-msse2 -msse3 -mssse3 -msse4.1 -msse4.2)
if (WIN32)
    add_compile_options(-Xclang -fvisibility=hidden -Wno-defaulted-function-deleted)
else()
    add_compile_options(-fvisibility=hidden)
    add_link_options(-rdynamic)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_link_options("-Wl,--thinlto-cache-dir=${PROJECT_BINARY_DIR}/LTO.cache")
    add_link_options("-Wl,--thinlto-jobs=64")
    add_compile_options(-Ofast -flto=thin)
    add_link_options(-flto=thin)
endif()

include(src/CMakeLists.txt)
