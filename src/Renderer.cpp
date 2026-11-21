// Renderer.cpp - the software renderer: procedural textures, lighting, the column renderer and billboards.
// (See Renderer.h for the technique.)
#include "Renderer.h"
#include "Entities.h"

namespace {
template <class F> Tex makeTex(int w, int h, F f) {
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
const uint8_t* texAt(const Tex& t, float u, float v) {
    int x = clampi(u * (float)t.w, 0, t.w - 1), y = clampi(v * (float)t.h, 0, t.h - 1);
    return &t.d[((size_t)y * t.w + x) * 3];
}

const int DG[10][5] = {{7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
                              {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7}};

float projY(float hz, float eye, float h, float t) { return hz - (h - eye) * RH / t; }
int toRow(float y) {                            // screen y -> row index, safe for huge/NaN values
    if (!(y > -1e6f)) return -1000000;
    if (y > 1e6f) return 1000000;
    return (int)std::ceil(y);
}
const int BAY[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
}  // namespace

Renderer::Renderer() {
    for (int i = 0; i < RW * RH; i++) pix[i] = 0xff0b0907u;
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++) {
            float nx = ((float)x / RW - 0.5f) * 2, ny = ((float)y / RH - 0.5f) * 2;
            vig[y * RW + x] = std::clamp(1.0f - 0.55f * std::max(0.0f, nx * nx * 0.6f + ny * ny * 0.9f - 0.3f), 0.35f, 1.0f);   // vignette
        }
    for (float& z : zbuf) z = 1e9f;
}

// ---------------------------------------------------------------- procedural textures
void Renderer::buildTextures(int floorCount) {
    auto wallFn = [](int x, int y, int, int h) -> RGB {
        float v = (float)y / h, k = 0.88f + 0.24f * hs(x, y);
        RGB c;
        if (v > 0.52f && v < 0.57f) c = {112, 124, 130};                       // trim line
        else if (v >= 0.57f) { c = {62, 84, 92}; if (hs(x >> 2, 9) > 0.9f) k *= 0.8f; }   // painted lower wall
        else { c = {150, 158, 160}; if (hs(x >> 3, 3) > 0.86f) k *= 0.55f + 0.45f * v / 0.52f; }  // plaster + stains
        return mul(c, k);
    };
    tWall = makeTex(64, 64, wallFn);
    tRail = makeTex(64, 64, [&](int x, int y, int w, int h) -> RGB {              // wall with a brass handrail band
        float v = (float)y / h;
        if (v > 0.40f && v < 0.45f) return mul({176, 132, 62}, 0.9f + 0.2f * hs(x, y));
        return wallFn(x, y, w, h);
    });
    tFloor = makeTex(64, 64, [](int x, int y, int, int) -> RGB {                   // grimy checkered lino
        float k = 0.9f + 0.2f * hs(x, y);
        if (x % 32 < 1 || y % 32 < 1) k *= 0.55f;
        if (hs(x >> 3, y >> 3) > 0.9f) k *= 0.7f;
        return mul(((x >> 5) + (y >> 5)) & 1 ? RGB{62, 74, 76} : RGB{50, 62, 64}, k);
    });
    tCeil = makeTex(64, 64, [](int x, int y, int, int) -> RGB {
        float k = 0.9f + 0.2f * hs(x, y);
        if (x % 32 < 1 || y % 32 < 1) k *= 0.6f;
        return mul({68, 70, 76}, k);
    });
    tStair = makeTex(64, 64, [](int x, int y, int, int) -> RGB {                   // stair tread with a worn nosing
        bool nose = x < 7;
        return mul(nose ? RGB{176, 156, 112} : RGB{88, 82, 74}, (0.9f + 0.2f * hs(x, y)) * (nose ? 1.0f : 0.85f + 0.15f * x / 64.0f));
    });
    tRiser = makeTex(64, 16, [](int x, int y, int, int h) -> RGB {             // stair riser
        float v = (float)y / h;
        return mul({104, 98, 86}, (0.85f + 0.3f * hs(x, y)) * (1.0f - 0.4f * v));
    });
    tMetal = makeTex(64, 64, [](int x, int y, int, int) -> RGB {                // brushed steel panels (lift cabin)
        float k = 0.9f + 0.2f * hs(x >> 3, y) ;
        if (x % 32 < 1) k *= 0.55f;
        return mul({150, 158, 166}, k * (0.9f + 0.1f * hs(x, y)));
    });
    tLift = makeTex(64, 64, [](int x, int y, int w, int h) -> RGB {              // lift doors: two steel panels, seam, call plate
        float u = (float)x / w, v = (float)y / h;
        RGB c = {92, 100, 108};
        if (u < 0.08f || u > 0.92f || v < 0.04f) c = {52, 56, 60};
        if (std::fabs(u - 0.5f) < 0.012f) c = {38, 40, 42};
        if (u > 0.55f && u < 0.66f && v > 0.40f && v < 0.52f) c = {214, 170, 70};  // call button
        return mul(c, (0.9f + 0.2f * hs(x >> 2, y)) * (0.92f + 0.12f * hs(x, y)));
    });
    tMetalDoor = makeTex(64, 64, [](int x, int y, int w, int h) -> RGB {         // painted steel door: boiler room / roof
        float u = (float)x / w, v = (float)y / h;
        RGB c = {70, 86, 80};
        if (u < 0.08f || u > 0.92f || v < 0.04f) c = {44, 52, 50};
        if (v > 0.45f && v < 0.52f) c = {176, 52, 40};                                 // red warning stripe
        if (u > 0.74f && u < 0.82f && v > 0.52f && v < 0.60f) c = {200, 190, 160};    // padlock
        return mul(c, 0.9f + 0.2f * hs(x, y));
    });
    doorTex.assign((size_t)floorCount, std::array<Tex, DOOR_KINDS>());
    for (int f = 0; f < floorCount; f++) {
        for (int n = 1; n <= DOOR_APT_MAX; n++) {
            string lab = std::to_string(f + 1) + "0" + std::to_string(n);
            bool tape = (f == 0 && n == 6);                                     // crime scene
            doorTex[(size_t)f][(size_t)n] = makeTex(64, 64, [lab, tape](int x, int y, int w, int h) -> RGB {
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
        doorTex[(size_t)f][DOOR_ELEVATOR] = tLift;
        doorTex[(size_t)f][DOOR_BOILER] = tMetalDoor;
        doorTex[(size_t)f][DOOR_ROOF] = tMetalDoor;
    }
}

// ---------------------------------------------------------------- lighting + pixel output
void Renderer::putPix(int x, int y, float r, float g, float b, float L, float f) {
    if ((unsigned)x >= (unsigned)RW || (unsigned)y >= (unsigned)RH) return;
    L = std::min(L, 1.5f);
    f *= vig[y * RW + x];
    // ordered dither + a little film grain, then 5-bit colour for the retro look
    float n = (BAY[((y & 3) << 2) | (x & 3)] - 7.5f) * 1.5f + ((int)(hash32(x, y, frame) & 15) - 7.5f) * 0.35f;
    float cl = cold;                     // cold light from a lightning flash coming through a nearby window
    r = (r * (0.12f + L) + r * cl * 0.22f) * f + n;          // cool ambient, warm lamp light
    g = (g * (0.15f + L * 0.78f) + g * cl * 0.40f) * f + n;
    b = (b * (0.21f + L * 0.50f) + b * cl * 0.80f) * f + n;
    auto q = [](float v) { return (uint32_t)((int)std::clamp(v, 0.0f, 255.0f) & 248); };
    pix[y * RW + x] = 0xff000000u | (q(b) << 16) | (q(g) << 8) | q(r);
}
// a pixel that is not lit by the hallway lamps (the view outside the window); still gets vignette, dither and grain
void Renderer::putFlat(int x, int y, float r, float g, float b) {
    if ((unsigned)x >= (unsigned)RW || (unsigned)y >= (unsigned)RH) return;
    float f = vig[y * RW + x];
    float n = (BAY[((y & 3) << 2) | (x & 3)] - 7.5f) * 1.5f + ((int)(hash32(x, y, frame) & 15) - 7.5f) * 0.35f;
    auto q = [](float v) { return (uint32_t)((int)std::clamp(v, 0.0f, 255.0f) & 248); };
    pix[y * RW + x] = 0xff000000u | (q(b * f + n) << 16) | (q(g * f + n) << 8) | q(r * f + n);
}
// additive light (bulb glow); the vignette is applied too so glows at the screen edge do not pop
void Renderer::addLight(int x, int y, float r, float g, float b) {
    if ((unsigned)x >= (unsigned)RW || (unsigned)y >= (unsigned)RH) return;
    uint32_t p = pix[y * RW + x];
    float f = 0.55f + 0.45f * vig[y * RW + x];
    float n = (BAY[((y & 3) << 2) | (x & 3)] - 7.5f) * 1.2f;
    auto q = [](float v) { return (uint32_t)((int)std::clamp(v, 0.0f, 255.0f) & 248); };
    float pr = (float)(p & 255) + r * f + n, pg = (float)((p >> 8) & 255) + g * f + n, pb = (float)((p >> 16) & 255) + b * f + n;
    pix[y * RW + x] = 0xff000000u | (q(pb) << 16) | (q(pg) << 8) | q(pr);
}

float Renderer::lampL(float wx, float wy) {   // sum of light pools around the bulbs (also sets cold)
    float s = 0;
    cold = 0;
    for (const Lamp& l : lvl->lamps) {
        if (l.k <= 0.002f) continue;
        float ax = wx - l.x, ay = wy - l.y;
        if (ax > l.r || ax < -l.r) continue;
        float d = ax * ax + ay * ay;
        if (d < l.r * l.r) {
            float k = 1.0f - d / (l.r * l.r);
            if (l.win) { cold += k * k * l.k; continue; }
            if (d < 0.03f && l.k > bulb) bulb = l.k;
            s += k * k * l.k;
        }
    }
    return s;
}


// ---------------------------------------------------------------- floor, ceiling, windows, rooms
void Renderer::drawCeilRow(const Cam& c, int x, int y, const Cell& cell, float rx, float ry) {
    float dv = c.hz - (y + 0.5f);
    if (dv < 0.01f) return;
    float t = (cell.C - c.eye) * RH / dv, wx = c.px + t * rx, wy = c.py + t * ry;
    bulb = 0;
    float L = lampL(wx, wy);
    if (bulb > 0) { putPix(x, y, 70, 62, 52, 0.25f, 1); return; }    // dark ceiling mount the cord hangs from (the bulb and its glow are drawn in drawGlows)
    const uint8_t* p = texAt(tCeil, frac(wx * 0.5f), frac(wy * 0.5f));
    putPix(x, y, p[0], p[1], p[2], L * 0.8f, std::max(0.04f, std::exp(-t / 16)));
}
void Renderer::drawFloorRow(const Cam& c, int x, int y, const Cell& cell, float rx, float ry) {
    float dv = (y + 0.5f) - c.hz;
    if (dv < 0.01f) return;
    float t = (c.eye - cell.F) * RH / dv, wx = c.px + t * rx, wy = c.py + t * ry;
    bulb = 0;
    float L = lampL(wx, wy);
    const uint8_t* p;
    if (cell.stair) {                                  // treads: nosing on the low edge, whichever way the flight runs
        float a = cell.stairAxis == 0 ? wx : wy, b2 = cell.stairAxis == 0 ? wy : wx, uu = frac(a);
        if (cell.stairSlope < 0) uu = 1.0f - uu;
        p = texAt(tStair, uu, frac(b2 * 0.5f));
    } else p = texAt(tFloor, frac(wx * 0.5f), frac(wy * 0.5f));
    putPix(x, y, p[0], p[1], p[2], L, std::max(0.04f, std::exp(-t / 16)));
}
// What you see through a window: open night air, a long way up. No street (we are high above it), just rain falling
// between the building and a far limit, a dim suburban skyline in three depth layers, a murky orange glow on the
// horizon, and lightning. The skyline depends only on the compass direction of the ray, so every window agrees.
RGB Renderer::outsideView(float rx, float ry, float hx, float hy, int nside, float h, float sl, float az) {
    float dr = nside == 0 ? std::fabs(rx) : std::fabs(ry);               // depth gained per unit of ray parameter
    float lr = nside == 0 ? ry : rx;                                     // lateral movement per unit
    float a0 = nside == 0 ? hy : hx;
    if (dr < 1e-4f) return {8, 10, 14};
    float e = sl / dr;                                                   // elevation: 0 = eye level, + up, - down
    float fl = wx->flash;

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
    if (wx->boltT >= 0.0f && wx->boltT < 0.3f && fl > 0.25f && e > -0.14f && e < 0.62f) {
        float dA = az - wx->boltAz; dA -= 6.2831853f * std::floor(dA / 6.2831853f + 0.5f);
        float path = 0.035f * std::sin(e * 34.0f + wx->boltSeed) + 0.012f * std::sin(e * 91.0f + wx->boltSeed * 2.3f);
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
            if (j < 2 && ((hv >> 20) & 31) == 0 && e < top + 0.03f && std::fabs(fu - 0.5f) < 0.16f && std::sin(time_ * 2.2f + (float)(hv & 7)) > 0.3f)
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
        float q = hi * 1.5f * len + time_ * (3.2f + 0.8f * (float)i + (float)((hv >> 4) & 7) * 0.2f) * len + (float)((hv >> 8) & 255) / 255.0f * 9.0f;
        float fq = frac(q);
        if (fq > 0.3f) continue;
        float k = (1.0f - fq / 0.3f) * (0.95f - 0.10f * (float)i) * (0.5f + 1.1f * fl);
        col.r += 135 * k; col.g += 160 * k; col.b += 195 * k;
    }
    return col;
}

// The lit room you glimpse through an open door: wallpaper, a lamp, a cabinet, a picture, floorboards.
RGB Renderer::roomView(float u, float v, int n) {
    float k = 0.80f + 0.20f * hs((int)(u * 90), (int)(v * 90));
    RGB c;
    if (v > 0.80f) { c = {70, 46, 30}; if (((int)(u * 16)) & 1) c = mul(c, 0.8f); k *= 0.8f; }
    else {
        c = {128, 88, 54};
        if (((int)(u * 26)) & 1) c = mul(c, 0.9f);
        float d = std::hypot((u - 0.55f) * 1.2f, v - 0.30f);
        k *= 0.50f + 0.95f * std::max(0.0f, 1.0f - d * 1.8f);
        if (d < 0.035f) return {255, 232, 176};                                       // the lamp itself
        if (u > 0.30f && u < 0.44f && v > 0.50f) c = {46, 30, 22};                    // cabinet
        if (u > 0.64f && u < 0.78f && v > 0.20f && v < 0.38f) c = (u > 0.66f && u < 0.76f && v > 0.22f && v < 0.36f) ? RGB{92, 102, 112} : RGB{58, 38, 26};   // picture
    }
    if (n == 2) { float fl = 0.75f + 0.5f * hs((int)(time_ * 14), 2); c = {72 * fl, 104 * fl, 156 * fl}; }   // the TV
    return mul(c, k * 0.9f);
}

// The lift cabin seen through the opening doors: steel walls, a ceiling light, a dark floor.
RGB Renderer::cabinView(float u, float v) {
    float k = 0.85f + 0.15f * hs((int)(u * 60), (int)(v * 60));
    RGB c = {214, 188, 138};                                      // warm light inside the cabin, against the cool steel doors
    if (v > 0.84f) c = {96, 84, 66};
    if (std::fabs(u - 0.5f) < 0.004f) c = {120, 104, 80};
    return mul(c, k * (0.7f + 0.7f * (1.0f - v)));
}

// a wall face (plain wall, door, window, handrail, lift) between rows y0..y1
void Renderer::drawWallRows(const Cam& c, int x, int y0, int y1, float tFar, float hx, float hy, float u, int nside,
                            bool flip, const Cell& cur, const Cell& wall) {
    float uu = ((nside == 0 ? wall.sy : wall.sx) * 0.5f) + u * 0.5f;    // position along the whole tile
    if (flip) uu = 1.0f - uu;
    bulb = 0;
    float L = lampL(hx, hy), f = std::max(0.04f, std::exp(-tFar / 16)) * (nside ? 0.88f : 1.0f);
    float cs = 2.0f * x / RW - 1.0f, rx = c.dx + c.plx * cs, ry = c.dy + c.ply * cs;
    // Beside the stairs the floor height jumps with every step, so a pattern measured from it would zig-zag.
    // Measure from the smooth slope through the step edges instead: trim line and handrail become true diagonals.
    float refF = cur.stair ? cur.rf0 + cur.rfx * hx + cur.rfy * hy : cur.F;
    float az = wall.type == C_WINDOW ? std::atan2(ry, rx) : 0.0f;      // compass direction of this column (windows only)
    int doorId = wall.type == C_DOOR ? std::clamp((int)wall.door, 1, DOOR_KINDS - 1) : 0;
    float oa = doorId ? lvl->doorAmt[doorId] : 0.0f;                   // how far this door is open
    const std::array<Tex, DOOR_KINDS>& dts = doorTex[(size_t)lvl->index];
    for (int y = std::max(0, y0); y < std::min(RH, y1); y++) {
        float h = c.eye - ((y + 0.5f) - c.hz) * tFar / RH;
        float v = 1.0f - (h - refF) / WALL_H;                              // texture row follows the floor (or the stair slope)
        float r, g, b;
        if (wall.type == C_WINDOW) {
            bool inPane = uu > 0.14f && uu < 0.86f && v > 0.16f && v < 0.84f;
            if (inPane && !(std::fabs(uu - 0.5f) < 0.022f || std::fabs(v - 0.46f) < 0.022f)) {
                float sl = -((y + 0.5f) - c.hz) / RH;                        // height gained per unit of ray parameter
                RGB o = outsideView(rx, ry, hx, hy, nside, h, sl, az);
                if (hs((int)(uu * 100), ((int)(v * 150) + (int)(time_ * 34)) >> 1) > 0.985f) o = {o.r * 0.6f + 80, o.g * 0.6f + 88, o.b * 0.6f + 100};   // drops on the glass
                putFlat(x, y, o.r, o.g, o.b);
                continue;
            }
            if (inPane) { float k = 0.85f + 0.3f * hs(x, y); r = 56 * k; g = 47 * k; b = 40 * k; }                          // cross bars
            else if (uu > 0.10f && uu < 0.90f && v > 0.12f && v < 0.84f) { float k = 0.85f + 0.3f * hs(x, y); r = 62 * k; g = 52 * k; b = 44 * k; }   // frame
            else if (uu > 0.07f && uu < 0.93f && v >= 0.84f && v < 0.90f) { float k = 0.9f + 0.2f * hs(x, y); r = 128 * k; g = 130 * k; b = 128 * k; }  // sill
            else { const uint8_t* p = texAt(tWall, uu, v); r = p[0]; g = p[1]; b = p[2]; }
        } else {
            if (wall.type == C_DOOR && oa > 0.001f && v >= 0.04f && uu >= 0.10f && uu < 0.90f) {
                if (doorId == DOOR_ELEVATOR) {                                 // two steel panels slide apart
                    float half = 0.40f * oa;
                    if (uu >= 0.50f - half && uu < 0.50f + half) { RGB o = cabinView(uu, v); putFlat(x, y, o.r, o.g, o.b); continue; }
                    float span = std::max(0.02f, 0.40f - half);
                    float tu = uu < 0.5f ? 0.10f + (uu - 0.10f) * 0.40f / span : 0.50f + (uu - 0.50f - half) * 0.40f / span;
                    const uint8_t* q = texAt(dts[(size_t)doorId], tu, v);
                    putPix(x, y, q[0], q[1], q[2], L * (1.1f - 0.3f * std::clamp(v, 0.0f, 1.0f)), f);
                    continue;
                }
                if (doorId <= DOOR_APT_MAX) {                                  // apartment door: the leaf swings away toward its hinge
                    float leafEnd = 0.90f - 0.74f * oa;
                    if (uu >= leafEnd) { RGB o = roomView(uu, v, doorId); putFlat(x, y, o.r, o.g, o.b); continue; }
                    float tu = 0.10f + (uu - 0.10f) * 0.80f / (leafEnd - 0.10f), kk = 1.0f - 0.45f * oa;
                    const uint8_t* q = texAt(dts[(size_t)doorId], tu, v);
                    putPix(x, y, q[0] * kk, q[1] * kk, q[2] * kk, L * (1.1f - 0.3f * std::clamp(v, 0.0f, 1.0f)), f);
                    continue;
                }
            }
            const Tex& t = wall.type == C_DOOR ? dts[(size_t)doorId]
                         : (wall.type == C_RAIL && cur.stair) ? tRail
                         : wall.mat == MAT_METAL ? tMetal : tWall;
            const uint8_t* p = texAt(t, uu, v);
            r = p[0]; g = p[1]; b = p[2];
        }
        putPix(x, y, r, g, b, L * (1.1f - 0.3f * std::clamp(v, 0.0f, 1.0f)), f);
    }
}

void Renderer::drawRiserRows(int x, int y0, int y1, int topRow, int botRow, float tFar, float hx, float hy, float u) {
    bulb = 0;
    float L = lampL(hx, hy), f = std::max(0.04f, std::exp(-tFar / 16));
    float span = (float)std::max(1, botRow - topRow);
    for (int y = std::max(0, y0); y < std::min(RH, y1); y++) {
        const uint8_t* p = texAt(tRiser, u, ((float)y - topRow) / span);
        putPix(x, y, p[0], p[1], p[2], L, f);
    }
}

void Renderer::renderColumn(int x, const Cam& c) {
    float cs = 2.0f * x / RW - 1.0f, rx = c.dx + c.plx * cs, ry = c.dy + c.ply * cs;
    int mx = (int)std::floor(c.px), my = (int)std::floor(c.py);
    const float BIG = 1e30f;
    float ddx = std::fabs(rx) < 1e-9f ? BIG : std::fabs(1.0f / rx), ddy = std::fabs(ry) < 1e-9f ? BIG : std::fabs(1.0f / ry);
    int stx, sty; float sdx, sdy;
    if (rx < 0) { stx = -1; sdx = (c.px - mx) * ddx; } else { stx = 1; sdx = (mx + 1 - c.px) * ddx; }
    if (ry < 0) { sty = -1; sdy = (c.py - my) * ddy; } else { sty = 1; sdy = (my + 1 - c.py) * ddy; }

    int yT = 0, yB = RH;                 // rows still undrawn: [yT, yB)
    float tNear = 0.02f;
    zbuf[x] = 1e9f;
    for (int it = 0; it < 200 && yT < yB; it++) {
        const Cell& cur = lvl->cellAt(mx, my);
        if (cur.type != C_OPEN) break;   // started inside a wall (cannot happen with collision, but never crash)
        int nmx = mx, nmy = my, nside;
        float tFar;
        if (sdx < sdy) { tFar = sdx; sdx += ddx; nmx += stx; nside = 0; }
        else { tFar = sdy; sdy += ddy; nmy += sty; nside = 1; }
        if (tFar < tNear) tFar = tNear;

        // ceiling of this cell between the near and far edge
        {
            int a = toRow(projY(c.hz, c.eye, cur.C, tNear)), b = toRow(projY(c.hz, c.eye, cur.C, tFar));
            if (cur.C > c.eye + 0.01f)
                for (int y = std::max(yT, a); y < std::min(yB, b); y++) drawCeilRow(c, x, y, cur, rx, ry);
            yT = std::max(yT, b);
        }
        // floor of this cell
        {
            int a = toRow(projY(c.hz, c.eye, cur.F, tFar)), b = toRow(projY(c.hz, c.eye, cur.F, tNear));
            if (cur.F < c.eye - 0.01f)
                for (int y = std::max(yT, a); y < std::min(yB, b); y++) drawFloorRow(c, x, y, cur, rx, ry);
            yB = std::min(yB, a);
        }
        if (yT >= yB) break;

        const Cell& nx = lvl->cellAt(nmx, nmy);
        float hx = c.px + tFar * rx, hy = c.py + tFar * ry;
        float u = frac(nside == 0 ? hy : hx);
        if (nx.type != C_OPEN) {         // solid neighbour: its face fills the rest of the column
            bool flip = (nside == 0 && rx > 0) || (nside == 1 && ry < 0);
            drawWallRows(c, x, yT, yB, tFar, hx, hy, u, nside, flip, cur, nx);
            zbuf[x] = tFar;
            // an open doorway is a hole in the wall: whoever stands in it must not be hidden by the wall plane
            if (nx.type == C_DOOR && nx.door >= 1 && nx.door <= DOOR_APT_MAX && lvl->doorAmt[nx.door] > 0.3f) zbuf[x] = tFar + 2.5f;
            if (x == RW / 2) { focus.valid = true; focus.type = nx.type; focus.door = nx.door; focus.cx = nmx; focus.cy = nmy; focus.t = tFar; }
            break;
        }
        // open neighbour: draw the riser (floor goes up) and the ceiling step (ceiling comes down)
        if (nx.F > cur.F + 1e-4f) {
            int top = toRow(projY(c.hz, c.eye, nx.F, tFar)), bot = toRow(projY(c.hz, c.eye, cur.F, tFar));
            drawRiserRows(x, std::max(yT, top), yB, top, bot, tFar, hx, hy, u);
            yB = std::min(yB, top);
        }
        if (nx.C < cur.C - 1e-4f) {
            int bot = toRow(projY(c.hz, c.eye, nx.C, tFar));
            for (int y = std::max(0, yT); y < std::min(yB, bot); y++) {
                float h = c.eye - ((y + 0.5f) - c.hz) * tFar / RH;
                const uint8_t* p = texAt(tWall, u, 0.05f + 0.4f * frac(h));         // plaster, varied by height (no streaks)
                putPix(x, y, p[0], p[1], p[2], lampL(hx, hy) * 0.8f, std::max(0.04f, std::exp(-tFar / 16)));
            }
            yT = std::max(yT, bot);
        }
        mx = nmx; my = nmy; tNear = tFar;
    }
}

// ---------------------------------------------------------------- bulb glows
// Hanging bulbs: a cord, a hot core and a soft halo drawn on top of the picture (additive). The halo is the bulb's
// own glow - it adds no light to the scene. Columns where a wall is nearer than the bulb are skipped.
void Renderer::drawGlows(const Cam& c) {
    static float lut[1025];                                                  // halo profile by squared distance (0..1), built once
    static bool lutOk = false;
    if (!lutOk) {
        for (int i = 0; i <= 1024; i++) {
            float d2 = (float)i / 1024.0f, d = std::sqrt(d2);
            lut[i] = std::min(1.5f, 1.0f * std::exp(-d2 * 220.0f) + 0.62f * std::exp(-d2 * 22.0f) + 0.36f * (1 - d) * (1 - d) * (1 - d));
        }
        lutOk = true;
    }
    for (const Lamp& l : lvl->lamps) {
        if (l.win || l.k <= 0.002f) continue;
        float rx = l.x - c.px, ry = l.y - c.py, ty = rx * c.dx + ry * c.dy;
        if (ty < 0.35f) continue;
        float tx = (c.dx * ry - c.dy * rx) / PLANE;
        float bulbH = lvl->cellAt((int)std::floor(l.x), (int)std::floor(l.y)).C - 0.35f;
        float ceilH = bulbH + 0.35f;
        float sx = RW / 2.0f * (1 + tx / ty), sy = c.hz - (bulbH - c.eye) * RH / ty, sc = c.hz - (ceilH - c.eye) * RH / ty;
        float R = std::min(100.0f, 1.5f * RH / ty);                         // halo profile radius in pixels (1.5 cells)
        float Rb = R * 0.72f;                                               // beyond this the halo is too faint to see: skip it
        if (sx < -Rb || sx > RW + Rb) continue;                             // far off to the side: nothing to draw
        int xa = clampi(std::floor(sx - Rb), 0, RW), xb = clampi(std::ceil(sx + Rb), 0, RW);
        int ya = clampi(std::floor(sy - Rb), 0, RH), yb = clampi(std::ceil(sy + Rb), 0, RH);
        int cx = clampi(sx, 0, RW - 1);
        if (zbuf[cx] >= ty - 0.1f)                                            // cord
            for (int y = clampi(std::floor(sc), 0, RH); y < clampi(std::floor(sy), 0, RH); y++) putFlat(cx, y, 34, 30, 26);
        for (int x = xa; x < xb; x++) {
            if (zbuf[x] < ty - 0.1f) continue;
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


// ---------------------------------------------------------------- billboards (people and props)
// Every billboard is two crossed vertical planes: one faces along the heading and shows the front or the back
// (whichever side you are on), the other is perpendicular and shows the side (mirrored from the other side).
void Renderer::drawBillboards(const Cam& c, const vector<Billboard>& bills) {
    struct Item { const Billboard* s; float ty, tx; int idx; };
    vector<Item> list;
    for (size_t i = 0; i < bills.size(); i++) {
        const Billboard& s = bills[i];
        float rx = s.x - c.px, ry = s.y - c.py, ty = rx * c.dx + ry * c.dy;
        if (ty > 0.4f) list.push_back({&s, ty, (c.dx * ry - c.dy * rx) / PLANE, (int)i});
    }
    std::sort(list.begin(), list.end(), [](const Item& a, const Item& b) { return a.ty > b.ty; });   // far to near
    nearBill = -1;
    for (const Item& it : list) {
        const Billboard& s = *it.s;
        const SpriteSet& set = spriteSet(s.art);
        float H = s.h * WALL_H;                                              // world height
        float fx = std::cos(s.yaw), fy = std::sin(s.yaw);                    // heading
        float cxv = c.px - s.x, cyv = c.py - s.y;                            // sprite -> camera
        bool viewFront = cxv * fx + cyv * fy >= 0;
        const SpriteArt& A = viewFront ? set.front : set.back;               // plane A: across the heading, front or back
        const SpriteArt& B = set.side;                                       // plane B: along the heading, the side
        float hw[2] = {H * A.w / A.h * 0.5f, H * B.w / B.h * 0.5f};
        float Dx[2] = {fy, fx}, Dy[2] = {-fx, fy};                           // plane directions: A = (fy,-fx), B = (fx,fy)
        const SpriteArt* art[2] = {&A, &B};
        float rad = std::max(hw[0], hw[1]);
        bulb = 0;
        float L = lampL(s.x, s.y) + 0.12f + s.lit;
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
                if (t < 0.15f || std::fabs(sp) > hw[pl] || zbuf[x] < t) continue;
                float u = pl == 0 ? 0.5f + (viewFront ? sp : -sp) / (2 * hw[0]) : 0.5f - sp / (2 * hw[1]);
                hits[nh++] = {pl, t, u};
            }
            if (nh == 2 && hits[0].t < hits[1].t) std::swap(hits[0], hits[1]);   // far plane first
            for (int k = 0; k < nh; k++) {
                const Hit& ht = hits[k];
                const SpriteArt& a = *art[ht.pl];
                float top = c.hz - (s.floorH + H - c.eye) * RH / ht.t, bot = c.hz - (s.floorH - c.eye) * RH / ht.t;
                if (bot - top < 0.5f) continue;
                int u = clampi(ht.u * a.w, 0, a.w - 1);
                int ya = clampi(std::floor(top), 0, RH), yb = clampi(std::ceil(bot), 0, RH);
                float ft = std::max(0.05f, std::exp(-ht.t / 16));
                for (int y = ya; y < yb; y++) {
                    int v = clampi((y + 0.5f - top) / (bot - top) * a.h, 0, a.h - 1);
                    size_t ai = (size_t)v * a.w + u;
                    char ch = a.c[ai];
                    RGB col;
                    if (ch == ' ' || !spriteColor(ch, s.main, s.acc, col)) continue;
                    int8_t sd = a.s[ai];
                    col = mul(col, sd < 0 ? 0.62f : sd > 0 ? 1.28f : 1.0f);
                    if (ch == 'F') putPix(x, y, col.r, col.g, col.b, 1.4f, std::max(ft, 0.6f));   // eyes glow
                    else putPix(x, y, col.r, col.g, col.b, L, ft);
                }
            }
        }
        if (s.who && it.ty < 4.8f && std::fabs(it.tx / it.ty) < 0.3f && it.ty < zbuf[RW / 2] + 0.1f) nearBill = it.idx;   // the person you are looking at
    }
}

// ---------------------------------------------------------------- one frame
void Renderer::render(const Level& level, const CamState& cs, const vector<Billboard>& bills, const Weather& weather, float time) {
    lvl = &level; wx = &weather; time_ = time;
    Cam c;
    c.px = cs.x; c.py = cs.y; c.eye = cs.feet + EYE_H; c.hz = RH / 2.0f + cs.bob + cs.pitch;
    c.dx = std::cos(cs.yaw); c.dy = std::sin(cs.yaw);
    c.plx = -c.dy * PLANE; c.ply = c.dx * PLANE;
    for (int i = 0; i < RW * RH; i++) pix[i] = 0xff0b0907u;
    focus = Focus();
    for (int x = 0; x < RW; x++) renderColumn(x, c);
    drawGlows(c);
    drawBillboards(c, bills);
    frame++;
}

// ---------------------------------------------------------------- lightning: a bolt every 6-20 seconds: flash, flicker, flash, fade
void Weather::update(float dt, float time, float playerYaw) {
    if (boltT < 0 && time >= nextBolt) {
        boltT = 0;
        boltAz = playerYaw + ((float)(hash32(bolts, 5, 9) & 1023) / 1023.0f - 0.5f) * 1.3f;   // strike somewhere near where the player is looking
        boltSeed = (float)(hash32(bolts, 6, 2) & 1023) / 100.0f;
    }
    if (boltT < 0) { flash = 0; return; }
    boltT += dt;
    float t = boltT;
    flash = t < 0.07f ? 1.0f : t < 0.16f ? 0.2f : t < 0.24f ? 0.9f : 0.9f * std::exp(-(t - 0.24f) * 7.0f);
    if (t > 1.4f) {
        boltT = -1; flash = 0; bolts++;
        nextBolt = time + 6.0f + 14.0f * (float)(hash32(bolts, 17, 3) & 1023) / 1023.0f;
    }
}
