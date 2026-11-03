<div align="center">

<img src="assets/horror_banner.svg" alt="WATCH-MAN: a bleak 2030 detective game, raycasted in C++17" width="100%">

<br>

<img src="https://img.shields.io/badge/C%2B%2B17-161d1a?style=for-the-badge&logo=cplusplus&logoColor=e0a458" alt="C++17">
<img src="https://img.shields.io/badge/Raylib-161d1a?style=for-the-badge&logoColor=e0a458" alt="Raylib">
<img src="https://img.shields.io/badge/whisper.cpp-161d1a?style=for-the-badge&logoColor=e0a458" alt="whisper.cpp">
<img src="https://img.shields.io/badge/llama.cpp-161d1a?style=for-the-badge&logoColor=e0a458" alt="llama.cpp">
<img src="https://img.shields.io/badge/Piper_TTS-161d1a?style=for-the-badge&logoColor=e0a458" alt="Piper TTS">
<a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-161d1a?style=for-the-badge&labelColor=161d1a&color=c4552d" alt="MIT License"></a>

</div>

<br>

<img src="assets/sec-briefing.svg" alt="01 Briefing" width="100%">

> **WATCH:** *"Case loaded. Don't expect me to enjoy it."*

Set in **2030** in a bleak, Soviet-toned urban hellscape of identical brutalist apartment blocks, **WATCH-MAN** puts you in the trench coat of a weary 40-something detective investigating a grisly murder inside **Block 7, Apartment 106**.

The police department has automated investigations, assigning every detective an AI wrist-watch system trained to detect evidence invisible to human eyes. The catch? **Your watch AI has gone rogue.**

Mirroring your own cynicism, the AI insults you, refuses to cooperate, and intentionally withholds critical clues. With a proper tech patch taking over a week, you must investigate the crime scene, interrogate suspicious residents, and **negotiate with, manipulate, or butter up your hostile wrist AI** before the killer escapes.

| Field | Details |
|:---|:---|
| **Setting** | 2030, Block 7, Apartment 106 |
| **Genre** | First-person detective noir |
| **Engine** | Custom C++17 raycaster built on Raylib |
| **Voice** | Fully offline: whisper.cpp, llama.cpp, Piper |
| **Origin** | Academic semester project in data structures and algorithms |

<br>

<img src="assets/sec-footage.svg" alt="02 Footage" width="100%">

<p align="center">
  <a href="assets/game_screenshot.png"><img src="assets/gameplay-frame.svg" alt="WATCH-MAN gameplay: a first-person view down a dim apartment corridor in Block 7" width="92%"></a>
  <br>
  <sub><i>Figure 1: The custom C++ raycaster rendering a Block 7 corridor. Click the image for the full-size screenshot.</i></sub>
</p>

<br>

<img src="assets/sec-watch.svg" alt="03 The rogue watch" width="100%">

The wrist AI is not a static menu. It has hidden psychological drives (**Ego**, **Paranoia**, **Trust**). Everything you say is transcribed, classified into an emotional intent, and those intents move the drives that decide whether a restricted clue gets unlocked.

<img src="assets/watch-ai.svg" alt="The rogue wrist AI has hidden drives: Ego, Paranoia and Trust. In this illustration, pleading fails, flattery raises trust, and a clue is unlocked." width="100%">

<sub><i>The exchange above illustrates the mechanic. It is not a transcript from the game.</i></sub>

The whole voice loop runs on your machine, with nothing sent to the cloud:

<img src="assets/voice-pipeline.svg" alt="Offline voice pipeline: microphone, whisper.cpp speech to text, intent classification with a trie, AI state machine, game state that owns all evidence, llama.cpp for personality only, Piper text to speech." width="100%">

<br>

<img src="assets/sec-systems.svg" alt="04 Core systems" width="100%">

<p align="center">
  <img src="assets/feature-raycaster.svg" alt="Custom C++17 raycaster: built from scratch with Raylib and no engine. Corridors, dynamic torch lighting, procedurally textured walls and billboard NPC sprites." width="49%">
  <img src="assets/feature-voice.svg" alt="Offline voice companion: real-time speech-to-text with whisper.cpp, local LLM inference with llama.cpp, neural text-to-speech with Piper. 100% offline." width="49%">
  <img src="assets/feature-negotiation.svg" alt="Rogue AI negotiation state machine: hidden drives (Ego, Paranoia, Trust). Player voice is classified into emotional intents that unlock restricted case clues." width="49%">
  <img src="assets/feature-separation.svg" alt="Strict game state separation: the game state owns all ground-truth evidence, the LLM only drives personality and voice generation, which prevents hallucinated clues." width="49%">
</p>

<br>

<img src="assets/sec-dsa.svg" alt="05 Under the hood" width="100%">

Built as an academic semester project, the game's core architecture leans on foundational data structures:

| Data Structure / Algorithm | System Integration |
| :--- | :--- |
| **Grid Graph & BFS / A\*** | Used for NPC pathfinding and real-time hallway pacing behavior. |
| **Hash Map (`std::unordered_map`)** | Stores discovered clues and case file dossiers with $O(1)$ dynamic lookup. |
| **Prefix Tree (Trie)** | High-speed keyword and intent matching against player speech transcripts. |
| **Directed Acyclic Graph (DAG) & Topological Sort** | Manages evidence dependencies, ensuring prerequisite clues are linked before unlocking higher-level deductions. |
| **Priority Queue (Min-Heap)** | Schedules game events, flickering lighting states, and audio spatial cues. |

<p align="center">
  <img src="assets/evidence-dag.svg" alt="Illustration of an evidence dependency graph: clues feed deductions, which feed a conclusion, with a topological unlock order." width="49%">
  <img src="assets/pathfinding-grid.svg" alt="Illustration of breadth-first search expanding across a grid of corridors to find the shortest path for an NPC." width="49%">
  <br>
  <sub><i>Simplified illustrations of the DAG and grid-graph ideas, not in-game screenshots.</i></sub>
</p>

<br>

<img src="assets/sec-build.svg" alt="06 Stack and build" width="100%">

| Layer | Tech |
| :--- | :--- |
| **Language** | C++17 |
| **Graphics & windowing** | [Raylib](https://www.raylib.com/) |
| **Build system** | CMake (MinGW-w64 / MSYS2) |
| **Speech-to-text** | `whisper.cpp` (quantized `tiny.en` / `base.en`) |
| **LLM engine** | `llama.cpp` (quantized, low-latency instruct model) |
| **Text-to-speech** | `Piper TTS` |

### Prerequisites

- GCC / G++ supporting C++17
- CMake 3.16+
- Raylib binaries installed

### Build

```bash
# Clone the repository
git clone https://github.com/abduhoo-bin-nasir/watch-man.git
cd watch-man

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build .

# Run the executable
./watch_man
```

<br>

<img src="assets/footer.svg" alt="The watch is still listening" width="100%">

<p align="center"><sub>Built by <a href="https://abduhoo-bin-nasir.github.io">Abduhoo Bin Nasir</a> &nbsp;·&nbsp; <a href="https://github.com/abduhoo-bin-nasir">GitHub</a></sub></p>
