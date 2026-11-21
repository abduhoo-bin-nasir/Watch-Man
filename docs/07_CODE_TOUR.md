# 07 - Code tour and exercises (how to make this project yours)

Goal: after this you can explain every file, trace any feature from key press to pixels, and add content without help. Budget: about two evenings. **Do the exercises yourself, rebuilding after each.** Reading alone won't stick.

Total code: ~3,060 lines. The biggest are `Renderer.cpp` (600), `CharacterArt.cpp` (500, mostly art), `Ui.cpp` (460), `Game.cpp` (355).

## Part 1 - Reading order

| # | File | Time | Look for |
|---|---|---|---|
| 1 | `Common.h` | 10 min | the constants; why `STAIR_TILES * 2 * STEP_H == WALL_H` is a `static_assert`; what `Input` carries |
| 2 | `LevelData.cpp` | 20 min | the ASCII floors (count the doors); `flights`; `characters`/`homes`; the comments are the legend |
| 3 | `Level.h`, then `Level.cpp` | 30 min | `Cell` fields; `Level::build` in 3 phases: plan -> cells, flights -> stair cells, lamps; `passable`; `doorInfoAt` |
| 4 | `Entities.h`, `Entities.cpp` | 20 min | the class tree; `Wanderer::update` (patrol); `Resident::appearAt`; `Player::walk` (sliding along walls = test x and y separately) |
| 5 | `CharacterArt.cpp` | 20 min | the header comment, one design, `finishArt` (outline + shading), the cast list |
| 6 | `Dialogue.h`, `Dialogue.cpp`, `DialogueData.cpp` | 25 min | `Topic`/`Script`; `Conversation::rebuild` (which questions appear), `choose` (what happens when you ask) |
| 7 | `ClueBook.h`, `ClueBook.cpp` | 25 min | `give` (the deduction loop), `buildTopo` (Kahn). **Draw the graph on paper first** |
| 8 | `Game.h` | 10 min | the mode enum; every public field is read by `Ui` |
| 9 | `Game.cpp` | 45 min | `update` (top to bottom), `interact`, `startKnock`/`updateKnock`, `startFade`/`updateFade`, `startRide`/`updateRide` |
| 10 | `05_RENDERER.md`, then `Renderer.h`, `Renderer.cpp` | 90 min | `renderColumn` first; `drawWallRows` last |
| 11 | `Ui.cpp` | 30 min | helpers at the top (`hand`, `stamp`, `wrapText`), then `drawJournal`'s four tabs |

## Part 2 - Trace three things end to end

**A. "I press E at door 101."**
`Game::run` reads `IsKeyPressed(KEY_E)` into `Input.interact` -> `Game::update` (mode WALK) -> `interact()` -> not a person, not in the lift, `renderer.focus` is a door -> `Level::doorInfoAt` -> `startKnock` (mode KNOCK, finds the `Resident` with `residentAt`).
Each frame: the player is eased in front of the door; `updateKnock` fires three knock beats (screen shake + caption), at 2.2 s prints the resident's `openLine`, at 2.9 s calls `Resident::appearAt`, sets `doorTgt = 1` and `startTalk` -> `Conversation::begin` (mode TALK).
Meanwhile `Game::update` moves `doorAmt` toward `doorTgt`; `Renderer::drawWallRows` draws the door leaf narrowing and `roomView` behind it; `collectBillboards` now includes the resident (`inDoorway`); `Ui::drawDialogue` draws the box.
You pick a question -> `Conversation::update` -> `choose` -> topic marked `done`, `ClueBook::give` (journal + toast) -> menu rebuilt (new questions may appear). Leaving -> `endTalk` sets `doorTgt = 0`; once the door is nearly shut the resident is hidden again.

**B. "I walk up the stairs."**
`Level::build` marked the last two steps of the upper flight with `link = +1`. `Game::updateWalk` sees `here.link != 0` -> `startFade(+1)`. `updateFade`: fade out 0.18 s; then `player.feet -= WALL_H` (the same physical spot is one storey lower on the next floor), `building.setCurrent`, fade in 0.30 s. The cells at the arrival spot have heights that agree because both floors were built from the **same** `FlightDef`.

