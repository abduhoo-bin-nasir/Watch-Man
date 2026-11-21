# 08 - Testing, known limits and risks

## 1. How this was verified (and what that does NOT prove)

| Check | Method | Proves |
|---|---|---|
| Compiles cleanly | g++ 13 `-std=c++17 -Wall -Wextra` against raylib 5.5, no warnings | syntax/type correctness on Linux |
| Logic | `tests/logic_test.cpp`: a headless scripted playthrough (stairs both ways, lift ride, door knocks, every interview, the whole case to the ending) | the rules behave as designed |
| Memory/UB safety | the same test built with AddressSanitizer + UBSan | no out-of-range reads/writes, no undefined behaviour on those paths |
| Robustness | 40,000 steps of random input (random teleports, random menu picks, random keys) asserting the player is never inside a wall | no crashes or wall-clipping in tested situations |
| Visual sanity | scripted run in a virtual display (software OpenGL) with screenshots reviewed by eye | the renderer produces the intended pictures |

**Not verified:** Windows/MinGW build, real GPU, window resizing on your monitor, mouse look, audio (none), long play sessions, feel/pacing. **"Looks right in a screenshot" is not "feels good to play"; only you can confirm that.**

## 2. Running the tests
`cmake -S . -B build -G "MinGW Makefiles"` then `cmake --build build -j` builds two programs: `block7.exe` (the game) and `logic_test.exe` (headless; run it from the console, it prints `ok:`/`FAIL:` lines and a summary).

## 3. Test plan for upcoming features
| Feature | Tests to write first |
|---|---|
| Level validator (BFS) | passes on the shipped plans; a plan with an unreachable door fails; stairs/lift counted as connections |
| Intent engine | labelled sentence set; overlaps; empty/very long input; case and punctuation |
| Gating | try all clue orders x trust levels: no clue ever revealed early |
| Pathfinder | optimality on hand-made grids; floor change; blocked paths; 100 random queries timing |
| Scheduler | ordering of equal timestamps; reschedule; large N |
| Timeline | known conflicts found, non-conflicts not flagged, brute-force comparison on random data |
| Save/load | round trip equality of the whole game state |

## 4. Known limitations
- Two floors only; a third needs data work (exercise E10).
- No audio, title screen, pause or save yet.
- People walk fixed loops or stand still (no schedules).
- The lift is one cabin per floor in the same place (no call/queue simulation).
- Levels/clues are compiled into the program (no external data files); changing them needs a rebuild.
- `lampL` loops all lamps per pixel; fine for ~20 lamps per floor, needs a spatial index for hundreds.
- Text uses raylib's built-in font; the journal's handwriting is just slight tilts.

## 5. Risk register
| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Scope creep ("one more mechanic") | high | high | the matrix in `03_VERTICAL_SLICE.md` is the contract |
| Not understanding your own code | medium | high | M0: tour + exercises before M2 |
| Voice pipeline too slow/heavy on weak laptops | high | medium | last milestone; optional; text fallback always; 1-3B model only |
| Writing volume for the Watch's lines | medium | medium | template per (intent x mood x clue), generated variants only as polish |
| DSA judged "minor" | was high | high | `06_DSA.md`: five central structures, removal test, evidence pack |
| Bugs from hand-edited ASCII levels | medium | low | validator (BFS) + tests |
| Windows-only build surprises | medium | medium | build on Windows early (M1), keep CMake simple, FetchContent raylib |

## 6. Open decisions (need you)
1. **Deadline:** do you want the in-game "Watch is fixed at 06:00" deadline (GDD section 9) or an untimed investigation with schedules only?
2. **Fail state:** is a wrong accusation game over, or a lost chance you can recover from?
3. **Watch interface:** a text box that opens on a key, or a persistent chat line on the HUD?
4. **Audio source:** generated sounds, or free CC0 assets with attribution?
5. **Voice:** is it in or out of the slice? (Recommendation: out until everything else is finished.)

## 7. Definition of done for the slice
- [ ] Every row marked DONE/PLANNED in `03_VERTICAL_SLICE.md` is DONE
- [ ] Both floors playable from title screen to an ending
- [ ] No crash in a 30-minute session and in the random-input fuzz
- [ ] All DSA unit tests pass and the benchmark numbers are recorded
- [ ] README with screenshots, build instructions and this `docs/` folder committed
