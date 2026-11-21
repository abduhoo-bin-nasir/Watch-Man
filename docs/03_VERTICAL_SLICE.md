# 03 - Vertical Slice Definition and Roadmap

## 1. What "the slice" is

**The slice is the whole game, small.** Two floors of Block 7, one complete case, every mechanic the final game will ever have. After the slice, growth = more data (floors, people, clues, cases). **If something needs new engine code, it belongs in the slice.**

*Definition of done for the slice:* a stranger can start the game from a title screen, solve the case (or fail it), hear the world, negotiate with the Watch, and reach one of several endings, without crashing, in 20-40 minutes.

## 2. Feature matrix (status audit)

Effort: S = a day or less, M = a few days, L = a week or more.

### A. World and engine
| ID | Feature | Status | Notes |
|---|---|---|---|
| W-01 | Software renderer: textured walls/floors/ceilings, per-cell heights | [DONE] | `Renderer.cpp`, see `05_RENDERER.md` |
| W-02 | Lighting: hanging bulbs, buzzing/dying bulbs, vignette, grain | [DONE] | |
| W-03 | Windows with rain, skyline and thunder | [DONE] | |
| W-04 | Stacked floors with identical footprint | [DONE] | `Level`/`Building` |
| W-05 | Central stairwell: up flight + down flight on each floor, floor change by fade | [DONE] | one physical flight laid into both floors |
| W-06 | Lift in a corner (call, ride, doors) | [DONE] | |
| W-07 | Doors that open with the resident in the doorway; flats can't be entered | [DONE] | |
| W-08 | Level format readable/editable as ASCII; level validator tool | [PARTIAL] | format done; **validator (BFS reachability check) missing** (S) |
| W-09 | Audio (ambience, foley, UI sounds) | [PLANNED] | **M** |

### B. Characters and dialogue
| ID | Feature | Status | Notes |
|---|---|---|---|
| C-01 | Character class hierarchy, ASCII-art designs, cast list | [DONE] | |
| C-02 | Menu-based interviews gated by clues | [DONE] | |
| C-03 | Residents answer doors; hallway people walk up to you | [DONE] | |
| C-04 | **People with schedules** (move between rooms/floors by time of day) | [PLANNED] | needs A* + scheduler, **M** |
| C-05 | **Pathfinding** across both floors via stairs and lift | [PLANNED] | A*, **M** |
| C-06 | Reactions: people say something different after the arrest / at night | [PLANNED] | **S** |

### C. Case and journal
| ID | Feature | Status | Notes |
|---|---|---|---|
| J-01 | Journal: Case, Notes, People, Board | [DONE] | |
| J-02 | Clue DAG, topological order, auto deductions | [DONE] | |
| J-03 | Confrontation + one ending | [DONE] | |
| J-04 | **Statements and contradiction detection** (alibi timeline page in the journal) | [PLANNED] | interval structure, **M** |
| J-05 | Fail state / wrong accusation / second ending | [PLANNED] | **S** |

### D. The Watch AI (the gimmick)
| ID | Feature | Status | Notes |
|---|---|---|---|
| A-01 | Watch text interface (open with a key, type, see replies, HUD wrist icon) | [PLANNED] | **M** |
| A-02 | **Intent engine** (Aho-Corasick keyword automaton) | [PLANNED] | **M** |
| A-03 | Trust meter + hidden axes (Ego, Paranoia, Boredom) + visible tells | [PLANNED] | **M** |
| A-04 | Trust/DAG-gated clue reveal; withholding; sulking | [PLANNED] | **M** |
| A-05 | Canned lines per (intent x mood x clue) | [PLANNED] | **S** (writing) |
| A-06 | Optional voice loop (STT -> game -> optional LLM phrasing -> TTS) | [PLANNED] | **L**, last; text input always works |

### E. Game flow and polish
| ID | Feature | Status | Notes |
|---|---|---|---|
| G-01 | Title screen, pause menu, quit/restart | [PLANNED] | **S** |
| G-02 | Save/load (journal, clue state, time, position) | [PLANNED] | **S-M** |
| G-03 | Tutorial hints (first knock, first journal entry) | [PLANNED] | **S** |
| G-04 | In-game clock with schedules/deadline (see GDD section 9) | [PARTIAL] | clock runs; no events use it yet |
| G-05 | Event scheduler (ambient events, watch outbursts, timed story beats) | [PLANNED] | min-heap, **M** |
| G-06 | Settings (volume, mouse sensitivity, text speed) | [PLANNED] | **S** |

### F. Project quality
| ID | Feature | Status |
|---|---|---|
| Q-01 | Documentation set (`docs/`) | [DONE: this folder] |
| Q-02 | Automated tests for logic modules (DAG, intent engine, A*, timeline) | [PARTIAL] (headless playthrough test exists, not in repo yet) |
| Q-03 | README with screenshots for the portfolio | [PLANNED] |

## 3. Milestones (recommended order)

Each milestone ends in something you can run and demo.

| # | Milestone | Features | Why in this order |
|---|---|---|---|
| **M0** | **Understand + document** | Q-01, code tour exercises (`07_CODE_TOUR.md`) | You must own the code before extending it |
| **M1** | **Foundations** | W-08 validator (BFS), Q-02 test harness into the repo, G-01 title/pause | small wins, adds BFS and a test habit |
| **M2** | **The Watch, text only** | A-01..A-05 | the gimmick; intent engine + trust + DAG gating; core DSA lands here |
| **M3** | **Living building** | C-04, C-05, G-05, G-04 | A* + heap scheduler; people move; time matters |
| **M4** | **Deduction depth** | J-04, J-05, C-06 | interval timeline; contradictions; fail state |
| **M5** | **Sound and polish** | W-09, G-02, G-03, G-06 | makes it feel like a game |
| **M6** | **Voice (optional)** | A-06 | riskiest, last; the game is complete without it |

## 4. Acceptance criteria (examples)

- **A-02 Intent engine:** given 50 hand-written test sentences with known intent labels, >= 90% classified correctly; classification of a 200-character transcript in < 1 ms; unit test with overlapping phrases ("sorry" inside "I'm not sorry").
- **A-04 Gating:** a clue the Watch holds cannot be revealed before its prerequisite clues are in the journal AND trust >= threshold; verified by a test that tries every ordering.
- **C-05 Pathfinding:** from any walkable cell to any other (both floors) a path exists; path length is optimal on a hand-checked set; never routes through a closed door/wall; 100 random queries < 5 ms total.
- **J-04 Timeline:** the two contradictions in `02_NARRATIVE_BIBLE.md` section 4 are found; a statement that does not conflict produces no flag.
- **G-02 Save/load:** save, quit, reload: same journal, same clue flags, same clock, same floor and position.

## 5. Out of scope for the slice
More buildings or floors, extra cases, multiplayer, 3D models, procedural case generation, controller support, localisation. All of them are *expansions* of data or can be bolted on later.

## 6. Expansion path after the slice
1. **More floors:** add `LevelDef`s and flights in `LevelData.cpp` (the second-floor/roof flight becomes a real link automatically when a third floor exists).
2. **More people and lines:** `CastDef` + `Script` + `HomeDef`/`Placement`.
3. **More clues/cases:** `ClueBook.cpp` (or a loader for a case file; the structures stay the same).
4. **Bigger story:** the Watch's cover-up thread, a second case in the same block.

## 7. Risks (short; full list in `08_TESTING_AND_RISKS.md`)
Scope creep (mitigation: this matrix is the contract), voice performance on weak laptops (mitigation: last, optional), writing volume for the Watch's lines (mitigation: templates by intent x mood), being unable to explain the code (mitigation: M0).
