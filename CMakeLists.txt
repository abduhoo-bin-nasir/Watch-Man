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

set(GAME_SOURCES
  src/Game.cpp
  src/Level.cpp        src/LevelData.cpp
  src/Entities.cpp     src/CharacterArt.cpp
  src/Dialogue.cpp     src/DialogueData.cpp
  src/ClueBook.cpp
  src/Renderer.cpp
  src/Ui.cpp)

add_executable(Watch-Man src/main.cpp ${GAME_SOURCES})
target_include_directories(Watch-Man PRIVATE src)
target_link_libraries(Watch-Man PRIVATE raylib)

if(MSVC)
  target_compile_options(Watch-Man PRIVATE /W3)
else()
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

# Headless logic tests (no window). Run ./build/logic_test after building.
option(BUILD_TESTS "Build the headless logic tests" ON)
if(BUILD_TESTS)
  add_executable(logic_test tests/logic_test.cpp ${GAME_SOURCES})
  target_include_directories(logic_test PRIVATE src)
  target_link_libraries(logic_test PRIVATE raylib)
endif()
