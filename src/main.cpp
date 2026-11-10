// Block 7 - pixel-art raycaster (C++17 + raylib 5.x)
//
// Rendering technique: a column renderer in the style of Doom/Build. Every grid cell has its own floor and
// ceiling height. For each screen column a ray walks the grid cell by cell (DDA) and keeps a "window" [yT, yB)
// of rows that are still undrawn. Per cell it draws the ceiling span, the floor span, then the face between this
// cell and the next one (a wall, a stair riser or a ceiling step). Stairs are therefore real steps - there is no
// special case for them in the renderer.
//
// Controls: W/S move, A/D strafe, Left/Right turn, PgUp/PgDn look up/down (Home = recentre),
//           TAB = mouse look on/off, E = interact, ESC = quit
#include "raylib.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using std::string;
using std::vector;

// ---------------------------------------------------------------- constants
constexpr int RW = 320, RH = 180;            // internal render resolution (scaled up with nearest filtering)
constexpr int TILES_X = 36, TILES_Y = 15;    // map size in tiles
constexpr int GW = TILES_X * 2, GH = TILES_Y * 2;   // a tile is 2x2 cells, so a stair step can be half a tile deep
constexpr float WALL_H = 2.0f;               // floor-to-ceiling height, in cells
constexpr float EYE_H = 1.0f;                // eye height above the floor
constexpr float STEP_H = 0.2f;               // height of one stair step (one step per cell)
constexpr float LAMP_R = 7.2f;               // radius of a bulb's light pool, in cells
constexpr float PLANE = 0.5f * RW / RH;      // camera plane length (field of view)

// ---------------------------------------------------------------- world grid
enum CellType : uint8_t { C_OPEN, C_WALL, C_DOOR, C_WINDOW, C_RAIL };
struct Cell {
    CellType type = C_WALL;
    bool stair = false;
    uint8_t door = 0;      // 1..8 for doors
    uint8_t sx = 0, sy = 0; // which half of its tile this cell is (for texture mapping)
    float F = 0, C = WALL_H; // floor and ceiling height
};
static Cell gGrid[GH][GW];
static const Cell kSolid{};
static const Cell& cellAt(int x, int y) {
    if (x < 0 || y < 0 || x >= GW || y >= GH) return kSolid;   // outside the map is solid
    return gGrid[y][x];
}

// One apartment floor (16x15 tiles). The same layout is used for both floors.
static const char* BASE[TILES_Y] = {
    "###W###W###W####", "#..............#", "#.####1##2####.#", "W.############.#", "#.3##########4.#",
    "#.############.#", "#.############.#", "#.5##########6.#", "#.############.#", "#.############.W",
    "#.############.#", "#.############.#", "#.####7##8####.#", "#..............#", "###W###W###W####"};

// World = ground floor (tiles 0..15) + a 4-tile stair run (16..19) + upper floor (tiles 20..35).
// The flight starts in the ground floor's east wall (tile 15) and rises east until it reaches the upper floor's
// west wall (tile 20), so both floors live in one continuous grid and the stairs are walkable geometry.
static void buildWorld() {
    for (int ty = 0; ty < TILES_Y; ty++) {
        string row(BASE[ty]);
        if (ty == 5 || ty == 7) row[15] = 'k'; else if (ty == 6) row[15] = 'u';
        row += (ty == 6) ? "uuuu" : (ty == 5 || ty == 7) ? "kkkk" : "####";
        string up(BASE[ty]);
        if (ty == 5 || ty == 7) up[0] = 'k'; else if (ty == 6) up[0] = '.';
        row += up;
        for (int tx = 0; tx < TILES_X; tx++) {
            char ch = row[tx];
            for (int j = 0; j < 2; j++)
                for (int i = 0; i < 2; i++) {
                    int cx = tx * 2 + i, cy = ty * 2 + j;
                    Cell c;
                    c.sx = (uint8_t)(cx & 1); c.sy = (uint8_t)(cy & 1);
                    switch (ch) {
                        case '.': case 'u':
                            c.type = C_OPEN; c.stair = (ch == 'u');
                            c.F = (ch == 'u') ? STEP_H * (float)(cx - 29) : (tx >= 20 ? WALL_H : 0.0f);
                            c.C = c.F + WALL_H;
                            break;
                        case 'W': c.type = C_WINDOW; break;
                        case 'k': c.type = C_RAIL; break;
                        case '#': c.type = C_WALL; break;
                        default:
                            if (ch >= '1' && ch <= '8') { c.type = C_DOOR; c.door = (uint8_t)(ch - '0'); }
                            break;
                    }
                    gGrid[cy][cx] = c;
                }
        }
    }
}

// ---------------------------------------------------------------- small helpers
struct RGB { float r, g, b; };
static RGB mul(RGB c, float k) { return {c.r * k, c.g * k, c.b * k}; }
static float frac(float v) { return v - std::floor(v); }
static float hs(int x, int y) { float n = std::sin(x * 127.1f + y * 311.7f) * 43758.5453f; return n - std::floor(n); }
static inline uint32_t hash32(uint32_t a, uint32_t b, uint32_t c) {
    uint32_t h = a * 374761393u + b * 668265263u + c * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static inline int clampi(float f, int lo, int hi) {   // NaN-safe float -> clamped int
    if (!(f > (float)lo)) return lo;
    if (f >= (float)hi) return hi;
    return (int)f;
}
static int toRow(float y) {                            // screen y -> row index, safe for huge/NaN values
    if (!(y > -1e6f)) return -1000000;
    if (y > 1e6f) return 1000000;
    return (int)std::ceil(y);
}

// ---------------------------------------------------------------- procedural textures
struct Tex { int w = 0, h = 0; vector<uint8_t> d; };
template <class F> static Tex makeTex(int w, int h, F f) {
    Tex t; t.w = w; t.h = h; t.d.resize((size_t)w * h * 3);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            RGB c = f(x, y, w, h);
            size_t i = ((size_t)y * w + x) * 3;
            t.d[i] = (uint8_t)std::clamp(c.r, 0.0f, 255.0f);
            t.d[i + 1] = (uint8_t)std::clamp(c.g, 0.0f, 255.0f);
            t.d[i + 2] = (uint8_t)std::clamp(c.b, 0.0f, 255.0f);
        }
    return t;
}
static const uint8_t* texAt(const Tex& t, float u, float v) {
    int x = clampi(u * (float)t.w, 0, t.w - 1), y = clampi(v * (float)t.h, 0, t.h - 1);
    return &t.d[((size_t)y * t.w + x) * 3];
}

