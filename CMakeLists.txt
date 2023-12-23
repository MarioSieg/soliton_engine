cmake_minimum_required(VERSION 3.10)

project(lunam LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(/O2 /fp:fast /GL /Gw /Gy /Oi /Ot /Oy)
    add_link_options(/LTCG)
endif()

include(src/CMakeLists.txt)
include(extern/CMakeLists.txt)

target_compile_options(lunam PRIVATE /W3)
