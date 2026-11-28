# 09 - Audio

Everything you hear is **generated in code at start-up** (about 2 seconds, on a second thread). No audio files, no licences.
Press **M** to mute / unmute.

## Files

| File | Role | Edit it to... |
|---|---|---|
| `Synth.h/.cpp` | Builds every sound: oscillators, noise, filters. 48 buffers, 44.1 kHz mono. | change what a sound *is* (rain, creak, piano...) |
| `Scheduler.h` | Hand-written **binary min-heap** `MinHeap<T>` and `EventScheduler` built on it. | add new kinds of delayed events |
| `Audio.h/.cpp` | `Mixer` (software mixer) and `Audio` (decides what you hear this frame). | change how loudness, muffling, panning work |
| `AudioData.cpp` | **Content**: which sound lives behind which apartment door. | put a sound behind a door (data only) |
| `Game.cpp` | `makeAudioScene()` fills a snapshot of the world each frame; `run()` opens the sound card. | - |

## How a frame works

1. `Game::update` ends by handing `Audio::update` an `AudioScene`: where you are, which way you face, game mode, journal page,
   lightning state, who is walking where, the building.
2. `Audio` works out the continuous layers (set as voice gains, smoothed on the audio thread):
   * **Rain through windows**: loudness follows the *nearest* window (`1 / (1 + (d / 2.2)^2)`); panned toward it.
   * **Rain and wind on the roof**: full outdoors, muffled in the stair hut. The building and the roof have different music.
   * **A bad bulb**, **the lift hum**, **Boris purring** (follows the cat), and **things behind doors** (`AudioData.cpp`).
   * Each positional sound: gain falls with distance, **walls make it quieter and duller** (a line through the grid counts walls
     and closed doors; open doors and rails are free), another floor is quieter and duller still, and it is panned left/right.
   * A sound behind a **shut door** is muffled; when the door opens it clears; through the **peephole** it is clearest.
3. It notices what **just happened** by comparing with the last frame: a door started opening/closing (creak), you stepped
   (every 1.35 cells), someone else stepped (they fade in and out as they pass; steps overhead are dull thuds), a knock, the journal
   opened or turned a page, a clue banner appeared, you looked through a peephole.
4. **Lightning** schedules its **thunder 0.3 - 2.9 s later** through the scheduler. The scheduler also fires Boris's meows (every
   18-45 s), a distant door slam (40-90 s) and knocking in the pipes (55-130 s).

## Why a min-heap (for the DSA report)

Events are pushed in any order and popped in time order: push/pop `O(log n)`, "when is the next one?" `O(1)`.
Removal test: without it every delayed sound would be its own timer and nothing could be added without touching `Game.cpp`.
The same structure will drive the building's schedules, the lift cut at 22:00 and the Watch's outbursts. Tested in
`scheduler_test` against `std::priority_queue` (200,000 random operations, ordering, same-time ties, early release).

## Tuning

* How far a sound carries / how loud: `AudioData.cpp` (per door) and the numbers in `Audio::update` (`world`, `mScale`, base gains).
* Footstep distance between steps: `1.35f` (you), `1.15f` (people), `0.6f` (cat) in `Audio::update`.
* Thunder delay: `0.3 + 2.6 * random` in `Audio::update`.
* What a sound sounds like: the matching function in `Synth.cpp`.
* Listen without running the game: `ctest` writes every sound to `build/test_output/sounds/*.wav`.

## Tests

`scheduler_test`, `synth_test` (finite, not clipped, loops seamless), `mixer_test`, `audio_test` (the rules above against the real
building), `game_audio_test` (the real game: steps, knock + creak, journal pages, roof, piano through the peephole).

## Known limits

* Everything was designed with measurements and spectrograms, **not by ear**. Levels and taste need your ears: all gains are in one
  place each (above).
* Speech is murmur, not words (TV commentator, chess radio). Stereo only, no reverb: steps in the stairwell do not echo yet.
* The Watch's voice (milestone M6) will plug into the same mixer as one more voice.