static const int DG[10][5] = {{7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
                              {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7}};
static Tex TW, TR, TF, TC, TS, TRISER, DT[2][9];

static void initTextures() {
    auto wallFn = [](int x, int y, int, int h) -> RGB {
        float v = (float)y / h, k = 0.88f + 0.24f * hs(x, y);
        RGB c;
        if (v > 0.52f && v < 0.57f) c = {112, 124, 130};                       // trim line
        else if (v >= 0.57f) { c = {62, 84, 92}; if (hs(x >> 2, 9) > 0.9f) k *= 0.8f; }   // painted lower wall
        else { c = {150, 158, 160}; if (hs(x >> 3, 3) > 0.86f) k *= 0.55f + 0.45f * v / 0.52f; }  // plaster + stains
        return mul(c, k);
    };
    TW = makeTex(64, 64, wallFn);
    TR = makeTex(64, 64, [&](int x, int y, int w, int h) -> RGB {              // wall with a brass handrail band
        float v = (float)y / h;
        if (v > 0.40f && v < 0.45f) return mul({176, 132, 62}, 0.9f + 0.2f * hs(x, y));
        return wallFn(x, y, w, h);
    });
    TF = makeTex(64, 64, [](int x, int y, int, int) -> RGB {                   // grimy checkered lino
        float k = 0.9f + 0.2f * hs(x, y);
        if (x % 32 < 1 || y % 32 < 1) k *= 0.55f;
        if (hs(x >> 3, y >> 3) > 0.9f) k *= 0.7f;
        return mul(((x >> 5) + (y >> 5)) & 1 ? RGB{62, 74, 76} : RGB{50, 62, 64}, k);
    });
    TC = makeTex(64, 64, [](int x, int y, int, int) -> RGB {
        float k = 0.9f + 0.2f * hs(x, y);
        if (x % 32 < 1 || y % 32 < 1) k *= 0.6f;
        return mul({68, 70, 76}, k);
    });
    TS = makeTex(64, 64, [](int x, int y, int, int) -> RGB {                   // stair tread with a worn nosing
        bool nose = x < 7;
        return mul(nose ? RGB{176, 156, 112} : RGB{88, 82, 74}, (0.9f + 0.2f * hs(x, y)) * (nose ? 1.0f : 0.85f + 0.15f * x / 64.0f));
    });
    TRISER = makeTex(64, 16, [](int x, int y, int, int h) -> RGB {             // stair riser
        float v = (float)y / h;
        return mul({104, 98, 86}, (0.85f + 0.3f * hs(x, y)) * (1.0f - 0.4f * v));
    });
    for (int f = 0; f < 2; f++)
        for (int n = 1; n <= 8; n++) {
            string lab = std::to_string(f + 1) + "0" + std::to_string(n);
            bool tape = (f == 0 && n == 6);                                     // crime scene
            DT[f][n] = makeTex(64, 64, [lab, tape](int x, int y, int w, int h) -> RGB {
                float u = (float)x / w, v = (float)y / h;
                RGB c = {132, 74, 40};
                if (u < 0.08f || u > 0.92f || v < 0.04f) c = {92, 50, 28};
                else if (u > 0.18f && u < 0.82f && ((v > 0.34f && v < 0.52f) || (v > 0.60f && v < 0.94f))) c = {162, 94, 50};
                if (v > 0.07f && v < 0.27f && u > 0.27f && u < 0.73f) c = {192, 186, 150};   // number plate
                int gy = (int)(h * 0.1f);
                if (y >= gy && y < gy + 10)
                    for (int k = 0; k < 3; k++) {
                        int gx = w / 2 - 11 + k * 8;
                        if (x >= gx && x < gx + 6 && ((DG[lab[k] - '0'][(y - gy) >> 1] >> (2 - ((x - gx) >> 1))) & 1)) c = {44, 38, 32};
                    }
                if (u > 0.74f && u < 0.80f && v > 0.50f && v < 0.56f) c = {236, 200, 110};   // knob
                if (tape && ((v > 0.38f && v < 0.45f) || (v > 0.62f && v < 0.69f))) c = (((x + y) >> 2) & 1) ? RGB{232, 188, 40} : RGB{28, 28, 30};
                return mul(c, 0.92f + 0.16f * hs(x, y));
            });
        }
}

// ---------------------------------------------------------------- frame buffer + lighting
static uint32_t gPix[RW * RH];
static float gVig[RW * RH];
static float gZ[RW];
static uint32_t gFrame = 0;
static float gCold = 0;                    // set by lampL: cold flash light at the sample point
static const int BAY[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

static inline void putPix(int x, int y, float r, float g, float b, float L, float f) {
    if ((unsigned)x >= (unsigned)RW || (unsigned)y >= (unsigned)RH) return;
    L = std::min(L, 1.5f);
    f *= gVig[y * RW + x];
    // ordered dither + a little film grain, then 5-bit colour for the retro look
    float n = (BAY[((y & 3) << 2) | (x & 3)] - 7.5f) * 1.5f + ((int)(hash32(x, y, gFrame) & 15) - 7.5f) * 0.35f;
    float cl = gCold;                     // cold light from a lightning flash coming through a nearby window
    r = (r * (0.12f + L) + r * cl * 0.22f) * f + n;          // cool ambient, warm lamp light
    g = (g * (0.15f + L * 0.78f) + g * cl * 0.40f) * f + n;
    b = (b * (0.21f + L * 0.50f) + b * cl * 0.80f) * f + n;
    auto q = [](float v) { return (uint32_t)((int)std::clamp(v, 0.0f, 255.0f) & 248); };
    gPix[y * RW + x] = 0xff000000u | (q(b) << 16) | (q(g) << 8) | q(r);
}
// a pixel that is not lit by the hallway lamps (the view outside the window); still gets vignette, dither and grain
static inline void putFlat(int x, int y, float r, float g, float b) {
    if ((unsigned)x >= (unsigned)RW || (unsigned)y >= (unsigned)RH) return;
    float f = gVig[y * RW + x];
    float n = (BAY[((y & 3) << 2) | (x & 3)] - 7.5f) * 1.5f + ((int)(hash32(x, y, gFrame) & 15) - 7.5f) * 0.35f;
    auto q = [](float v) { return (uint32_t)((int)std::clamp(v, 0.0f, 255.0f) & 248); };
    gPix[y * RW + x] = 0xff000000u | (q(b * f + n) << 16) | (q(g * f + n) << 8) | q(r * f + n);
}
// additive light (bulb glow); the vignette is applied too so glows at the screen edge do not pop
static inline void addLight(int x, int y, float r, float g, float b) {
    if ((unsigned)x >= (unsigned)RW || (unsigned)y >= (unsigned)RH) return;
    uint32_t p = gPix[y * RW + x];
    float f = 0.55f + 0.45f * gVig[y * RW + x];
    float n = (BAY[((y & 3) << 2) | (x & 3)] - 7.5f) * 1.2f;
    auto q = [](float v) { return (uint32_t)((int)std::clamp(v, 0.0f, 255.0f) & 248); };
    float pr = (float)(p & 255) + r * f + n, pg = (float)((p >> 8) & 255) + g * f + n, pb = (float)((p >> 16) & 255) + b * f + n;
    gPix[y * RW + x] = 0xff000000u | (q(pb) << 16) | (q(pg) << 8) | q(pr);
}

struct Lamp { float x, y, k; float r = LAMP_R; bool win = false; };   // position in cells, k = brightness (flicker); win = cold flash light by a window
static vector<Lamp> gLamps;
static float gBulb = 0;                    // set by lampL when the sample point is right under a bulb

static float lampL(float wx, float wy) {   // sum of light pools around the bulbs (also sets gCold)
    float s = 0;
    gCold = 0;
    for (const Lamp& l : gLamps) {
        if (l.k <= 0.002f) continue;
        float ax = wx - l.x, ay = wy - l.y;
        if (ax > l.r || ax < -l.r) continue;
        float d = ax * ax + ay * ay;
        if (d < l.r * l.r) {
            float k = 1.0f - d / (l.r * l.r);
            if (l.win) { gCold += k * k * l.k; continue; }
            if (d < 0.03f && l.k > gBulb) gBulb = l.k;
            s += k * k * l.k;
        }
    }
    return s;
}

// ---------------------------------------------------------------- sprites (people, props)
// Sprites are drawn procedurally on a small canvas; an outline and cel shading (light from the top left) are then
// added automatically, so every character gets the same readable pixel-art treatment.
// Colour keys: C/K main colour, Q dark main, R accent, S skin, H hat/hair, P trousers, B boots, W white, Y yellow,
// G leaf green, T brown, Z metal, N pink, F glowing eyes, E eyes, O outline.
struct SpriteArt { int w = 0, h = 0; vector<char> c; vector<int8_t> s; };   // colour key + shade (-1, 0, +1) per pixel
struct Canvas {
    int w, h, ox = 0;                     // ox shifts everything right (room for accessories that get mirrored on the back view)
    vector<char> g, d;                    // g = shaded shapes, d = flat details drawn on top
    Canvas(int w_, int h_, int ox_ = 0) : w(w_), h(h_), ox(ox_), g((size_t)w_ * h_, ' '), d((size_t)w_ * h_, ' ') {}
    void px(int x, int y, char ch) { x += ox; if (x >= 0 && y >= 0 && x < w && y < h) g[(size_t)y * w + x] = ch; }
    void rect(int x0, int y0, int x1, int y1, char ch) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) px(x, y, ch); }
    void ell(float cx, float cy, float rx, float ry, char ch) {
        for (int y = 0; y < h; y++) for (int x = -ox; x < w - ox; x++) { float a = (x - cx) / rx, b = (y - cy) / ry; if (a * a + b * b <= 1.0f) px(x, y, ch); }
    }
    void det(int x, int y, char ch) { x += ox; if (x >= 0 && y >= 0 && x < w && y < h) d[(size_t)y * w + x] = ch; }
    void drect(int x0, int y0, int x1, int y1, char ch) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) det(x, y, ch); }
    // mirrored copies (about x = cm / 2) for things that sit on one side of the body: they swap sides on the back view
    void mrect(bool mir, int cm, int x0, int y0, int x1, int y1, char ch) { if (mir) rect(cm - x1, y0, cm - x0, y1, ch); else rect(x0, y0, x1, y1, ch); }
    void mdet(bool mir, int cm, int x, int y, char ch) { det(mir ? cm - x : x, y, ch); }
    SpriteArt finish() const {
        SpriteArt a; a.w = w; a.h = h; a.c.assign((size_t)w * h, ' '); a.s.assign((size_t)w * h, 0);
        auto G = [&](int x, int y) -> char {
            if (x < 0 || y < 0 || x >= w || y >= h) return ' ';
            size_t i = (size_t)y * w + x;
            return d[i] != ' ' ? d[i] : g[i];
        };
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                size_t i = (size_t)y * w + x;
                if (d[i] != ' ') { a.c[i] = d[i]; continue; }
                char me = g[i];
                if (me == ' ') continue;
                a.c[i] = me;
                bool shadow = G(x + 1, y) != me || G(x, y + 1) != me;        // edge facing away from the light
                bool light = G(x - 1, y) == ' ' || G(x, y - 1) == ' ';       // silhouette edge facing the light
                a.s[i] = shadow ? -1 : (light ? 1 : 0);
            }
        vector<char> src = a.c;                                              // outline: empty pixels touching a filled one
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                size_t i = (size_t)y * w + x;
                if (src[i] != ' ') continue;
                bool near = (x > 0 && src[i - 1] != ' ') || (x < w - 1 && src[i + 1] != ' ') || (y > 0 && src[i - w] != ' ') || (y < h - 1 && src[i + w] != ' ');
                if (near) a.c[i] = 'O';
            }
        return a;
    }
};
enum Art { A_MAN, A_OLD, A_CAT, A_PLANT, A_BIN, A_CRATE, A_JANITOR };
// Every sprite is two crossed vertical planes (like grass in Minecraft): one plane faces along the character's heading
// and shows the front or the back (whichever side you are on), the other is perpendicular and shows the side
// (mirrored when seen from the other side). Side art always faces LEFT in the image.
struct SpriteSet { SpriteArt front, back, side; };
static vector<SpriteSet> ART;
static SpriteSet sameArt(const SpriteArt& a) { return {a, a, a}; }

