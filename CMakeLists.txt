cmake_minimum_required(VERSION 3.10)

project(soliton_engine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/compiler_config.cmake)
include(cmake/engine.cmake)
include(cmake/extern.cmake)
