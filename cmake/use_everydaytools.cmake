cmake_minimum_required(VERSION 3.16)

include(FetchContent)

FetchContent_Declare(
  everydaytools
  GIT_REPOSITORY https://github.com/Sunday111/EverydayTools
  GIT_TAG        master
)

FetchContent_GetProperties(everydaytools)
if (NOT everydaytools_POPULATED)
    FetchContent_Populate(everydaytools)
endif()

add_subdirectory(${everydaytools_SOURCE_DIR} ${everydaytools_BINARY_DIR})