static SpriteSet makeMan() {   // long coat, fur hat with ear flaps, red scarf with a tail down the chest
    SpriteSet S;
    for (int v = 0; v < 3; v++) {
        bool back = v == 1, side = v == 2;
        Canvas m(side ? 14 : 18, 34);
        if (!side) {
            m.rect(5, 28, 7, 31, 'P'); m.rect(10, 28, 12, 31, 'P');
            m.rect(4, 32, 7, 32, 'B'); m.rect(10, 32, 13, 32, 'B');
            m.rect(4, 12, 13, 27, 'C'); m.rect(2, 13, 3, 25, 'C'); m.rect(14, 13, 15, 25, 'C');
            m.rect(2, 26, 3, 27, 'S'); m.rect(14, 26, 15, 27, 'S');
            m.rect(6, 6, 11, 9, back ? 'H' : 'S');
            m.rect(5, 10, 12, 12, 'R'); if (!back) m.rect(9, 13, 11, 20, 'R');
            m.rect(5, 1, 12, 3, 'H'); m.rect(4, 4, 13, 5, 'H'); m.rect(4, 6, 5, 9, 'H'); m.rect(12, 6, 13, 9, 'H');
            if (!back) {
                m.det(7, 7, 'E'); m.det(10, 7, 'E'); m.det(8, 9, 'O'); m.det(9, 9, 'O');
                m.drect(8, 21, 8, 27, 'O'); m.drect(5, 22, 6, 22, 'O'); m.drect(11, 22, 12, 22, 'O');
                m.det(7, 15, 'Y'); m.det(7, 18, 'Y');
            } else {
                m.drect(8, 13, 8, 27, 'O'); m.drect(8, 24, 9, 24, 'O');            // centre seam and a vent
            }
        } else {
            m.rect(4, 28, 6, 31, 'P'); m.rect(7, 28, 9, 31, 'P');
            m.rect(2, 32, 6, 32, 'B'); m.rect(7, 32, 10, 32, 'B');
            m.rect(3, 12, 10, 27, 'C');
            m.rect(3, 10, 10, 12, 'R'); m.rect(10, 13, 11, 20, 'R');
            m.rect(5, 13, 8, 25, 'C'); m.rect(5, 26, 8, 27, 'S');
            m.rect(4, 6, 9, 9, 'S'); m.px(3, 8, 'S');
            m.rect(3, 1, 10, 3, 'H'); m.rect(3, 4, 10, 5, 'H'); m.rect(7, 6, 9, 9, 'H');
            m.det(5, 7, 'E'); m.drect(5, 22, 6, 22, 'O');
        }
        (v == 0 ? S.front : v == 1 ? S.back : S.side) = m.finish();
    }
    return S;
}
static SpriteSet makeOld() {   // headscarf, shawl, cardigan, long skirt, handbag
    SpriteSet S;
    for (int v = 0; v < 3; v++) {
        bool back = v == 1, side = v == 2;
        Canvas o(side ? 14 : 22, 30, side ? 0 : 2);
        if (!side) {
            o.rect(5, 27, 7, 28, 'B'); o.rect(10, 27, 12, 28, 'B');
            o.rect(5, 19, 12, 26, 'P'); o.rect(4, 23, 13, 26, 'P');
            o.rect(5, 11, 12, 20, 'C'); o.rect(3, 12, 4, 21, 'C'); o.rect(13, 12, 14, 21, 'C');
            o.rect(3, 22, 4, 23, 'S'); o.rect(13, 22, 14, 23, 'S');
            o.mrect(back, 17, 15, 24, 17, 28, 'T');                                   // handbag: other hand on the back view
            o.rect(4, 10, 13, 13, 'R'); if (!back) o.rect(5, 14, 7, 16, 'R');
            o.rect(6, 5, 11, 9, back ? 'R' : 'S');
            o.rect(5, 2, 12, 4, 'R'); o.rect(5, 5, 5, 10, 'R'); o.rect(12, 5, 12, 10, 'R');
            o.mdet(back, 17, 15, 23, 'O'); o.mdet(back, 17, 16, 23, 'O');
            if (!back) { o.px(6, 5, 'W'); o.px(11, 5, 'W'); o.det(7, 7, 'E'); o.det(10, 7, 'E'); o.det(8, 9, 'O'); o.det(9, 9, 'O'); }
            else { o.px(7, 10, 'W'); o.px(10, 10, 'W'); o.det(8, 4, 'O'); }                // grey wisps and the knot of the scarf
        } else {
            o.rect(2, 27, 6, 28, 'B');
            o.rect(3, 19, 10, 26, 'P'); o.rect(2, 23, 11, 26, 'P');
            o.rect(3, 11, 10, 20, 'C'); o.rect(9, 10, 11, 15, 'R');
            o.rect(3, 10, 10, 13, 'R');
            o.rect(5, 12, 8, 21, 'C'); o.rect(5, 22, 8, 23, 'S'); o.rect(3, 24, 7, 28, 'T');
            o.rect(4, 5, 9, 9, 'S'); o.px(3, 8, 'S');
            o.rect(3, 2, 10, 4, 'R'); o.rect(6, 5, 10, 10, 'R');
            o.det(5, 7, 'E');
        }
        (v == 0 ? S.front : v == 1 ? S.back : S.side) = o.finish();
    }
    return S;
}
static SpriteSet makeJanitor() {   // cap, moustache, work shirt, overalls, broom in the right hand
    SpriteSet S;
    for (int v = 0; v < 3; v++) {
        bool back = v == 1, side = v == 2;
        Canvas j(side ? 16 : 24, 34, side ? 0 : 2);
        if (!side) {
            j.mrect(back, 19, 18, 3, 18, 30, 'T'); j.mrect(back, 19, 17, 30, 19, 32, 'Y');           // broom
            j.rect(5, 26, 8, 31, 'C'); j.rect(11, 26, 14, 31, 'C');
            j.rect(4, 32, 8, 32, 'B'); j.rect(11, 32, 15, 32, 'B');
            j.rect(5, 11, 14, 26, 'R'); j.rect(2, 12, 4, 23, 'R'); j.rect(15, 12, 17, 23, 'R');
            j.rect(6, 14, 13, 26, 'C'); j.rect(6, 12, 7, 13, 'C'); j.rect(12, 12, 13, 13, 'C');
            j.rect(2, 24, 4, 25, 'S'); j.mrect(back, 19, 16, 20, 18, 21, 'S');
            j.rect(8, 11, 11, 11, 'S'); j.rect(7, 6, 12, 10, back ? 'H' : 'S');
            j.rect(7, 2, 12, 3, 'H'); j.rect(6, 4, 13, 5, 'H'); if (!back) j.rect(5, 5, 9, 5, 'H');
            if (!back) { j.det(8, 8, 'E'); j.det(11, 8, 'E'); j.drect(8, 9, 11, 9, 'H'); j.drect(8, 18, 11, 18, 'O'); j.det(7, 15, 'Y'); j.det(12, 15, 'Y'); }
            else { for (int t = 0; t < 8; t++) { j.det(7 + t, 14 + t, 'O'); j.det(12 - t, 14 + t, 'O'); } }   // crossed overall straps
        } else {
            j.rect(2, 3, 2, 30, 'T'); j.rect(1, 30, 3, 32, 'Y');
            j.rect(5, 26, 7, 31, 'C'); j.rect(8, 26, 10, 31, 'C');
            j.rect(3, 32, 7, 32, 'B'); j.rect(8, 32, 11, 32, 'B');
            j.rect(4, 11, 11, 26, 'R'); j.rect(4, 14, 11, 26, 'C'); j.rect(5, 12, 6, 13, 'C');
            j.rect(5, 12, 8, 23, 'R'); j.rect(2, 20, 5, 21, 'S');
            j.rect(6, 11, 9, 11, 'S'); j.rect(5, 6, 10, 10, 'S'); j.px(4, 9, 'S');
            j.rect(5, 2, 10, 3, 'H'); j.rect(4, 4, 10, 5, 'H'); j.rect(3, 5, 5, 5, 'H');
            j.det(6, 8, 'E'); j.det(5, 9, 'H'); j.det(6, 9, 'H');
        }
        (v == 0 ? S.front : v == 1 ? S.back : S.side) = j.finish();
    }
    return S;
}
static SpriteSet makeCat() {   // tabby, red collar, glowing eyes; the side view faces left
    SpriteSet S;
    {   Canvas f(12, 16);
        f.ell(6, 11, 4, 3.2f, 'K'); f.ell(6, 6, 3.6f, 3.2f, 'K');
        f.rect(2, 2, 3, 4, 'K'); f.rect(9, 2, 10, 4, 'K');
        f.rect(3, 11, 4, 14, 'K'); f.rect(7, 11, 8, 14, 'K'); f.rect(3, 14, 4, 14, 'W'); f.rect(7, 14, 8, 14, 'W');
        f.rect(5, 10, 6, 12, 'W'); f.rect(6, 3, 6, 4, 'Q'); f.px(4, 3, 'Q'); f.px(8, 3, 'Q');
        f.rect(3, 9, 8, 9, 'R');
        f.det(4, 6, 'F'); f.det(8, 6, 'F'); f.det(6, 7, 'N'); f.det(3, 3, 'N'); f.det(9, 3, 'N'); f.det(6, 10, 'Y');
        S.front = f.finish(); }
    {   Canvas b(12, 16);
        b.ell(6, 10, 4.3f, 4.2f, 'K'); b.ell(6, 5, 3.3f, 2.8f, 'K');
        b.rect(2, 1, 3, 3, 'K'); b.rect(9, 1, 10, 3, 'K');
        b.rect(3, 12, 5, 14, 'K'); b.rect(7, 12, 9, 14, 'K'); b.rect(3, 14, 5, 14, 'W'); b.rect(7, 14, 9, 14, 'W');
        b.rect(9, 5, 10, 10, 'K'); b.rect(10, 3, 10, 4, 'K'); b.px(10, 3, 'Q');
        b.rect(5, 6, 5, 9, 'Q'); b.rect(7, 6, 7, 9, 'Q'); b.rect(4, 11, 8, 11, 'Q');
        b.rect(4, 7, 8, 7, 'R');
        S.back = b.finish(); }
    {   Canvas k(24, 16);
        k.ell(12, 9, 6.5f, 3.2f, 'K'); k.ell(5, 7, 3.3f, 3.0f, 'K');
        k.rect(2, 2, 3, 4, 'K'); k.rect(6, 2, 7, 4, 'K');
        const int tail[][2] = {{18, 9}, {19, 8}, {20, 7}, {21, 6}, {21, 5}, {21, 4}, {21, 3}, {20, 2}, {19, 2}};
        for (auto& t : tail) k.rect(t[0], t[1], t[0] + 1, t[1], 'K');
        k.rect(4, 11, 5, 14, 'K'); k.rect(7, 11, 8, 14, 'K'); k.rect(15, 11, 16, 14, 'K'); k.rect(17, 11, 18, 14, 'K');
        k.rect(4, 14, 5, 14, 'W'); k.rect(7, 14, 8, 14, 'W'); k.rect(15, 14, 16, 14, 'W'); k.rect(17, 14, 18, 14, 'W');
        for (int x : {10, 12, 14, 16}) k.rect(x, 6, x, 8, 'Q');
        k.px(21, 4, 'Q'); k.px(21, 6, 'Q'); k.px(5, 4, 'Q'); k.px(5, 5, 'Q');
        k.rect(8, 7, 8, 10, 'R');
        k.det(3, 6, 'F'); k.det(6, 6, 'F'); k.det(2, 8, 'N'); k.det(3, 3, 'N'); k.det(7, 3, 'N');
        k.det(8, 11, 'R'); k.det(9, 12, 'R');
        S.side = k.finish(); }
    return S;
}
static void buildArt() {
    ART.clear();
    ART.push_back(makeMan());
    ART.push_back(makeOld());
    ART.push_back(makeCat());
    {   // A_PLANT: potted leafy plant (same on every side)
        Canvas pl(14, 22);
        pl.ell(7, 8, 3, 6, 'G'); pl.ell(3.5f, 10, 2.5f, 4.5f, 'G'); pl.ell(10.5f, 10, 2.5f, 4.5f, 'G');
        pl.ell(5, 5, 2, 3.5f, 'G'); pl.ell(9, 5, 2, 3.5f, 'G');
        pl.rect(2, 14, 11, 15, 'T'); pl.rect(3, 16, 10, 19, 'T'); pl.rect(4, 20, 9, 20, 'T');
        pl.drect(7, 3, 7, 12, 'O');
        ART.push_back(sameArt(pl.finish()));
    }
    {   // A_BIN: dented bin with a lid and spilled rubbish
        Canvas bn(14, 19);
        bn.rect(5, 1, 8, 1, 'C'); bn.rect(1, 2, 12, 4, 'C'); bn.rect(2, 5, 11, 16, 'C'); bn.rect(3, 17, 10, 17, 'C');
        bn.drect(3, 8, 10, 8, 'O'); bn.drect(3, 11, 10, 11, 'O'); bn.drect(3, 14, 10, 14, 'O');
        bn.det(11, 4, 'W'); bn.det(12, 5, 'W');
        ART.push_back(sameArt(bn.finish()));
    }
    {   // A_CRATE: wooden crate with a diagonal brace and nails
        Canvas cr(18, 15);
        cr.rect(1, 1, 16, 13, 'C');
        cr.drect(1, 5, 16, 5, 'O'); cr.drect(1, 9, 16, 9, 'O');
        for (int t = 0; t < 12; t++) cr.det(3 + t, 11 - (t * 7) / 11, 'O');
        cr.det(2, 2, 'Y'); cr.det(15, 2, 'Y'); cr.det(2, 12, 'Y'); cr.det(15, 12, 'Y');
        ART.push_back(sameArt(cr.finish()));
    }
    ART.push_back(makeJanitor());
}

