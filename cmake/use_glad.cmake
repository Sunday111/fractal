cmake_minimum_required(VERSION 3.16)

include(FetchContent)

FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Sunday111/glad
  GIT_TAG        main
)

FetchContent_GetProperties(glad)
if (NOT glad_POPULATED)
    FetchContent_Populate(glad)
endif()

add_subdirectory(${glad_SOURCE_DIR} ${glad_BINARY_DIR})
