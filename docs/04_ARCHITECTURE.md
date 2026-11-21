# 04 - Architecture

## 1. Module map

`src/main.cpp` is two lines: `Game game; return game.run();`.

| File(s) | Responsibility | You edit it to... |
|---|---|---|
| `Common.h` | constants (`RW`, `RH`, `WALL_H`...), `RGB`, `V2`, hash helpers, `Input`, `ArtId` | (rarely) |
| `Game.cpp/.h` | the state machine and main loop | change rules/flow |
| `Level.cpp/.h` | ASCII plan -> `Level` grids; `Building` stacks them; stairs, lift doors | (engine) |
| `LevelData.cpp` | **the building**: floor plans, bulbs, flights, who stands/lives where, props | change the map |
| `Entities.cpp/.h` | `Entity > Prop / Character > Wanderer, Resident / Player` | (engine) |
| `CharacterArt.cpp` | **the cast and their pixel designs** | change looks |
| `Dialogue.cpp/.h` | `Conversation`: menu, typing effect, clue hand-out | (engine) |
| `DialogueData.cpp` | **every line people say** | change dialogue |
| `ClueBook.cpp/.h` | **the clues** + the DAG logic | change the case |
| `Renderer.cpp/.h` | the software renderer | (engine) |
| `Ui.cpp/.h` | HUD, dialogue box, journal, ending (draw only) | change how screens look |

Rule of thumb: files ending in `Data.cpp`, plus `CharacterArt.cpp` and the top of `ClueBook.cpp`, are **content**; the rest is **engine**.

## 2. Class diagram

```mermaid
classDiagram
  class Entity { +x +y +yaw +floor +update(dt) }
  class Prop { +art +h +main }
  class Character { +id +name +role +art +main +acc +h +script +notes +met +talking +hidden +lit }
  class Wanderer { +setRoute() +update(dt) }
  class Resident { +door +inDoorway +appearAt() +leave() }
  class Player { +feet +pitch +bob +steer() +walk() +easeTo() +lookAt() +settle() }
  Entity <|-- Prop
  Entity <|-- Character
  Entity <|-- Player
  Character <|-- Wanderer
  Character <|-- Resident

  class Level { +grid +lamps +doorAmt +doorTgt +build() +cellAt() +passable() +inCar() }
  class Building { +floors +current() }
  Building *-- Level

  class ClueBook { +clues(hash map) +topo +give() +canConfront() }
  class Conversation { +speaker +text +menu +begin() +update() }
  class Renderer { +render() +focus +nearBill }
  class Ui { +draw() +pointerInput() }
  class Game { +update() +run() +mode }
  Game *-- Building
  Game *-- Player
  Game *-- ClueBook
  Game *-- Renderer
  Game *-- Conversation
  Game *-- Ui
  Game o-- Character
  Game o-- Prop
  Conversation --> Character
  Conversation --> ClueBook
```

**Inheritance rationale:** `Entity` = anything with a place. `Character` adds identity and dialogue. `Wanderer` and `Resident` differ in *behaviour*, not in data: one walks a route, the other lives behind a door and steps into the doorway. `Player` shares only position and heading, so it is an `Entity` but not a `Character`.

## 3. Game states

```mermaid
stateDiagram-v2
  [*] --> WALK
  WALK --> KNOCK: E at apartment door
  KNOCK --> WALK: nobody answers
  KNOCK --> TALK: resident opens door
  WALK --> TALK: E at a hallway person
  TALK --> WALK: "That's all" / ESC
  TALK --> END: confront with enough clues
  END --> WALK: Enter (suspect taken away)
  WALK --> JOURNAL: J
  TALK --> JOURNAL: J
  JOURNAL --> WALK: J / ESC
  JOURNAL --> TALK: J / ESC (returns to where you were)
  WALK --> FADE: step on a stair "link" cell
  FADE --> WALK: floor switched, fade in
  WALK --> RIDE: E inside the lift cabin
  RIDE --> WALK: doors open on the other floor
```

During `JOURNAL` the world is frozen (no clock, no patrols). During `KNOCK`/`TALK`/`FADE`/`RIDE` the player cannot move; the camera and position are eased by the game.

## 4. One frame

```mermaid
flowchart TD
  A[Game::run: read keyboard/mouse into Input] --> B[Ui::pointerInput: menu hover/click]
  B --> C[Game::update dt, Input]
  C --> D[clock, timers, Weather::update, Level::updateLamps]
  D --> E[player steer/walk or scripted easing]
  E --> F[Character::update: patrols]
  F --> G[door animation, lift door cells, residents leave]
  G --> H[collectBillboards: people + props on this floor]
  H --> I[Renderer::render -> 320x180 pixel buffer + Focus + nearBill]
  I --> J[mode-specific update: knock / talk / fade / ride / end / interact]
  J --> K[Ui::draw: upload texture, HUD, dialogue, journal, fade overlay]
```

`Renderer` only fills a memory buffer (no raylib calls), which is why the game logic can be tested without a window.

## 5. Data formats you edit

### Floor plan (`LevelData.cpp`)
One character per **tile**; each tile = 2x2 **cells**. Legend: `#` wall, `W` window, `.` floor, `k` stair handrail wall, `m` metal wall (cabin), `1-8` apartment doors, `E` lift doors, `L` lift cabin, `u`/`d` stair tiles (documentation only). Both floors are 19 x 15 tiles.

### Flights (staircases)
`{tx, ty, dir, lowFloor}`: a flight rises in `dir` over `STAIR_TILES` (5) tiles = 10 steps of 0.2 = one storey. It is laid into **both** floors it joins (on the lower floor F = 0.2..2.0, on the upper floor F = -1.8..0.0), so heights agree cell by cell. `lowFloor = -1` is a flight from the basement (ends in a locked boiler door); `lowFloor = floorCount-1` goes to the roof (locked door).

### Placement of people
`Placement{id, floor, tile, patrol route, speed, facing}` for hallway people; `HomeDef{id, floor, door}` for residents; `PropDef` for crates.

### Cast, dialogue, clues
`CastDef` (appearance + identity), `Script`/`Topic` (words), `ClueBook::add` (case). They connect by **string ids** (`"volkova"`, `"argue"`).

## 6. Design decisions (and why)
| Decision | Reason |
|---|---|
| Each floor is its own grid | The renderer cannot draw a room above another room. Stacking by data + a fade is simple and robust. |
| One `FlightDef` per physical staircase, laid into both floors | The two floors can never disagree about heights; hand-editing two copies would drift. |
| 2x2 cells per tile | A step is half a tile deep, so a flight is 5 tiles = 10 steps. |
| Game owns facts, AI only voices them | Solvability, testability, offline operation. |
| Content in `*Data.cpp` files, strings as ids | Edit story without touching engine code. |
| `Renderer` has no raylib calls | Testable headless; fast; the pixel buffer is uploaded once per frame. |
| ESC backs out of menus | `SetExitKey(KEY_NULL)`; quitting is ESC in WALK only. |
