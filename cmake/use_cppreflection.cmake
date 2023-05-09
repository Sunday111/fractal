cmake_minimum_required(VERSION 3.16)

include(FetchContent)
include(use_everydaytools)

FetchContent_Declare(
  cppreflection
  GIT_REPOSITORY https://github.com/Sunday111/CppReflection
  GIT_TAG        master
)

FetchContent_GetProperties(cppreflection)
if (NOT cppreflection_POPULATED)
    FetchContent_Populate(cppreflection)
endif()

add_subdirectory(${cppreflection_SOURCE_DIR} ${cppreflection_BINARY_DIR})