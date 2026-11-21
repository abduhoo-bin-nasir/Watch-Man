# Block 7 / WATCH-MAN

A first-person noir detective game in C++17 + raylib with a custom software renderer.
**Start with [`docs/README.md`](docs/README.md)** (game design, narrative bible, architecture, renderer, DSA plan, code tour with exercises).

## Source map

`src/main.cpp` only does `Game game; return game.run();`. Everything else is split by responsibility:

| File | What it is | Edit it to... |
|---|---|---|
| `LevelData.cpp` | ASCII floor plans, light bulbs, staircases, who stands/lives where, props | change the building |
| `CharacterArt.cpp` | pixel designs (ASCII grids) + the cast list (name, colours, height) | change how people look |
| `DialogueData.cpp` | every line of dialogue, "no answer" door lines | change what people say |
| `ClueBook.cpp` | the clues and their dependencies (a DAG) | change the case |
| `Level.cpp/.h` | ASCII plan -> stacked floors, stairs, lift doors | (engine) |
| `Entities.cpp/.h` | `Entity > Prop / Character > Wanderer, Resident / Player` | (engine) |
| `Dialogue.cpp/.h` | the conversation menu and typing effect | (engine) |
| `Renderer.cpp/.h` | the 320x180 software renderer | (engine) |
| `Ui.cpp/.h` | HUD, dialogue box, journal, ending | (look of the screens) |
| `Game.cpp/.h` | state machine: walk, knock, talk, stairs, lift, journal | (rules) |

Build: `cmake -S . -B build -G "MinGW Makefiles"` then `cmake --build build -j` then `build\block7.exe`.

Tests: `build\logic_test.exe` runs the headless logic tests (`tests/logic_test.cpp`).
