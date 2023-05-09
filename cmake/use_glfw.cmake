cmake_minimum_required(VERSION 3.16)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG        master
)

FetchContent_MakeAvailable(glfw)