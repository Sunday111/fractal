
cmake_minimum_required(VERSION 3.16)

include(FetchContent)

FetchContent_Declare(
  eigen
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen
  GIT_TAG        master
)

FetchContent_MakeAvailable(eigen)