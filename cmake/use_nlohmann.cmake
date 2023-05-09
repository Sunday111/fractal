cmake_minimum_required(VERSION 3.16)

include(FetchContent)

FetchContent_Declare(
  nlohmann
  GIT_REPOSITORY https://github.com/nlohmann/json
  GIT_TAG        develop
)

FetchContent_MakeAvailable(nlohmann)