**C. "A pixel on screen."**
`Renderer::render` -> 320 x `renderColumn` -> per cell ceiling/floor spans and a wall face -> `putPix` (light, fog, dither, grain) -> `pix[]` -> `Ui::draw` `UpdateTexture` -> `DrawTexturePro` scaled up.

## Part 3 - Exercises (do them in order)

Each ends with a **check**: something you can see to prove it worked.

### Level 1: change content (one file each)
**E1. Rewrite a line.** In `DialogueData.cpp` change Mrs. Volkova's first answer. *Check:* knock on 101, ask, read your text.

**E2. Recolour a person.** In `CharacterArt.cpp` change Misha's `main` colour in `castList()`. Then change a pixel key in `MAN_FRONT` (e.g. his hat `H` -> `R`). *Check:* the person in the doorway and in the journal's People page change.

**E3. Add a bulb and a window.** In `LevelData.cpp` add a `Bulb` in a dark corner and change one `#` in an outer wall row to `W`. *Check:* a new light pool; a new window with rain. *Learn:* tile vs cell coordinates (bulbs are in tile units, x.5 = centre of a tile).

### Level 2: content that touches several files
**E4. A new question that needs a clue.** Add a topic to Mr. Gromov in `DialogueData.cpp` whose `req` is `{"scarf"}`. *Check:* it appears only after Misha has told you about the scarf. *Learn:* gating is an AND over the `req` list.

**E5. A new clue in the graph.** In `ClueBook.cpp` add `ladder` (root clue) and a deduction `alibi` that requires `{"ladder", "stairs"}` with `autoF = true`. Make Gromov's "ladder" answer give `ladder`. *Check:* the Board shows a new card, new strings, and the deduction appears by itself when both are found. *Learn:* depth, topological order, auto-deductions.

**E6. A new person end to end.** Create a resident behind door 5 (105):
1. `CharacterArt.cpp`: add a `CastDef` with id `"lev"`, an existing design, your colours.
2. `DialogueData.cpp`: add an `else if (id == "lev")` script.
3. `LevelData.cpp`: add `{"lev", 0, 5}` to `homes`.
*Check:* knocking on 105 now opens. *Learn:* the id strings tie the three data files together; no engine code changed.

### Level 3: change the world
**E7. Move the lift or add a second apartment door.** Edit the rows of **both** floors so they stay aligned (same width, doors have one hallway side). *Check:* no crash; you can reach it. *Learn:* why both floors share a footprint.

**E8. Break it on purpose, then fix it.** (a) Make two clues require each other. Predict what `buildTopo` does, then run: `topo.size()` will be smaller than `ids.size()` (Kahn leaves cycle members unprocessed). Add a check that reports it. *This is a real DSA lesson: cycle detection with Kahn's algorithm.* (b) Change a flight so its two floors disagree and read the screen.

**E9. Renderer experiments** (`Common.h`, `Renderer.cpp`): change `PLANE` (field of view), the fog constant `16`, `LAMP_R`, the vignette. *Learn by seeing*, then revert.

### Level 4: stretch
**E10. A third floor.** Add a third `LevelDef` and make the second-floor "roof" flight link to it (it becomes a real link automatically when a floor exists above). Alternate which side the up flight is on. *Expect to hit and fix:* door labels, who lives where, the lift ride order.

## Part 4 - Self-check (can you answer these without looking?)
1. Why does one ray per column give the perpendicular distance without fish-eye correction?
2. What does `zbuf` do and who writes it?
3. Why can a staircase be drawn without any special renderer code?
4. Why are floors separate grids, and what hides the swap?
5. Which structure decides when a deduction card appears? Walk through `give`.
6. What is the difference between `Wanderer` and `Resident`?
7. Where does the game decide what clue a question gives? How would you make a question give two clues?
8. What would break if `STAIR_TILES` were 4?
9. Which three data files must agree when you add a person?
10. Which structures from `06_DSA.md` are done, which are planned, and which feature does each one power?

If you can answer 8 of 10 you own the project well enough to direct the next milestone.