struct V2 { float x, y; };
struct Sprite {
    string name;
    int art = 0;
    float x = 0, y = 0, h = 1;            // position (cells) and height as a fraction of the wall height
    RGB main{200, 200, 200}, acc{200, 60, 60};
    vector<string> lines;                 // dialogue
    vector<V2> wp;                        // patrol waypoints (cells)
    int wi = 0, n = 0;
    float speed = 0;
    float yaw = 0;                        // heading in radians (the direction the character faces)
};
static vector<Sprite> gSprites;
static const Sprite* gNear = nullptr;

static bool colorFor(char ch, const Sprite& s, RGB& o) {
    switch (ch) {
        case 'C': case 'K': o = s.main; return true;
        case 'D': o = mul(s.main, 0.55f); return true;
        case 'R': o = s.acc; return true;
        case 'H': o = {84, 90, 102}; return true;
        case 'O': o = {16, 14, 20}; return true;
        case 'Q': o = mul(s.main, 0.68f); return true;
        case 'N': o = {196, 120, 124}; return true;
        case 'Z': o = {110, 116, 122}; return true;
        case 'F': o = {226, 230, 96}; return true;
        case 'S': o = {210, 176, 150}; return true;
        case 'E': o = {22, 24, 32}; return true;
        case 'P': o = {66, 78, 100}; return true;
        case 'B': o = {58, 60, 66}; return true;
        case 'W': o = {200, 206, 212}; return true;
        case 'Y': o = {236, 200, 90}; return true;
        case 'G': o = {84, 128, 96}; return true;
        case 'T': o = {130, 96, 70}; return true;
        default: return false;
    }
}

// tile coordinates in, cell coordinates stored (a tile is 2 cells)
static Sprite& addSprite(const char* name, int art, float tx, float ty, float h, RGB main = {200, 200, 200}, RGB acc = {200, 60, 60}) {
    Sprite s; s.name = name; s.art = art; s.x = tx * 2; s.y = ty * 2; s.h = h; s.main = main; s.acc = acc;
    gSprites.push_back(s);
    return gSprites.back();
}
static void patrol(Sprite& s, std::initializer_list<V2> pts, float tilesPerSec) {
    for (V2 p : pts) s.wp.push_back({p.x * 2, p.y * 2});
    s.wi = s.wp.size() > 1 ? 1 : 0;
    s.speed = tilesPerSec * 2;
    if (s.wp.size() > 1) s.yaw = std::atan2(s.wp[1].y - s.wp[0].y, s.wp[1].x - s.wp[0].x);
}
static void spawnSprites() {
    const RGB slate{130, 140, 150}, wood{150, 112, 76}, coat{96, 92, 112}, shawl{150, 80, 70};
    // ground floor
    addSprite("Mrs. Volkova", A_OLD, 5.5f, 1.5f, .8f, coat, shawl).lines = {"Shouting at two a.m. Anton in 106 owed money.", "I told you already. Mind Boris, he bites."};
    Sprite& igor = addSprite("Man in red scarf", A_MAN, 14.5f, 1.5f, .95f, {64, 68, 78}, {196, 52, 40});
    igor.lines = {"Detective? I was home all night."};
    patrol(igor, {{14.5f, 1.5f}, {14.5f, 13.5f}, {1.5f, 13.5f}, {1.5f, 1.5f}}, .8f);
    Sprite& cat = addSprite("Boris the cat", A_CAT, 6.5f, 13.5f, .34f, {220, 140, 60});
    cat.lines = {"Boris hisses. A red wool thread hangs from his collar."};
    patrol(cat, {{4.5f, 13.5f}, {11.5f, 13.5f}}, .5f);
    for (V2 p : {V2{1.5f, 1.5f}, V2{14.5f, 13.5f}, V2{9.5f, 1.5f}}) addSprite("", A_PLANT, p.x, p.y, .4f);
    addSprite("", A_BIN, 3.5f, 13.5f, .3f, slate); addSprite("", A_BIN, 10.5f, 1.5f, .3f, slate);
    addSprite("", A_CRATE, 12.5f, 13.5f, .3f, wood); addSprite("", A_CRATE, 1.5f, 9.5f, .3f, wood);
    // upper floor (tile x + 20)
    Sprite& pavel = addSprite("Pavel (janitor)", A_JANITOR, 34.5f, 9.5f, .9f, {78, 112, 140}, {226, 170, 60});
    pavel.lines = {"Nobody comes up here after ten. Mind the stairs.", "The light by the landing has been dying for weeks."};
    pavel.yaw = 3.14159265f;                                           // facing west, down the corridor
    Sprite& tenant = addSprite("Old tenant", A_OLD, 24.0f, 13.5f, .8f, coat, shawl);
    tenant.lines = {"The elevator hasn't worked since spring."};
    tenant.yaw = -1.5707963f;
    addSprite("", A_PLANT, 21.5f, 1.5f, .4f);
    addSprite("", A_CRATE, 34.5f, 3.5f, .3f, wood); addSprite("", A_BIN, 28.5f, 13.5f, .3f, slate);
    int k = 0;
    for (Sprite& sp : gSprites) {
        if (sp.name == "Mrs. Volkova") sp.yaw = 1.5707963f;            // watching the doors
        if (sp.name.empty()) sp.yaw = 0.35f + 1.37f * (float)(k++);   // props sit at slightly different angles
    }
}

