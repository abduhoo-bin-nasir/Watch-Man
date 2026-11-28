// AudioData.cpp - CONTENT: which sound lives behind which apartment door. Edit freely; the engine (Audio.cpp) never changes.
//   floor 0 = ground, 1 = second floor; door = apartment number (1..8, so 2 on floor 0 is flat 102)
//   sound: a name from Sounds::byName (tvMatch, radioChess, piano, opera, keyboard, fan, crtHum)
//   gain: loudness at the door, radius: how far (in cells) it carries before it halves.
#include "Audio.h"

const std::vector<AmbientDef>& ambientDefs() {
    static const std::vector<AmbientDef> defs = {
        {0, 2, "tvMatch",    0.60f, 3.4f},    // 102: a football match, too loud
        {0, 8, "radioChess", 0.50f, 3.0f},    // 108: Colonel Fedorov's radio announcing chess moves
        {1, 1, "opera",      0.50f, 3.4f},    // 201: Tamara humming
        {1, 2, "keyboard",   0.40f, 2.8f},    // 202: Kolya streaming
        {1, 6, "fan",        0.30f, 2.4f},    // 206: a fan (Zoya sleeps)
        {1, 7, "crtHum",     0.18f, 2.2f},    // 207: the television with the sound off
        {1, 8, "piano",      0.60f, 5.0f},    // 208: piano exercises, audible from the stairs
    };
    return defs;
}
