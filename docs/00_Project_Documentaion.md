# WATCH-MAN - project documentation

A first-person noir detective game (C++17, raylib, custom software renderer). Your watch AI has gone rogue; you must investigate and negotiate with it to solve a murder.

## Read in this order
| # | Document | Answers |
|---|---|---|
| 01 | [Game Design](01_GAME_DESIGN.md) | What is the game? Pillars, loop, the Watch AI, journal, tone |
| 02 | [Narrative Bible](02_NARRATIVE_BIBLE.md) | What really happened? Timeline, cast, clue graph, statements vs facts |
| 03 | [Vertical Slice](03_VERTICAL_SLICE.md) | What exactly are we building now? Feature matrix, milestones, acceptance criteria |
| 04 | [Architecture](04_ARCHITECTURE.md) | How is the code organised? Classes, states, frame flow, data formats |
| 05 | [Renderer](05_RENDERER.md) | How does raycasting work? (from zero) |
| 06 | [DSA](06_DSA.md) | Which data structures, why, and is it enough? |
| 07 | [Code Tour](07_CODE_TOUR.md) | How do I learn and change the code? Reading order, traces, exercises |
| 08 | [Testing and Risks](08_TESTING_AND_RISKS.md) | What is verified, what is risky, what is undecided? |

## Status in one table
| Area | State |
|---|---|
| Renderer, lighting, weather | done |
| Two stacked floors, central stairwell, lift | done |
| Doors with residents in the doorway, interviews, journal, case board, one ending | done |
| The Watch AI (intent engine, trust, gating) | planned (M2) |
| Schedules, pathfinding, event scheduler | planned (M3) |
| Alibi timeline / contradictions | planned (M4) |
| Audio, title/pause/save | planned (M5) |
| Voice | optional (M6) |

## Glossary
- **Cell / tile:** a cell is one grid square the renderer works with; a tile (one character of a floor plan) is 2x2 cells.
- **DDA:** the algorithm that walks a ray from grid line to grid line.
- **Billboard:** a flat sprite that always faces the camera (here, two crossed planes) used for people and props.
- **Flight:** one physical staircase, laid into both floors it joins.
- **Link cell:** the last steps of a flight; stepping on one changes floor.
- **Clue DAG:** the graph of which clues need which others.
- **Deduction:** a clue found automatically when its prerequisites are all in the journal.
- **Topological order (Kahn):** an order of the clue graph where every clue comes after its prerequisites.
- **Vertical slice:** the complete game at small scale.
- **Removal test:** "if I delete this structure, which gameplay stops working?"
