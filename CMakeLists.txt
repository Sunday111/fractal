cmake_minimum_required(VERSION 3.16)

project(Fractals)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchContent)

add_subdirectory(klgl)
add_subdirectory(fractal)
