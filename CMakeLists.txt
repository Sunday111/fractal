cmake_minimum_required(VERSION 3.16)

project(Fractals)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchContent)
include(generic_compile_options)

set(CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/code)
file(GLOB_RECURSE target_headers ${CODE_DIR}/*.hpp)
file(GLOB_RECURSE target_sources ${CODE_DIR}/*.cpp)

set(target_name ass)
add_executable(${target_name} ${target_sources} ${target_headers})
target_include_directories(${target_name} PUBLIC ${CODE_DIR})

set_generic_compile_options(${target_name} PUBLIC)

FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG        4.5.0
)

FetchContent_MakeAvailable(raylib)

target_link_libraries(${target_name} PUBLIC raylib)
