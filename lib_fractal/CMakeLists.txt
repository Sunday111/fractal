cmake_minimum_required(VERSION 3.16)

include(generic_compile_options)

set(CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/code)
file(GLOB_RECURSE target_headers ${CODE_DIR}/*.hpp)
file(GLOB_RECURSE target_sources ${CODE_DIR}/*.cpp)

set(target_name lib_fractal)
add_library(${target_name} ${target_sources} ${target_headers})
target_include_directories(${target_name} PUBLIC ${CODE_DIR}/include)
target_include_directories(${target_name} PRIVATE ${CODE_DIR})

set_generic_compile_options(${target_name} PUBLIC)