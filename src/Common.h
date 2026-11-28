// Common.h - constants, tiny math helpers and the few types every module shares.
#pragma once
#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using std::string;
using std::vector;

// ---- rendering and world scale
constexpr int RW = 320, RH = 180;            // internal render resolution (scaled up with nearest filtering)
constexpr float WALL_H = 2.0f;               // floor-to-ceiling height, in cells (one storey)
constexpr float EYE_H = 1.0f;                // eye height above the floor
constexpr float STEP_H = 0.2f;               // height of one stair step (one step per cell)
constexpr int STAIR_TILES = 5;               // a flight is 5 tiles = 10 steps of STEP_H = exactly one storey
static_assert((int)(STAIR_TILES * 2 * STEP_H * 100 + 0.5f) == (int)(WALL_H * 100 + 0.5f), "a flight must rise exactly one storey");
constexpr float LAMP_R = 7.2f;               // radius of a bulb's light pool, in cells
constexpr float PLANE = 0.5f * RW / RH;      // camera plane length (field of view)
constexpr float PITCH_MAX = 0.30f * RH;      // look up/down limit in screen rows

// ---- small value types
struct RGB { float r, g, b; };
struct V2 { float x, y; };
inline RGB mul(RGB c, float k) { return {c.r * k, c.g * k, c.b * k}; }
inline float frac(float v) { return v - std::floor(v); }
inline float hs(int x, int y) { float n = std::sin(x * 127.1f + y * 311.7f) * 43758.5453f; return n - std::floor(n); }
inline uint32_t hash32(uint32_t a, uint32_t b, uint32_t c) {
    uint32_t h = a * 374761393u + b * 668265263u + c * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
inline int clampi(float f, int lo, int hi) {   // NaN-safe float -> clamped int
    if (!(f > (float)lo)) return lo;
    if (f >= (float)hi) return hi;
    return (int)f;
}

// ---- which pixel-art design a character or prop uses (the designs live in CharacterArt.cpp)
enum ArtId { ART_MAN, ART_OLD, ART_CAT, ART_CRATE, ART_JANITOR, ART_WOMAN, ART_TANK, ART_ANTENNA,
              ART_TABLE, ART_SOFA, ART_WARDROBE, ART_BED, ART_LAMP, ART_TV, ART_PIANO, ART_CHAIR,   // furniture seen through peepholes
              ART_COUNT };
constexpr float SKY_H = 40.0f;        // ceiling height of an outdoor cell: open sky (nothing is drawn there but the night)
constexpr float PEEP_FOV = 2.1f;      // horizontal field of view through a peephole (radians, about 120 degrees)
constexpr float PARAPET_H = 1.2f;     // height of the roof's edge wall; above it you look out over the city

// ---- one frame of player input, gathered by Game::run() and consumed by Game::update()
struct Input {
    float turn = 0, fw = 0, st = 0, mouseX = 0, mouseY = 0, pitchKey = 0;
    bool interact = false, recenter = false, peep = false;
    bool up = false, down = false, left = false, right = false, enter = false, esc = false, journal = false;
    int num = 0, pick = -1, hover = -1, tabPick = -1;       // number key pressed, clicked menu item, hovered menu item, clicked journal tab
};

inline const Color COL_ME{232, 176, 74, 255}, COL_SYS{110, 125, 135, 255};