static void spawnLamps() {
    // bulbs hang at irregular distances along the hallways; each floor gets its own set
    const V2 base[] = {{2.6f, 1.5f}, {6.2f, 1.5f}, {8.6f, 1.5f}, {12.8f, 1.5f}, {14.5f, 3.4f}, {14.5f, 8.2f}, {14.5f, 12.3f},
                       {11.6f, 13.5f}, {7.0f, 13.5f}, {3.3f, 13.5f}, {1.5f, 10.0f}, {1.5f, 5.5f}};
    for (int f = 0; f < 2; f++)
        for (V2 p : base) gLamps.push_back({(p.x + 20.0f * f) * 2, p.y * 2, 1.0f});
    gLamps.push_back({17.5f * 2, 6.5f * 2, 1.0f});    // over the stairs
    // cold flash light just inside every window; dark except during lightning (appended last so bulb indices stay put)
    for (int cy = 0; cy < GH; cy++)
        for (int cx = 0; cx < GW; cx++) {
            const Cell& w = gGrid[cy][cx];
            if (w.type != C_WINDOW || w.sx || w.sy) continue;          // one lamp per window tile
            float ox = 0, oy = 0;
            if (cellAt(cx, cy - 1).type == C_OPEN) oy = -1; else if (cellAt(cx, cy + 2).type == C_OPEN) oy = 1;
            else if (cellAt(cx - 1, cy).type == C_OPEN) ox = -1; else if (cellAt(cx + 2, cy).type == C_OPEN) ox = 1;
            Lamp l{cx + 1.0f + ox * 1.6f, cy + 1.0f + oy * 1.6f, 0.0f};
            l.r = 5.0f; l.win = true;
            gLamps.push_back(l);
        }
}

// ---------------------------------------------------------------- camera + column renderer
struct Cam { float px, py, eye, hz, dx, dy, plx, ply; };
static float projY(const Cam& c, float h, float t) { return c.hz - (h - c.eye) * RH / t; }

struct Focus { bool valid = false; CellType type = C_WALL; int door = 0, cx = 0; float t = 99; };
static Focus gFocus;
static float gTime = 0;
static float gFlash = 0;                 // lightning brightness 0..1
static float gBoltT = -1.0f, gBoltAz = 0, gBoltSeed = 0;   // running bolt: time since start (-1 = none), where in the sky, its shape

static void drawCeilRow(const Cam& c, int x, int y, const Cell& cell, float rx, float ry) {
    float dv = c.hz - (y + 0.5f);
    if (dv < 0.01f) return;
    float t = (cell.C - c.eye) * RH / dv, wx = c.px + t * rx, wy = c.py + t * ry;
    gBulb = 0;
    float L = lampL(wx, wy);
    if (gBulb > 0) { putPix(x, y, 70, 62, 52, 0.25f, 1); return; }    // dark ceiling mount the cord hangs from (the bulb and its glow are drawn in drawGlows)
    const uint8_t* p = texAt(TC, frac(wx * 0.5f), frac(wy * 0.5f));
    putPix(x, y, p[0], p[1], p[2], L * 0.8f, std::max(0.04f, std::exp(-t / 16)));
}
static void drawFloorRow(const Cam& c, int x, int y, const Cell& cell, float rx, float ry) {
    float dv = (y + 0.5f) - c.hz;
    if (dv < 0.01f) return;
    float t = (c.eye - cell.F) * RH / dv, wx = c.px + t * rx, wy = c.py + t * ry;
    gBulb = 0;
    float L = lampL(wx, wy);
    const uint8_t* p = cell.stair ? texAt(TS, frac(wx), frac(wy * 0.5f)) : texAt(TF, frac(wx * 0.5f), frac(wy * 0.5f));
    putPix(x, y, p[0], p[1], p[2], L, std::max(0.04f, std::exp(-t / 16)));
}
// What you see through a window: open night air, a long way up. No street (we are high above it), just rain falling
// between the building and a far limit, a dim suburban skyline in three depth layers, a murky orange glow on the
// horizon, and lightning. The skyline depends only on the compass direction of the ray, so every window agrees.
static RGB outsideView(float rx, float ry, float hx, float hy, int nside, float h, float sl, float az) {
    float dr = nside == 0 ? std::fabs(rx) : std::fabs(ry);               // depth gained per unit of ray parameter
    float lr = nside == 0 ? ry : rx;                                     // lateral movement per unit
    float a0 = nside == 0 ? hy : hx;
    if (dr < 1e-4f) return {8, 10, 14};
    float e = sl / dr;                                                   // elevation: 0 = eye level, + up, - down
    float fl = gFlash;

    // ---- sky: murky orange glow low down, near-black overhead; clouds only show when lightning lights them
    float glow = 0.75f + 0.25f * std::sin(az * 2.0f + 1.0f);              // the city is brighter in some directions
    float g = std::sqrt(std::clamp(e * 1.0f, 0.0f, 1.0f));
    RGB col{(84.0f * (1 - g) + 7.0f * g) * glow, (48.0f * (1 - g) + 9.0f * g) * glow, (38.0f * (1 - g) + 17.0f * g) * glow};
    if (fl > 0.02f) {
        float cl = 0.3f + 0.7f * (0.5f + 0.5f * std::sin(az * 5.0f + std::sin(e * 9.0f + az * 2.0f) * 1.7f));
        float k = fl * cl * (0.45f + std::min(1.0f, std::max(e, 0.0f) * 2.0f));
        col.r += 105 * k; col.g += 125 * k; col.b += 190 * k;
    }
    // ---- the bolt itself (first 0.3 s of a strike): a jagged line plus one branch, with a soft glow
    if (gBoltT >= 0.0f && gBoltT < 0.3f && fl > 0.25f && e > -0.14f && e < 0.62f) {
        float dA = az - gBoltAz; dA -= 6.2831853f * std::floor(dA / 6.2831853f + 0.5f);
        float path = 0.035f * std::sin(e * 34.0f + gBoltSeed) + 0.012f * std::sin(e * 91.0f + gBoltSeed * 2.3f);
        float d1 = dA - path;
        float d2 = e > 0.06f && e < 0.30f ? dA - path - (0.30f - e) * 0.45f : 1.0f;     // branch
        float m = std::min(std::fabs(d1), std::fabs(d2) * 1.3f);
        if (m < 0.0075f) col = {245, 248, 255};
        else { float k = std::exp(-m * m / 0.0009f) * 0.6f * fl; col.r += 110 * k; col.g += 130 * k; col.b += 255 * k; }
    }
    // ---- skyline: three layers of blocks, far to near; each block has a height, lit windows and sometimes a beacon
    struct Lay { float bw, base, amp; RGB body; float lit; int cols; };
    static const Lay LAY[3] = {{0.050f, -0.02f, 0.17f, {26, 30, 44}, 0.10f, 4},
                               {0.080f, -0.10f, 0.20f, {16, 19, 30}, 0.13f, 5},
                               {0.135f, -0.22f, 0.26f, {8, 10, 17}, 0.16f, 6}};
    for (int j = 0; j < 3; j++) {
        const Lay& L = LAY[j];
        float u = (az + 3.5f) / L.bw; int idx = (int)std::floor(u); float fu = u - (float)idx;
        uint32_t hv = hash32((uint32_t)idx, (uint32_t)j, 21);
        float tv = (float)(hv & 1023) / 1023.0f;
        float top = ((hv >> 12) & 7) == 0 ? L.base - 0.04f : L.base + L.amp * tv * tv;
        if (e >= top) {
            if (j < 2 && ((hv >> 20) & 31) == 0 && e < top + 0.03f && std::fabs(fu - 0.5f) < 0.16f && std::sin(gTime * 2.2f + (float)(hv & 7)) > 0.3f)
                col = {255, 56, 44};                                            // red beacon on a tall block
            continue;
        }
        float lit = 1.0f + fl * 1.3f;
        col = mul(L.body, lit);
        if (fu > 0.12f && fu < 0.88f) {
            float wx = fu * (float)L.cols, we = e / 0.03f;
            float fx = wx - std::floor(wx), fe = we - std::floor(we);
            if (fx > 0.2f && fx < 0.8f && fe > 0.25f && fe < 0.75f &&
                (hash32((uint32_t)idx * 8u + (uint32_t)wx, (uint32_t)(int)std::floor(we), (uint32_t)j + 40u) & 255) < (uint32_t)(L.lit * 255.0f)) {
                bool warm = (hv & 1) != 0 || ((hv >> 5) & 3) != 0;
                float k = 0.30f + 0.12f * (float)j;
                col = warm ? RGB{255 * k, 190 * k, 110 * k} : RGB{190 * k, 215 * k, 245 * k};
            }
        }
    }
    // ---- murk: far below eye level everything dissolves into dark haze (the street is never visible)
    float mk = std::clamp((-e - 0.30f) / 0.25f, 0.0f, 1.0f);
    RGB mc = mul({15, 12, 15}, 1.0f + fl * 2.0f);
    col = {col.r * (1 - mk) + mc.r * mk, col.g * (1 - mk) + mc.g * mk, col.b * (1 - mk) + mc.b * mk};
    // ---- rain: six depth layers out to a fixed distance, nothing beyond it (so the skyline stays clear)
    static const float RD[6] = {0.8f, 1.8f, 3.2f, 5.2f, 8.0f, 12.0f};
    for (int i = 0; i < 6; i++) {
        float ti = RD[i] / dr;
        float ai = a0 + lr * ti, hi = h + sl * ti;
        float sc = std::clamp(60.0f / RD[i], 5.0f, 60.0f), fa = frac(ai * sc);   // streak columns per cell: ~2 px wide at any depth
        if (fa < 0.2f || fa > 0.8f) continue;
        uint32_t hv = hash32((uint32_t)(int)std::floor(ai * sc), (uint32_t)i, 9);
        static const uint32_t DENS[6] = {10, 14, 22, 34, 50, 64};            // of 256: near drops are sparse, far rain is a haze
        if (((hv >> 16) & 255) >= DENS[i]) continue;
        float len = 1.0f / (1.0f + 0.45f * (float)i);                         // far streaks are long in the world, short on screen
        float q = hi * 1.5f * len + gTime * (3.2f + 0.8f * (float)i + (float)((hv >> 4) & 7) * 0.2f) * len + (float)((hv >> 8) & 255) / 255.0f * 9.0f;
        float fq = frac(q);
        if (fq > 0.3f) continue;
        float k = (1.0f - fq / 0.3f) * (0.95f - 0.10f * (float)i) * (0.5f + 1.1f * fl);
        col.r += 135 * k; col.g += 160 * k; col.b += 195 * k;
    }
    return col;
}

