cmake_minimum_required(VERSION 3.10)

project(lunam LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(src/CMakeLists.txt)
include(extern/CMakeLists.txt)
