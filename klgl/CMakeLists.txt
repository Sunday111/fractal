cmake_minimum_required(VERSION 3.16)

include(generic_compile_options)
include(FetchContent)
include(use_fmtlib)
include(use_glad)
include(use_glfw)
include(use_eigen)
include(use_cppreflection)
include(use_imgui)
include(use_nlohmann)

set(CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/klgl)
file(GLOB_RECURSE target_headers ${CODE_DIR}/*.hpp)
file(GLOB_RECURSE target_sources ${CODE_DIR}/*.cpp)

set(target_name klgl)
add_library(${target_name} ${target_sources} ${target_headers})
set_generic_compile_options(${target_name} PUBLIC)
target_include_directories(${target_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${target_name} PUBLIC
    fmt::fmt
    glad
    glfw
    eigen
    imgui
    CppReflection
    nlohmann_json::nlohmann_json)