// a wall face (plain wall, door, window, handrail) between rows y0..y1
static void drawWallRows(const Cam& c, int x, int y0, int y1, float tFar, float hx, float hy, float u, int nside,
                         bool flip, const Cell& cur, const Cell& wall, int wcx) {
    float uu = ((nside == 0 ? wall.sy : wall.sx) * 0.5f) + u * 0.5f;    // position along the whole tile
    if (flip) uu = 1.0f - uu;
    gBulb = 0;
    float L = lampL(hx, hy), f = std::max(0.04f, std::exp(-tFar / 16)) * (nside ? 0.88f : 1.0f);
    float cs = 2.0f * x / RW - 1.0f, rx = c.dx + c.plx * cs, ry = c.dy + c.ply * cs;
    // Beside the stairs the floor height jumps with every step, so a pattern measured from it would zig-zag.
    // Measure from the smooth slope through the step edges instead: trim line and handrail become true diagonals.
    float refF = cur.stair ? STEP_H * (hx - 29.0f) : cur.F;
    float az = wall.type == C_WINDOW ? std::atan2(ry, rx) : 0.0f;      // compass direction of this column (windows only)
    for (int y = std::max(0, y0); y < std::min(RH, y1); y++) {
        float h = c.eye - ((y + 0.5f) - c.hz) * tFar / RH;
        float v = 1.0f - (h - refF) / WALL_H;                              // texture row follows the floor (or the stair slope)
        float r, g, b;
        if (wall.type == C_WINDOW) {
            bool inPane = uu > 0.14f && uu < 0.86f && v > 0.16f && v < 0.84f;
            if (inPane && !(std::fabs(uu - 0.5f) < 0.022f || std::fabs(v - 0.46f) < 0.022f)) {
                float sl = -((y + 0.5f) - c.hz) / RH;                        // height gained per unit of ray parameter
                RGB o = outsideView(rx, ry, hx, hy, nside, h, sl, az);
                if (hs((int)(uu * 100), ((int)(v * 150) + (int)(gTime * 34)) >> 1) > 0.985f) o = {o.r * 0.6f + 80, o.g * 0.6f + 88, o.b * 0.6f + 100};   // drops on the glass
                putFlat(x, y, o.r, o.g, o.b);
                continue;
            }
            if (inPane) { float k = 0.85f + 0.3f * hs(x, y); r = 56 * k; g = 47 * k; b = 40 * k; }                          // cross bars
            else if (uu > 0.10f && uu < 0.90f && v > 0.12f && v < 0.84f) { float k = 0.85f + 0.3f * hs(x, y); r = 62 * k; g = 52 * k; b = 44 * k; }   // frame
            else if (uu > 0.07f && uu < 0.93f && v >= 0.84f && v < 0.90f) { float k = 0.9f + 0.2f * hs(x, y); r = 128 * k; g = 130 * k; b = 128 * k; }  // sill
            else { const uint8_t* p = texAt(TW, uu, v); r = p[0]; g = p[1]; b = p[2]; }
        } else {
            const Tex& t = wall.type == C_DOOR ? DT[wcx >= 40 ? 1 : 0][std::clamp((int)wall.door, 1, 8)]
                         : (wall.type == C_RAIL && cur.stair) ? TR : TW;
            const uint8_t* p = texAt(t, uu, v);
            r = p[0]; g = p[1]; b = p[2];
        }
        putPix(x, y, r, g, b, L * (1.1f - 0.3f * std::clamp(v, 0.0f, 1.0f)), f);
    }
}
static void drawRiserRows(const Cam& c, int x, int y0, int y1, int topRow, int botRow, float tFar, float hx, float hy, float u) {
    (void)c;
    gBulb = 0;
    float L = lampL(hx, hy), f = std::max(0.04f, std::exp(-tFar / 16));
    float span = (float)std::max(1, botRow - topRow);
    for (int y = std::max(0, y0); y < std::min(RH, y1); y++) {
        const uint8_t* p = texAt(TRISER, u, ((float)y - topRow) / span);
        putPix(x, y, p[0], p[1], p[2], L, f);
    }
}

static void renderColumn(int x, const Cam& c) {
    float cs = 2.0f * x / RW - 1.0f, rx = c.dx + c.plx * cs, ry = c.dy + c.ply * cs;
    int mx = (int)std::floor(c.px), my = (int)std::floor(c.py);
    const float BIG = 1e30f;
    float ddx = std::fabs(rx) < 1e-9f ? BIG : std::fabs(1.0f / rx), ddy = std::fabs(ry) < 1e-9f ? BIG : std::fabs(1.0f / ry);
    int stx, sty; float sdx, sdy;
    if (rx < 0) { stx = -1; sdx = (c.px - mx) * ddx; } else { stx = 1; sdx = (mx + 1 - c.px) * ddx; }
    if (ry < 0) { sty = -1; sdy = (c.py - my) * ddy; } else { sty = 1; sdy = (my + 1 - c.py) * ddy; }

    int yT = 0, yB = RH;                 // rows still undrawn: [yT, yB)
    float tNear = 0.02f;
    gZ[x] = 1e9f;
    for (int it = 0; it < 200 && yT < yB; it++) {
        const Cell& cur = cellAt(mx, my);
        if (cur.type != C_OPEN) break;   // started inside a wall (cannot happen with collision, but never crash)
        int nmx = mx, nmy = my, nside;
        float tFar;
        if (sdx < sdy) { tFar = sdx; sdx += ddx; nmx += stx; nside = 0; }
        else { tFar = sdy; sdy += ddy; nmy += sty; nside = 1; }
        if (tFar < tNear) tFar = tNear;

        // ceiling of this cell between the near and far edge
        {
            int a = toRow(projY(c, cur.C, tNear)), b = toRow(projY(c, cur.C, tFar));
            if (cur.C > c.eye + 0.01f)
                for (int y = std::max(yT, a); y < std::min(yB, b); y++) drawCeilRow(c, x, y, cur, rx, ry);
            yT = std::max(yT, b);
        }
        // floor of this cell
        {
            int a = toRow(projY(c, cur.F, tFar)), b = toRow(projY(c, cur.F, tNear));
            if (cur.F < c.eye - 0.01f)
                for (int y = std::max(yT, a); y < std::min(yB, b); y++) drawFloorRow(c, x, y, cur, rx, ry);
            yB = std::min(yB, a);
        }
        if (yT >= yB) break;

        const Cell& nx = cellAt(nmx, nmy);
        float hx = c.px + tFar * rx, hy = c.py + tFar * ry;
        float u = frac(nside == 0 ? hy : hx);
        if (nx.type != C_OPEN) {         // solid neighbour: its face fills the rest of the column
            bool flip = (nside == 0 && rx > 0) || (nside == 1 && ry < 0);
            drawWallRows(c, x, yT, yB, tFar, hx, hy, u, nside, flip, cur, nx, nmx);
            gZ[x] = tFar;
            if (x == RW / 2) { gFocus.valid = true; gFocus.type = nx.type; gFocus.door = nx.door; gFocus.cx = nmx; gFocus.t = tFar; }
            break;
        }
        // open neighbour: draw the riser (floor goes up) and the ceiling step (ceiling comes down)
        if (nx.F > cur.F + 1e-4f) {
            int top = toRow(projY(c, nx.F, tFar)), bot = toRow(projY(c, cur.F, tFar));
            drawRiserRows(c, x, std::max(yT, top), yB, top, bot, tFar, hx, hy, u);
            yB = std::min(yB, top);
        }
        if (nx.C < cur.C - 1e-4f) {
            int bot = toRow(projY(c, nx.C, tFar));
            for (int y = std::max(0, yT); y < std::min(yB, bot); y++) {
                float h = c.eye - ((y + 0.5f) - c.hz) * tFar / RH;
                const uint8_t* p = texAt(TW, u, 0.05f + 0.4f * frac(h));         // plaster, varied by height (no streaks)
                putPix(x, y, p[0], p[1], p[2], lampL(hx, hy) * 0.8f, std::max(0.04f, std::exp(-tFar / 16)));
            }
            yT = std::max(yT, bot);
        }
        mx = nmx; my = nmy; tNear = tFar;
    }
}

