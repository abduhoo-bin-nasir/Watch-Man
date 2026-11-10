<div align="center">

<img src="assets/banner.svg" alt="WATCH-MAN: a bleak 2030 detective game, raycasted in C++17" width="100%">

<br>

<img src="https://img.shields.io/badge/C%2B%2B17-161d1a?style=for-the-badge&logo=cplusplus&logoColor=e0a458" alt="C++17">
<img src="https://img.shields.io/badge/Raylib_5.5-161d1a?style=for-the-badge&logoColor=e0a458" alt="Raylib 5.5">
<img src="https://img.shields.io/badge/CMake-161d1a?style=for-the-badge&logo=cmake&logoColor=e0a458" alt="CMake">
<img src="https://img.shields.io/badge/Status-In_development-161d1a?style=for-the-badge&labelColor=161d1a&color=e0a458" alt="Status: in development">
<a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-161d1a?style=for-the-badge&labelColor=161d1a&color=c4552d" alt="MIT License"></a>

</div>

<br>

<img src="assets/sec-briefing.svg" alt="01 Briefing" width="100%">

> **WATCH:** *"Case loaded. Don't expect me to enjoy it."*

Set in **2030** in a bleak, Soviet-toned urban hell of identical brutalist apartment blocks, **WATCH-MAN** puts you in the trench coat of a weary 40-something detective investigating a grisly murder inside **Block 7, Apartment 106**.

He hates AI. The department has issued him one anyway: a wrist-watch system trained to spot evidence invisible to human eyes. The catch is that **your watch has gone rogue**. It swears, refuses to work, and withholds clues it already knows. The patch takes a week. Until then you work the scene, question the neighbours, and **flatter, negotiate with and manipulate a hostile wrist AI** before the killer gets away.

<img src="assets/case-file.svg" alt="Case file 7-106. Setting: 2030, Block 7, Apartment 106. Genre: first-person detective noir. Engine: custom C++17 raycaster on Raylib 5.5. Voice: offline stack planned with whisper.cpp, llama.cpp and Piper. Origin: data structures and algorithms semester project. Status: in development." width="100%">

<br>

<img src="assets/stats.svg" alt="2030, apartment 106, 1 rogue watch AI, 5 data structure showcases" width="100%">

<br>

<p align="center">
  <img src="assets/feature-raycaster.svg" alt="Pixel raycaster, built: a Doom-style column renderer in custom C++17 and Raylib 5.5, with a floor and ceiling height for every grid cell." width="49%">
  <img src="assets/feature-look.svg" alt="Urban hell look, built: low-res 320x180 pixels, procedural textures and ceiling bulbs that cast light pools." width="49%">
  <img src="assets/feature-stairs.svg" alt="Real stairs, built: two floors joined by a 10-step flight, rendered with no special case." width="49%">
  <img src="assets/feature-watch.svg" alt="Rogue watch AI, planned: a wrist AI that swears, stalls and withholds clues, and can be flattered, bribed or threatened." width="49%">
  <img src="assets/feature-voice.svg" alt="Offline voice, planned: speech-to-text, a small local LLM and text-to-speech on your machine, with a text-input fallback." width="49%">
  <img src="assets/feature-facts.svg" alt="Game owns the facts, a design rule: clues, suspects and trust live in game state, and the LLM only supplies voice and personality." width="49%">
</p>

<br>

<img src="assets/sec-footage.svg" alt="02 Footage" width="100%">

<p align="center">
  <a href="assets/game_screenshot.png"><img src="assets/gameplay-frame.svg" alt="WATCH-MAN gameplay: a first-person view down a dim apartment corridor in Block 7" width="92%"></a>
  <br>
  <sub><i>Figure 1: The custom C++ raycaster rendering a Block 7 corridor. Click the image for the full-size screenshot.</i></sub>
</p>

Every frame is built at **320x180** and scaled up with nearest-neighbour filtering, so the pixels stay chunky on any screen. It is light enough for a low-end laptop.

<img src="assets/render-pipeline.svg" alt="Frame pipeline in five stages: columns, textures, sprites, light, and grain." width="100%">

<sub><i>A simplified walkthrough of how a frame comes together, drawn with a stand-in corridor. These are not captures from the game.</i></sub>

<br>

<img src="assets/sec-block.svg" alt="03 The block" width="100%">

One floor is a **16 x 15 tile** hallway ring wrapped around a solid block of apartments. Doors **101-108** are downstairs and **201-208** upstairs. Door **106** is the crime scene. Rain only ever shows up streaking the hallway windows, because the whole story happens indoors.

<img src="assets/block-plan.svg" alt="Illustrative floor plan of a hallway ring around a solid block of apartments, with doors 101 to 108, door 106 taped off, windows in the outer walls, props, stairs and a patrolling man in a red scarf." width="100%">

<sub><i>An illustrative layout showing what the world contains. It is not the real level map.</i></sub>

The two floors do not stack. They sit **side by side in one grid**, joined by a stair run, which avoids floor-swap glitches entirely. Stairs are real steps in the same renderer.

<img src="assets/two-floors.svg" alt="Diagram: a 36 by 15 tile grid with the ground floor on tiles 0 to 15, a stair run on tiles 16 to 19 and the upper floor on tiles 20 to 35. Ten steps of 0.2 rise a total of 2.0." width="100%">

