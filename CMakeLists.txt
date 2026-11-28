cmake_minimum_required(VERSION 3.16)
project(Watch-Man LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Use an installed raylib if available, otherwise fetch and build raylib 5.5.
find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(raylib)
endif()

# Everything except main() is compiled ONCE into this library; the game and every test link it.
# (New source files: add them to this list. Nothing else needs to change.)
set(GAME_SOURCES
  src/Game.cpp
  src/Level.cpp        src/LevelData.cpp
  src/Entities.cpp     src/CharacterArt.cpp
  src/Dialogue.cpp     src/DialogueData.cpp
  src/ClueBook.cpp
  src/Audio.cpp        src/AudioData.cpp    src/Synth.cpp
  src/Renderer.cpp
  src/Ui.cpp)

add_library(watchman_core STATIC ${GAME_SOURCES})
target_include_directories(watchman_core PUBLIC src)
find_package(Threads REQUIRED)
target_link_libraries(watchman_core PUBLIC raylib Threads::Threads)

add_executable(Watch-Man src/main.cpp)
target_link_libraries(Watch-Man PRIVATE watchman_core)

if(MSVC)
  target_compile_options(watchman_core PRIVATE /W3)
  target_compile_options(Watch-Man PRIVATE /W3)
else()
  target_compile_options(watchman_core PRIVATE -Wall -Wextra)
  target_compile_options(Watch-Man PRIVATE -Wall -Wextra)
endif()

# Copy assets next to the executable if an assets directory is present.
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
  add_custom_command(TARGET Watch-Man POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:Watch-Man>/assets"
  )
endif()

# ---------------------------------------------------------------------------------------------------------------------
# Headless tests (no window). Build with the game; run all of them with:   ctest --test-dir build --output-on-failure
# Tests that draw write their pictures (.ppm) to  build/test_output/  (open them with an image viewer such as IrfanView or GIMP).
#   logic_test      your original whole-case test (tests/logic_test.cpp)
#   cast_test       every person is placed once on an open cell or behind a real door; every clue id in dialogue exists
#   peep_test       the peephole on every kind of door, looking around, stepping back, random-input fuzz
#   roof_walk_test  stairs to the roof and back, random-input fuzz on the roof, the lift skips the roof
#   mirror_test, sprite_test, facing_test, cast_view_test, flat_test, roof_test   write pictures to look at
option(BUILD_TESTS "Build the headless tests" ON)
if(BUILD_TESTS)
  enable_testing()
  set(TEST_OUT_DIR "${CMAKE_BINARY_DIR}/test_output")
  file(MAKE_DIRECTORY "${TEST_OUT_DIR}")

  set(TEST_NAMES logic_test cast_test mirror_test sprite_test facing_test cast_view_test flat_test roof_test)
  # these two reach into private members (#define private public), which MSVC cannot link; GCC / MinGW / Clang can
  if(NOT MSVC)
    list(APPEND TEST_NAMES peep_test roof_walk_test)
  endif()

  foreach(t ${TEST_NAMES})
    if(EXISTS "${CMAKE_SOURCE_DIR}/tests/${t}.cpp")
      add_executable(${t} tests/${t}.cpp)
      target_link_libraries(${t} PRIVATE watchman_core)
      target_compile_definitions(${t} PRIVATE TEST_OUT=\"${TEST_OUT_DIR}/\")
      add_test(NAME ${t} COMMAND ${t} WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
    endif()
  endforeach()
endif()
