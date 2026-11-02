cmake_minimum_required(VERSION 3.16)
project(Block7 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Use an installed raylib if there is one, otherwise download and build it.
find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(raylib)
endif()

add_executable(block7 src/main.cpp)
target_link_libraries(block7 PRIVATE raylib)

if(MSVC)
  target_compile_options(block7 PRIVATE /W3)
else()
  target_compile_options(block7 PRIVATE -Wall -Wextra)
endif()