<br>

<img src="assets/sec-cast.svg" alt="04 The cast" width="100%">

Nobody in Block 7 is telling you everything.

<img src="assets/lineup.svg" alt="Line-up of persons of interest: the detective, Mrs. Volkova the nosy old neighbour, Pavel the janitor upstairs, Boris the cat, the man in the red scarf who patrols the ring, and the rogue watch." width="100%">

<sub><i>Sprites above are redrawn illustrations of the cast, not the in-game assets.</i></sub>

<br>

<img src="assets/sec-watch.svg" alt="05 The rogue watch" width="100%">

The watch is not a menu. The design gives it hidden drives (**Ego**, **Paranoia**, **Boredom**) and a **trust** meter. Whatever you say gets classified into an intent (flattery, apology, threat, bribe, question, insult), the intent moves the numbers, and restricted clues unlock as trust crosses thresholds.

<img src="assets/watch-ai.svg" alt="Illustration of the rogue watch: hidden drives, a trust meter with clue gates, six intent types, and a sample exchange where flattery unlocks a clue." width="100%">

<sub><i>The exchange above illustrates the planned mechanic. It is not a transcript from the game.</i></sub>

The voice loop is designed to run entirely on your machine, with nothing sent to the cloud. A text-input fallback keeps the game playable without a microphone.

<img src="assets/voice-pipeline.svg" alt="Planned offline voice pipeline: microphone, whisper.cpp, intent matching, watch state, game state that owns the facts, llama.cpp for voice only, and Piper text to speech, with a text-input fallback." width="100%">

> [!IMPORTANT]
> **The game owns every fact.** Clues, suspects, the trust meter and what the watch knows all live in game state. The LLM is only ever asked for voice and personality, so it can never invent evidence.

<br>

<img src="assets/sec-dsa.svg" alt="06 Under the hood" width="100%">

WATCH-MAN started as a **Data Structures and Algorithms** semester project, so the systems are built to show the structures off.

| Data structure / algorithm | Where it fits | Status |
| :--- | :--- | :---: |
| **Hash map** (`std::unordered_map`) | Clue database and case-file dossiers with O(1) lookup | Planned |
| **DAG + topological sort** | Clue dependencies, so clue B only unlocks after clue A | Planned |
| **Trie / Aho-Corasick** | Spotting intent keywords in spoken transcripts | Planned |
| **Grid graph, BFS / A\*** | NPC pathfinding on the tile grid | Planned |
| **Priority queue (min-heap)** | Scheduling ambient events: flicker, sounds, patrols | Planned |

<img src="assets/evidence-dag.svg" alt="Illustrative evidence dependency graph: five clues feed two deductions, which feed a conclusion, numbered in one valid topological order." width="100%">

<img src="assets/pathfinding-grid.svg" alt="Breadth-first search expanding around the hallway ring from an NPC, with a prop blocking one route and the shortest path highlighted." width="100%">

<sub><i>Illustrations of the ideas. The BFS is a real search run on a made-up map, not a capture from the game.</i></sub>

<br>

<img src="assets/sec-status.svg" alt="07 Case status" width="100%">

This is a solo, in-progress project, so here is exactly where it stands.

<img src="assets/status.svg" alt="Case status. Built: pixel column renderer, real stairs, procedural textures and sprites, lighting, NPC waypoint loops, movement and mouse look. Planned in order: clue system and case board, trust meter and intent classifier, offline voice loop, NPC pathfinding and event scheduler, module split." width="100%">

<br>

<img src="assets/sec-build.svg" alt="08 Build" width="100%">

<img src="assets/controls.svg" alt="Controls: W A S D to move, left and right arrows to turn, TAB to toggle mouse look, E to interact, C for the case board (planned)." width="100%">

| Key | Action |
| :--- | :--- |
| `W` `A` `S` `D` | Move |
| `←` `→` | Turn |
| `TAB` | Toggle mouse look |
| `E` | Interact |
| `C` | Case board *(planned)* |

| Layer | Tech |
| :--- | :--- |
| **Language** | C++17 |
| **Graphics and windowing** | [Raylib](https://www.raylib.com/) 5.5 |
| **Build system** | CMake (MinGW-w64 / MSYS2) |
| **Speech-to-text** | `whisper.cpp`, quantized `tiny` / `base` *(planned)* |
| **LLM** | `llama.cpp`, a 1-3B quantized model *(planned)* |
| **Text-to-speech** | `Piper TTS` *(planned)* |

### Prerequisites

- A C++17 compiler (GCC via MinGW-w64 / MSYS2 on Windows)
- CMake 3.16+

CMake downloads and builds Raylib for you through `FetchContent`, so the first configure can take several minutes.

### Build and run

```bash
# Clone the repository
git clone https://github.com/abduhoo-bin-nasir/watch-man.git
cd watch-man

# Configure and build
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build -j

# Run (Windows)
build\block7.exe
```

> [!NOTE]
> The executable name comes from the CMake target, which is `block7` in the current build.

<br>

<img src="assets/footer.svg" alt="The watch is still listening" width="100%">

<p align="center"><sub>Built by <a href="https://abduhoo-bin-nasir.github.io">Abduhoo Bin Nasir</a> &nbsp;·&nbsp; <a href="https://github.com/abduhoo-bin-nasir">GitHub</a></sub></p>