// ---------------------------------------------------------------- player + game state
static float gPx = 5.0f, gPy = 3.0f, gAng = 0, gFeet = 0, gPhase = 0, gBob = 0;
static float gPitch = 0;                       // look up/down, in screen rows (positive = looking up)
constexpr float PITCH_MAX = 0.30f * RH;        // about 54 rows; more than this and the shear starts to look stretched
static bool gMoving = false;
struct LogLine { string text; Color col; };
static vector<LogLine> gLog;
static const Color COL_ME{232, 176, 74, 255}, COL_SYS{110, 125, 135, 255};
static void say(const string& t, Color c) { gLog.push_back({t, c}); }

static Cam makeCam() {
    Cam c;
    c.px = gPx; c.py = gPy; c.eye = gFeet + EYE_H; c.hz = RH / 2.0f + gBob + gPitch;
    c.dx = std::cos(gAng); c.dy = std::sin(gAng);
    c.plx = -c.dy * PLANE; c.ply = c.dx * PLANE;
    return c;
}

// Hanging bulbs: a cord, a hot core and a soft halo drawn on top of the picture (additive). The halo is the bulb's
// own glow - it adds no light to the scene. Columns where a wall is nearer than the bulb are skipped.
static void drawGlows(const Cam& c) {
    static float lut[1025];                                                  // halo profile by squared distance (0..1), built once
    static bool lutOk = false;
    if (!lutOk) {
        for (int i = 0; i <= 1024; i++) {
            float d2 = (float)i / 1024.0f, d = std::sqrt(d2);
            lut[i] = std::min(1.5f, 1.0f * std::exp(-d2 * 220.0f) + 0.62f * std::exp(-d2 * 22.0f) + 0.36f * (1 - d) * (1 - d) * (1 - d));
        }
        lutOk = true;
    }
    for (const Lamp& l : gLamps) {
        if (l.win || l.k <= 0.002f) continue;
        float rx = l.x - c.px, ry = l.y - c.py, ty = rx * c.dx + ry * c.dy;
        if (ty < 0.35f) continue;
        float tx = (c.dx * ry - c.dy * rx) / PLANE;
        float bulbH = cellAt((int)std::floor(l.x), (int)std::floor(l.y)).C - 0.35f;
        float ceilH = bulbH + 0.35f;
        float sx = RW / 2.0f * (1 + tx / ty), sy = c.hz - (bulbH - c.eye) * RH / ty, sc = c.hz - (ceilH - c.eye) * RH / ty;
        float R = std::min(100.0f, 1.5f * RH / ty);                         // halo profile radius in pixels (1.5 cells)
        float Rb = R * 0.72f;                                               // beyond this the halo is too faint to see: skip it
        if (sx < -Rb || sx > RW + Rb) continue;                             // far off to the side: nothing to draw
        int xa = clampi(std::floor(sx - Rb), 0, RW), xb = clampi(std::ceil(sx + Rb), 0, RW);
        int ya = clampi(std::floor(sy - Rb), 0, RH), yb = clampi(std::ceil(sy + Rb), 0, RH);
        int cx = clampi(sx, 0, RW - 1);
        if (gZ[cx] >= ty - 0.1f)                                            // cord
            for (int y = clampi(std::floor(sc), 0, RH); y < clampi(std::floor(sy), 0, RH); y++) putFlat(cx, y, 34, 30, 26);
        for (int x = xa; x < xb; x++) {
            if (gZ[x] < ty - 0.1f) continue;
            for (int y = ya; y < yb; y++) {
                float dx = x + 0.5f - sx, dy = y + 0.5f - sy, d2 = dx * dx + dy * dy;
                if (d2 > Rb * Rb) continue;
                float I = l.k * lut[std::min(1024, (int)(d2 / (R * R) * 1024.0f))];
                if (I < 0.012f) continue;
                addLight(x, y, 255 * I, 214 * I * (0.92f + 0.1f * I), 150 * I + 70 * std::max(0.0f, I - 0.8f));    // warm; hottest part tends to white
            }
        }
    }
}

static void renderScene() {
    Cam c = makeCam();
    for (int i = 0; i < RW * RH; i++) gPix[i] = 0xff0b0907u;
    gFocus = Focus();
    for (int x = 0; x < RW; x++) renderColumn(x, c);
    drawGlows(c);

    // sprites, far to near, depth-tested against the wall distance of each column
    struct Item { const Sprite* s; float ty, tx; };
    vector<Item> list;
    for (const Sprite& s : gSprites) {
        float rx = s.x - c.px, ry = s.y - c.py, ty = rx * c.dx + ry * c.dy;
        if (ty > 0.4f) list.push_back({&s, ty, (c.dx * ry - c.dy * rx) / PLANE});
    }
    std::sort(list.begin(), list.end(), [](const Item& a, const Item& b) { return a.ty > b.ty; });
    gNear = nullptr;
    for (const Item& it : list) {
        const Sprite& s = *it.s;
        if (s.art < 0 || s.art >= (int)ART.size()) continue;
        const SpriteSet& set = ART[(size_t)s.art];
        float H = s.h * WALL_H;                                              // world height
        float floorH = cellAt((int)std::floor(s.x), (int)std::floor(s.y)).F;
        float fx = std::cos(s.yaw), fy = std::sin(s.yaw);                    // heading
        float cxv = c.px - s.x, cyv = c.py - s.y;                            // sprite -> camera
        bool viewFront = cxv * fx + cyv * fy >= 0;
        const SpriteArt& A = viewFront ? set.front : set.back;               // plane A: across the heading, front or back
        const SpriteArt& B = set.side;                                       // plane B: along the heading, the side
        float hw[2] = {H * A.w / A.h * 0.5f, H * B.w / B.h * 0.5f};
        float Dx[2] = {fy, fx}, Dy[2] = {-fx, fy};                           // plane directions: A = (fy,-fx), B = (fx,fy)
        const SpriteArt* art[2] = {&A, &B};
        float rad = std::max(hw[0], hw[1]);
        gBulb = 0;
        float L = lampL(s.x, s.y) + 0.12f;
        float sxc = RW / 2.0f * (1 + it.tx / it.ty), halfPx = rad * RH / std::max(0.25f, it.ty - rad) + 1.0f;
        int xa = clampi(std::floor(sxc - halfPx), 0, RW), xb = clampi(std::ceil(sxc + halfPx), 0, RW);
        float ppx = s.x - c.px, ppy = s.y - c.py;
        for (int x = xa; x < xb; x++) {
            float cs = 2.0f * (x + 0.5f) / RW - 1.0f, rx = c.dx + c.plx * cs, ry = c.dy + c.ply * cs;
            struct Hit { int pl; float t, u; } hits[2]; int nh = 0;
            for (int pl = 0; pl < 2; pl++) {
                float den = rx * Dy[pl] - ry * Dx[pl];
                if (std::fabs(den) < 1e-5f) continue;
                float t = (ppx * Dy[pl] - ppy * Dx[pl]) / den, sp = (ppx * ry - ppy * rx) / den;
                if (t < 0.15f || std::fabs(sp) > hw[pl] || gZ[x] < t) continue;
                float u = pl == 0 ? 0.5f + (viewFront ? sp : -sp) / (2 * hw[0]) : 0.5f - sp / (2 * hw[1]);
                hits[nh++] = {pl, t, u};
            }
            if (nh == 2 && hits[0].t < hits[1].t) std::swap(hits[0], hits[1]);   // far plane first
            for (int k = 0; k < nh; k++) {
                const Hit& ht = hits[k];
                const SpriteArt& a = *art[ht.pl];
                float top = c.hz - (floorH + H - c.eye) * RH / ht.t, bot = c.hz - (floorH - c.eye) * RH / ht.t;
                if (bot - top < 0.5f) continue;
                int u = clampi(ht.u * a.w, 0, a.w - 1);
                int ya = clampi(std::floor(top), 0, RH), yb = clampi(std::ceil(bot), 0, RH);
                float ft = std::max(0.05f, std::exp(-ht.t / 16));
                for (int y = ya; y < yb; y++) {
                    int v = clampi((y + 0.5f - top) / (bot - top) * a.h, 0, a.h - 1);
                    size_t ai = (size_t)v * a.w + u;
                    char ch = a.c[ai];
                    RGB col;
                    if (ch == ' ' || !colorFor(ch, s, col)) continue;
                    int8_t sd = a.s[ai];
                    col = mul(col, sd < 0 ? 0.62f : sd > 0 ? 1.28f : 1.0f);
                    if (ch == 'F') putPix(x, y, col.r, col.g, col.b, 1.4f, std::max(ft, 0.6f));   // eyes glow
                    else putPix(x, y, col.r, col.g, col.b, L, ft);
                }
            }
        }
        if (!s.lines.empty() && it.ty < 4.8f && std::fabs(it.tx / it.ty) < 0.3f && it.ty < gZ[RW / 2] + 0.1f) gNear = &s;
    }
    gFrame++;
}

static bool passable(float x, float y, float curF) {
    const float r = 0.4f;
    for (int j = -1; j <= 1; j += 2)
        for (int i = -1; i <= 1; i += 2) {
            const Cell& c = cellAt((int)std::floor(x + i * r), (int)std::floor(y + j * r));
            if (c.type != C_OPEN || c.F > curF + 0.25f || c.C - c.F < 1.6f) return false;   // walls, tall steps, low ceilings
        }
    return true;
}

struct Input { float turn = 0, fw = 0, st = 0, mouseX = 0, mouseY = 0, pitchKey = 0; bool interact = false, recenter = false; };
static const char* KN[8] = {"A chain rattles. No one answers.", "A TV drones behind the door.", "A tired voice: \"Night shift. Not home.\"",
                            "A boy through the gap: \"Big guy came by. Red scarf.\"", "Mail piles up on the mat.",
                            "Crime scene tape across the frame. A muddy boot print by the hinge.", "Boiled cabbage smell. The door stays shut.",
                            "A radio plays softly. No answer."};

static float gNextBolt = 4.0f;
static uint32_t gBolts = 0;
static void updateThunder(float dt) {          // a bolt every 6-20 seconds: flash, flicker, flash, fade
    if (gBoltT < 0 && gTime >= gNextBolt) {
        gBoltT = 0;
        gBoltAz = gAng + ((float)(hash32(gBolts, 5, 9) & 1023) / 1023.0f - 0.5f) * 1.3f;   // strike somewhere near where the player is looking
        gBoltSeed = (float)(hash32(gBolts, 6, 2) & 1023) / 100.0f;
    }
    if (gBoltT < 0) { gFlash = 0; return; }
    gBoltT += dt;
    float t = gBoltT;
    gFlash = t < 0.07f ? 1.0f : t < 0.16f ? 0.2f : t < 0.24f ? 0.9f : 0.9f * std::exp(-(t - 0.24f) * 7.0f);
    if (t > 1.4f) {
        gBoltT = -1; gFlash = 0; gBolts++;
        gNextBolt = gTime + 6.0f + 14.0f * (float)(hash32(gBolts, 17, 3) & 1023) / 1023.0f;
    }
}

static void tick(float dt, const Input& in) {
    gTime += dt;
    updateThunder(dt);
    gAng += in.turn * 2.2f * dt + in.mouseX * 0.0025f;
    gPitch -= in.mouseY * 0.35f;                      // mouse up (negative delta) = look up
    gPitch += in.pitchKey * 70.0f * dt;
    if (in.recenter) gPitch = 0;
    gPitch = std::clamp(gPitch, -PITCH_MAX, PITCH_MAX);
    float dx = std::cos(gAng), dy = std::sin(gAng);
    float mvx = dx * in.fw - dy * in.st, mvy = dy * in.fw + dx * in.st, sp = 4.8f * dt;
    gMoving = (in.fw != 0 || in.st != 0);
    if (gMoving) {
        gPhase += dt * 9;
        float curF = cellAt((int)std::floor(gPx), (int)std::floor(gPy)).F;
        float nx = gPx + mvx * sp, ny = gPy + mvy * sp;
        if (passable(nx, gPy, curF)) gPx = nx;     // slide along walls: test each axis separately
        if (passable(gPx, ny, curF)) gPy = ny;
    }
    const Cell& here = cellAt((int)std::floor(gPx), (int)std::floor(gPy));
    gFeet += (here.F - gFeet) * (1.0f - std::exp(-dt * 16.0f));          // smooth camera over the steps
    gBob = gMoving ? std::sin(gPhase) * (here.stair ? 2.2f : 1.1f) : 0.0f;

    // lamps: one buzzes, one is dying (per floor)
    for (Lamp& l : gLamps) l.k = l.win ? gFlash * 1.1f : 1.0f;
    gLamps[3].k = hs((int)(gTime * 12), 3) > 0.8f ? 0.15f : 0.95f; gLamps[6].k = 0.25f;
    gLamps[15].k = hs((int)(gTime * 9), 7) > 0.85f ? 0.2f : 0.95f; gLamps[18].k = 0.3f;

    for (Sprite& s : gSprites) {                                         // patrols
        if (s.wp.empty()) continue;
        V2 w = s.wp[(size_t)s.wi];
        float ex = w.x - s.x, ey = w.y - s.y, d = std::hypot(ex, ey);
        if (d < 0.2f) s.wi = (s.wi + 1) % (int)s.wp.size();
        else { s.x += ex / d * s.speed * dt; s.y += ey / d * s.speed * dt; s.yaw = std::atan2(ey, ex); }
    }
    renderScene();

    if (in.interact) {
        if (gNear) {
            Sprite& s = const_cast<Sprite&>(*gNear);
            s.yaw = std::atan2(gPy - s.y, gPx - s.x);                    // turn to face whoever is talking
            say(s.name + ": \"" + s.lines[(size_t)s.n++ % s.lines.size()] + "\"", COL_ME);
        } else if (gFocus.valid && gFocus.type == C_DOOR && gFocus.t < 3.6f) {
            say("Knock knock, Apt " + std::to_string(gFocus.cx >= 40 ? 2 : 1) + "0" + std::to_string(gFocus.door) + ". " + KN[gFocus.door - 1], COL_ME);
        }
    }
}

static void initGame() {
    buildWorld(); initTextures(); buildArt(); spawnLamps(); spawnSprites();
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++) {
            float nx = ((float)x / RW - 0.5f) * 2, ny = ((float)y / RH - 0.5f) * 2;
            gVig[y * RW + x] = std::clamp(1.0f - 0.55f * std::max(0.0f, nx * nx * 0.6f + ny * ny * 0.9f - 0.3f), 0.35f, 1.0f);
        }
    say("Block 7, ground floor. The stairs are at the east end of the hallway, middle.", COL_SYS);
}

#ifndef BLOCK7_NO_MAIN
int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1100, 740, "Block 7 - pixel raycaster (raylib)");
    SetWindowMinSize(640, 480);
    SetTargetFPS(60);
    initGame();
    Image img{gPix, RW, RH, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);

    while (!WindowShouldClose()) {
        Input in;
        in.turn = (float)(IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT));
        in.fw = (float)((IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) - (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)));
        in.st = (float)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
        in.interact = IsKeyPressed(KEY_E);
        if (IsKeyPressed(KEY_TAB)) { if (IsCursorHidden()) EnableCursor(); else DisableCursor(); }
        if (IsCursorHidden()) { in.mouseX = GetMouseDelta().x; in.mouseY = GetMouseDelta().y; }
        in.pitchKey = (float)(IsKeyDown(KEY_PAGE_UP) - IsKeyDown(KEY_PAGE_DOWN));
        in.recenter = IsKeyPressed(KEY_HOME);
        tick(std::min(0.05f, GetFrameTime()), in);
        UpdateTexture(tex, gPix);

        BeginDrawing();
        ClearBackground(Color{7, 9, 12, 255});
        int ww = GetScreenWidth(), wh = GetScreenHeight(), logH = 118;
        float sc = std::min((float)ww / RW, (float)(wh - logH) / RH);
        Rectangle dst{(ww - RW * sc) / 2, 0, RW * sc, RH * sc};
        DrawTexturePro(tex, Rectangle{0, 0, (float)RW, (float)RH}, dst, Vector2{0, 0}, 0, WHITE);

        string prompt;
        if (gNear) prompt = "[E] Talk to " + gNear->name;
        else if (gFocus.valid && gFocus.type == C_DOOR && gFocus.t < 3.6f)
            prompt = "[E] Knock on Apt " + std::to_string(gFocus.cx >= 40 ? 2 : 1) + "0" + std::to_string(gFocus.door);
        if (!prompt.empty()) {
            int tw = MeasureText(prompt.c_str(), 20);
            DrawRectangle((int)(ww / 2 - tw / 2 - 10), (int)(dst.height - 58), tw + 20, 30, Color{0, 0, 0, 190});
            DrawText(prompt.c_str(), ww / 2 - tw / 2, (int)(dst.height - 52), 20, COL_ME);
        }
        const Cell& here = cellAt((int)std::floor(gPx), (int)std::floor(gPy));
        string where = here.stair ? "Stairs" : (gFeet > 1.0f ? "Upper floor" : "Ground floor");
        DrawText(where.c_str(), (int)dst.x + 12, 10, 18, Color{200, 210, 220, 200});

        int top = (int)dst.height;
        DrawRectangle(0, top, ww, wh - top, Color{10, 13, 16, 255});
        DrawText("WASD move   Left/Right turn   TAB mouse look   E interact", 10, top + 6, 14, COL_SYS);
        int first = std::max(0, (int)gLog.size() - 5);
        for (int i = first; i < (int)gLog.size(); i++)
            DrawText(gLog[(size_t)i].text.c_str(), 10, top + 28 + (i - first) * 17, 15, gLog[(size_t)i].col);
        EndDrawing();
    }
    UnloadTexture(tex);
    CloseWindow();
    return 0;
}
#endif
