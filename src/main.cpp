// Block 7 - ASCII raycaster detective demo (C++17 + raylib 5.x)
// Controls: W/S move, A/D turn, E talk/knock, T ramp style, R rain, F torch, C case file
#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using std::string;
using std::vector;

// ---------- screen / grid ----------
constexpr int GW = 120, GH = 40;          // character grid
constexpr int CW = 8, CH = 15;            // pixels per character cell
constexpr int LOG_H = 125;                // text panel under the grid
constexpr int SCR_W = GW * CW, SCR_H = GH * CH + LOG_H;

// ---------- map ----------
constexpr int MW = 16, MH = 15;
static const char* MAP[MH] = {
    "################", "#..............#", "#.####1##2####.#", "#.############.#",
    "#.3##########4.#", "#.############.#", "#.############.#", "#.5##########6.#",
    "#.############.#", "#.############.#", "#.############.#", "#.############.#",
    "#.####7##8####.#", "#..............#", "################"};

static char tileAt(int x, int y) {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return '#';
    return MAP[y][x];
}

struct Door { const char* apt; const char* text; const char* clue; };
static const Door DOORS[9] = {
    {"", "", nullptr},
    {"101", "A chain rattles. An old woman: \"Shouting, two nights back. Around two a.m. Now go away.\"",
     "Witness heard an argument around 2 a.m."},
    {"102", "No answer. A TV drones behind the door.", nullptr},
    {"103", "A tired man: \"Night shift. Wasn't even home. Ask the kid in 104.\"", nullptr},
    {"104", "A boy through the gap: \"Man in 106 owed money. Big guy came by. Red scarf.\"",
     "Debt collector with a red scarf visited 106."},
    {"105", "No answer. Mail piles up on the mat.", nullptr},
    {"106", "Crime scene. Tape across the frame. A size-44 boot print in the mud, red wool caught on the hinge.",
     "Size-44 boot print and red wool fiber at the scene."},
    {"107", "Boiled cabbage smell. The door stays shut.", nullptr},
    {"108", "A radio plays softly. No one answers.", nullptr}};

// ---------- palette ----------
enum Pal { P_WALL, P_DOOR, P_FLOOR, P_CEIL, P_RAIN, P_TAPE, P_LAMP, P_KNOB, P_EDGE,
           P_NPC, P_NPC2, P_NPC3, P_CAT, P_PLANT, P_BIN, P_CRATE, P_WIN, P_POSTER };
// Restrained palette: slate for the world, amber for light and tape, rust for doors and the one red accent.
static const Color PALC[] = {
    {140, 158, 170, 255}, {190, 100, 48, 255},  {64, 88, 98, 255},    {56, 70, 84, 255},
    {105, 135, 165, 255}, {232, 168, 48, 255},  {255, 196, 90, 255},  {250, 214, 140, 255},
    {170, 190, 202, 255}, {200, 150, 130, 255}, {120, 160, 190, 255}, {190, 60, 45, 255},
    {215, 140, 60, 255},  {90, 130, 100, 255},  {130, 140, 150, 255}, {150, 115, 80, 255},
    {90, 130, 170, 255},  {165, 72, 56, 255}};
static const Color COL_ME = {232, 176, 74, 255};
static const Color COL_SYS = {110, 125, 135, 255};
static const Color COL_WATCH = {196, 106, 106, 255};

// brightness -> colour. Bright = warm lamp light, dark = cool and dim (never fully black, so shapes stay readable)
static Color shadeC(Color c, float b) {
    float f = 0.28f + 0.72f * std::pow(b, 0.85f);
    float warm = std::clamp((b - 0.45f) * 1.6f, 0.0f, 1.0f);
    float r = c.r * f * (0.92f + 0.22f * warm), g = c.g * f * (0.95f + 0.05f * warm), bl = c.b * f * (1.05f - 0.30f * warm);
    return Color{(unsigned char)std::min(255.0f, r), (unsigned char)std::min(255.0f, g), (unsigned char)std::min(255.0f, bl), 255};
}

// ---------- frame buffer (the "screen" is just three arrays) ----------
static char gCh[GW * GH];
static unsigned char gPal[GW * GH];
static float gB[GW * GH];

static Color gCol[GW * GH];   // per-cell colour override (used by sprites); gPal == 255 means "use gCol"

static void put(int i, char c, int pal, float b) {
    gCh[i] = c;
    gPal[i] = (unsigned char)pal;
    gB[i] = std::clamp(b, 0.0f, 1.0f);
}
static void put2(int i, char c, Color col, float b) {
    gCh[i] = c;
    gPal[i] = 255;
    gCol[i] = col;
    gB[i] = std::clamp(b, 0.0f, 1.0f);
}
static Color cellColor(int i) { return shadeC(gPal[i] == 255 ? gCol[i] : PALC[gPal[i]], gB[i]); }

// Block-element characters live in the grid as small sentinel codes and become real Unicode glyphs at draw time.
constexpr char BLK_FULL = 1, BLK_DARK = 2, BLK_MED = 3, BLK_LIGHT = 4, BLK_LOW = 5, BLK_UP = 6;   // full, dark, medium, light, lower half, upper half
static bool gBlockOK = false;   // true once a TTF with the block glyphs has been loaded
static const char* STYLE_NAMES[3] = {"blocks", "ascii", "dense"};
static const char* RAMPS[3] = {
    " \x04\x03\x02\x01",
    " .:-=+*#%@",
    " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"};
static int gStyle = 0;   // 0 = blocks (default), 1 = plain ascii, 2 = dense ascii
static char solid() { return gStyle == 0 ? BLK_FULL : '#'; }
static char mid() { return gStyle == 0 ? BLK_DARK : '%'; }
static int toCodepoint(char c) {
    static const int uni[7] = {0, 0x2588, 0x2593, 0x2592, 0x2591, 0x2584, 0x2580};
    static const char asc[7] = {0, '#', '%', ':', '.', '_', '^'};
    if (c >= 1 && c <= 6) return gBlockOK ? uni[(int)c] : asc[(int)c];
    return c;
}
static const int BAYER[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

// brightness -> character, with ordered dithering so the bands are not visible
[[maybe_unused]] static char ramp(float b, int x, int y) {
    const char* r = RAMPS[gStyle];
    int n = (int)std::strlen(r);
    float v = b + (BAYER[(y & 3) * 4 + (x & 3)] / 16.0f - 0.47f) * 0.1f;
    return r[std::clamp((int)(v * n), 0, n - 1)];
}

// same ramp without dithering: used on walls/doors where noise would hurt readability (colour carries the shading)
static char rampN(float b) {
    const char* r = RAMPS[gStyle];
    int n = (int)std::strlen(r);
    return r[std::clamp((int)(b * n), 0, n - 1)];
}

static float hash2(int a, int b) {
    float s = std::sin(a * 12.9898f + b * 78.233f) * 43758.5453f;
    return std::fabs(s - std::floor(s));
}

// ---------- lighting ----------
static bool gTorch = true;
static float gFlick = 1.0f;

// Doom-style light pools: every ceiling lamp lights the world around it (walls, floor, sprites)
static vector<Vector2> LAMPS;
static float LF[16];   // per-lamp strength, so one can buzz and another can be dying

static float lampLight(float wx, float wy) {
    float sum = 0;
    for (size_t i = 0; i < LAMPS.size(); i++) {
        float k = std::max(0.0f, 1 - std::hypot(wx - LAMPS[i].x, wy - LAMPS[i].y) / 3.2f);
        sum += k * k * LF[i];
    }
    return sum;
}

// d = distance from camera, (wx,wy) = world position being lit, (x,y) = screen cell (for the torch cone)
static float light(float d, int x, int y, int side, float wx, float wy) {
    float fx = (float)x / GW - 0.5f, fy = ((float)y / GH - 0.5f) * 0.8f;
    float r = std::sqrt(fx * fx * 2 + fy * fy * 2.2f);
    float cone = gTorch ? (r < 0.16f ? 1.0f : std::max(0.0f, 1 - (r - 0.16f) * 3.4f)) : 0.0f;
    float fog = std::exp(-d / 12);
    float amb = gTorch ? 0.10f + 0.08f * fog : 0.16f + 0.12f * fog;
    return std::min(1.0f, (amb + lampLight(wx, wy) * 0.75f + cone * 0.5f * std::exp(-d / 16)) * gFlick * (side ? 0.85f : 1.0f));
}

// ---------- sprites (NPCs + decorations) ----------
enum Tex { T_MAN, T_OLD, T_CAT, T_PLANT, T_BIN, T_CRATE };
// Pixel-art sprites. Each letter is a paint colour (see glyphFor); ' ' is transparent.
static const vector<vector<string>> TEX = {
    {   // T_MAN: hat, face, scarf, coat, trousers, boots
        "   HHHH   ", "  HHHHHH  ", " HHHHHHHH ", "  SSSSSS  ", "  SESSES  ", "  SSSSSS  ", "   SSSS   ",
        "  RRRRRR  ", " CCRRRRCC ", "CCCCRRCCCC", "CCCCCCCCCC", "CCSCCCCSCC", "CCCCCCCCCC", " CCCCCCCC ",
        " CCCCCCCC ", "  PPP PPP ", "  PPP PPP ", " BBBB BBBB"},
    {   // T_OLD: grey hair, shawl, dress
        "   WWWW   ", "  WWWWWW  ", "  WSSSSW  ", "  SESSES  ", "  SSSSSS  ", "   SSSS   ", "  RRRRRR  ",
        " RRRRRRRR ", "RRRRRRRRRR", "RRRRRRRRRR", "SRRRRRRRRS", " CCCCCCCC ", " CCCCCCCC ", "CCCCCCCCCC",
        "CCCCCCCCCC", "  BB  BB  "},
    {   // T_CAT: ears, eyes, body, tail, legs
        "  K K         ", "  KKKK      K ", " KKYKYK     K ", "  KKKKKKKKKKK ", " KKKKKKKKKKKK ",
        " KKKKKKKKKKKK ", "  KK  KK  KK  ", "  KK  KK  KK  "},
    {   // T_PLANT
        "    GG    ", "  G GG G  ", " GGGGGGGG ", "  GGGGGG  ", " G GGGG G ", "   GGGG   ", "    GG    ",
        "   TTTT   ", "  TTTTTT  ", "  TTTTTT  ", "   TTTT   ", "   TTTT   "},
    {   // T_BIN
        " CCCCCCC ", "CCCCCCCCC", " CDCDCDC ", " CDCDCDC ", " CDCDCDC ", " CCCCCCC "},
    {   // T_CRATE
        "CCCCCCCCCC", "CDCCCCCCDC", "CCDCCCCDCC", "CCCDCCDCCC", "CCCCDDCCCC", "CCCDCCDCCC", "CCDCCCCDCC",
        "CCCCCCCCCC"}};

struct Sprite {
    string key, name;
    int tex = 0, pal = 0;
    float h = 1, x = 0, y = 0;     // h = fraction of wall height
    bool talk = false;
    float speed = 0;
    vector<Vector2> wp;            // waypoint loop (a tiny graph walk)
    int wi = 0, n = 0;
    float pause = 0;
    Color main{200, 200, 200, 255}, accent{200, 60, 60, 255};   // coat / scarf (or body colour for the cat)
};
static vector<Sprite> SP;
static Sprite* gFocusS = nullptr;

static Sprite mk(const char* key, const char* name, int tex, int pal, float h, float x, float y, bool talk) {
    Sprite s;
    s.key = key; s.name = name; s.tex = tex; s.pal = pal; s.h = h; s.x = x; s.y = y; s.talk = talk;
    return s;
}

struct Glyph { char c; Color col; };
static Glyph glyphFor(char p, const Sprite& s) {
    auto dim = [](Color c, float k) { return Color{(unsigned char)(c.r * k), (unsigned char)(c.g * k), (unsigned char)(c.b * k), 255}; };
    switch (p) {
        case 'H': return {solid(), Color{84, 90, 102, 255}};
        case 'S': return {gStyle == 0 ? BLK_FULL : '@', Color{210, 176, 150, 255}};
        case 'E': return {gStyle == 0 ? BLK_FULL : '@', Color{22, 24, 32, 255}};
        case 'C': return {mid(), s.main};
        case 'D': return {solid(), dim(s.main, .55f)};
        case 'R': return {solid(), s.accent};
        case 'P': return {gStyle == 0 ? BLK_DARK : 'H', Color{66, 78, 100, 255}};
        case 'B': return {solid(), Color{58, 60, 66, 255}};
        case 'W': return {mid(), Color{200, 206, 212, 255}};
        case 'K': return {solid(), s.main};
        case 'Y': return {gStyle == 0 ? BLK_FULL : 'o', Color{236, 200, 90, 255}};
        case 'G': return {gStyle == 0 ? BLK_DARK : '&', Color{84, 128, 96, 255}};
        case 'T': return {solid(), Color{130, 96, 70, 255}};
        default: return {'?', WHITE};
    }
}

static std::unordered_map<int, int> gDeco;   // wall decorations: 1 = window, 2 = poster

// ---------- game state ----------
static vector<std::pair<string, Color>> gLog;
static std::unordered_map<string, string> gClues;
static vector<string> gClueOrder;
static std::unordered_set<int> gKnocked;
static bool gDone = false;

static void say(const string& t, Color c) { gLog.push_back({t, c}); }

static void addClue(const string& key, const string& text) {
    if (gClues.count(key)) return;
    gClues[key] = text;
    gClueOrder.push_back(text);
    say("Clue logged: " + text, COL_SYS);
    if (gClues.size() == 4) say("Case file: enough to confront the man in the red scarf. He keeps pacing the halls.", COL_SYS);
}

static void talk(Sprite& s) {
    s.n++;
    auto line = [&](const string& t) { say(s.name + ": \"" + t + "\"", COL_ME); };
    if (s.key == "vol") {
        line(s.n == 1 ? "Shouting at two a.m. Anton in 106 owed money. Pavel the janitor sees everything."
                      : s.n == 2 ? "I told you already. Mind Boris, he bites." : "You people never listen.");
    } else if (s.key == "pav") {
        line("I saw a man in a red scarf take the south stairs at ten past two.");
        addClue("pav", "Janitor saw a red-scarfed man on the south stairs at ~2:10 a.m.");
    } else if (s.key == "cat") {
        say("You crouch. Boris hisses, a red wool thread caught on his collar.", COL_ME);
        addClue("cat", "Red wool on the cat's collar: the scarf man was in this hallway.");
    } else if (s.key == "igor") {
        s.pause = 4;
        if (gClues.size() < 4) {
            line(s.n == 1 ? "Detective? I was home all night." : "I have nothing to say.");
            if (s.n == 1) say("Watch AI: \"He's lying. Ask me nicely and I might tell you why.\"", COL_WATCH);
        } else {
            line("...Anton owed us. It got out of hand. I didn't mean to.");
            gDone = true;
            say("CASE CLOSED. You collected " + std::to_string(gClues.size()) + " clues.", COL_SYS);
            say("Watch AI: \"Fine. We make a decent team. Don't tell anyone I said that.\"", COL_WATCH);
        }
    }
}

static void knock(int n) {
    if (gKnocked.count(n)) { say(string("Apt ") + DOORS[n].apt + ": nothing new.", COL_SYS); return; }
    gKnocked.insert(n);
    say((n == 6 ? string("Apt ") + DOORS[n].apt + " - " : string("Knock knock, Apt ") + DOORS[n].apt + ". ") + DOORS[n].text, COL_ME);
    if (DOORS[n].clue) addClue("d" + std::to_string(n), DOORS[n].clue);
}

// ---------- renderer ----------
static int cid[GW], cs[GW];
static float cd[GW], cds[GW], cde[GW], chx[GW], chy[GW];
static float gPL = 0.5f * (float)(GW * CW) / (float)(GH * CH);

static void renderWorld(float px, float py, float ang, int tick, char& focusTile, float& focusDist) {
    float dx = std::cos(ang), dy = std::sin(ang);
    float plx = -dy * gPL, ply = dx * gPL;
    std::fill(gCh, gCh + GW * GH, ' ');

    for (int x = 0; x < GW; x++) {
        // ---- DDA: step ray through the tile grid until it hits a non-floor tile ----
        float cx = 2.0f * x / GW - 1, rx = dx + plx * cx, ry = dy + ply * cx;
        int mx = (int)px, my = (int)py;
        float ddx = std::fabs(1 / rx), ddy = std::fabs(1 / ry), sdx, sdy;
        int sx, sy, side = 0;
        if (rx < 0) { sx = -1; sdx = (px - mx) * ddx; } else { sx = 1; sdx = (mx + 1 - px) * ddx; }
        if (ry < 0) { sy = -1; sdy = (py - my) * ddy; } else { sy = 1; sdy = (my + 1 - py) * ddy; }
        for (int n = 0; n < 40; n++) {
            if (sdx < sdy) { sdx += ddx; mx += sx; side = 0; } else { sdy += ddy; my += sy; side = 1; }
            if (tileAt(mx, my) != '.') break;
        }
        char tile = tileAt(mx, my);
        float dist = std::max(0.05f, side ? sdy - ddy : sdx - ddx);
        float wx = side ? px + dist * rx : py + dist * ry;
        wx -= std::floor(wx);
        if (x == GW / 2) { focusTile = tile; focusDist = dist; }

        float lh = GH / dist, ds = (GH - lh) / 2, de = ds + lh;
        bool isDoor = tile != '#';
        cid[x] = (mx * 64 + my) * 2 + side; cd[x] = dist; cds[x] = ds; cde[x] = de; cs[x] = side;
        chx[x] = px + dist * rx; chy[x] = py + dist * ry;   // world position the ray hit (for lamp lighting)

        for (int y = 0; y < GH; y++) {
            int i = y * GW + x;
            if (y >= ds && y < de) {                       // ---- wall / door column ----
                float b = light(dist, x, y, side, chx[x], chy[x]), ty = (y - ds) / lh;
                if (lh > 4 && y - ds < 1) { put(i, gStyle == 0 ? BLK_UP : '-', P_EDGE, b + 0.2f); continue; }
                if (lh > 4 && de - y <= 1) { put(i, gStyle == 0 ? BLK_LOW : '_', P_EDGE, b + 0.2f); continue; }
                if (isDoor) {
                    int n = tile - '0';
                    float bd = std::max(b, 0.5f);
                    if (n == 6 && ((ty > .38f && ty < .45f) || (ty > .6f && ty < .67f))) { put(i, 'X', P_TAPE, 1); continue; }
                    if (lh > 12 && dist > 1.5f && ty > .14f && ty < .26f && wx > .3f && wx < .7f) {
                        put(i, DOORS[n].apt[std::min(2, (int)((wx - .3f) / .4f * 3))], P_KNOB, 1);
                        continue;
                    }
                    if (wx < .07f || wx > .93f || ty < .08f) put2(i, solid(), Color{112, 60, 30, 255}, bd);          // frame
                    else if (wx > .72f && wx < .8f && ty > .5f && ty < .57f) put(i, 'o', P_KNOB, 1);               // knob
                    else if (wx > .2f && wx < .8f && ((ty > .3f && ty < .45f) || (ty > .55f && ty < .9f))) put2(i, mid(), Color{214, 122, 54, 255}, bd);  // raised panels
                    else put2(i, solid(), Color{170, 92, 42, 255}, bd);
                } else {
                    auto it = gDeco.find(my * MW + mx);
                    if (it != gDeco.end() && lh > 6 && wx > .2f && wx < .8f && ty > .2f && ty < .6f) {
                        bool edge = wx < .26f || wx > .74f || ty < .26f || ty > .54f;
                        if (it->second == 1) put(i, edge ? solid() : ((x * 7 + y * 3 + tick) % 6 < 1 ? '/' : (gStyle == 0 ? BLK_LIGHT : ':')), P_WIN, edge ? std::max(b, .5f) : .55f);
                        else put(i, edge ? solid() : (hash2(x >> 1, y) > .5f ? mid() : (gStyle == 0 ? BLK_MED : '=')), P_POSTER, std::max(b, .5f));
                    } else {
                        // Soviet-style hallway wall: dark painted lower half, pale plaster above, a trim line, damp stains
                        float stain = hash2((int)(wx * 5), mx * 3 + my * 7);
                        float grime = (stain > .86f && ty < .55f) ? .6f : 1.0f;
                        if (ty > .52f && ty < .57f) put2(i, '=', Color{110, 126, 138, 255}, std::max(b, .35f));
                        else if (ty >= .57f) put2(i, rampN(b), Color{66, 92, 106, 255}, b);
                        else put2(i, rampN(b * grime), Color{150, 166, 176, 255}, b * grime);
                    }
                }
            } else {                                        // ---- floor / ceiling ----
                float rd = GH / std::fabs(2 * (y + .5f) - GH);
                bool isFloor = y >= de;
                float fwx = px + rd * rx, fwy = py + rd * ry, b = light(rd, x, y, 0, fwx, fwy);
                if (isFloor) {
                    float gx = fwx - std::floor(fwx), gy = fwy - std::floor(fwy);
                    float h = hash2((int)std::floor(fwx * 3), (int)std::floor(fwy * 3));
                    if (rd < 7 && gx < .05f && gy < .05f) put(i, '+', P_FLOOR, b + .15f);
                    else if (rd < 7 && gy < .05f) put(i, '-', P_FLOOR, b + .1f);
                    else if (rd < 7 && gx < .05f) put(i, '|', P_FLOOR, b + .1f);
                    else if (h > .94f) put(i, '~', P_RAIN, b * 1.4f);
                    else {   // checkered grimy tiles
                        float t = (((int)std::floor(fwx * 2) + (int)std::floor(fwy * 2)) & 1) ? 1.0f : .8f;
                        put(i, rampN(b * .6f * t), P_FLOOR, b * t);
                    }
                } else {
                    int lampIdx = -1;
                    for (size_t li = 0; li < LAMPS.size(); li++)
                        if (std::hypot(fwx - LAMPS[li].x, fwy - LAMPS[li].y) < .11f) lampIdx = (int)li;
                    if (lampIdx >= 0) put(i, solid(), P_LAMP, .35f + .65f * LF[lampIdx]);
                    else put(i, rampN(b * .35f), P_CEIL, b * .8f);
                }
            }
        }
    }
    // ---- vertical outlines where the visible wall changes ----
    for (int x = 1; x < GW; x++) {
        if (cid[x] == cid[x - 1]) continue;
        int k = cd[x] < cd[x - 1] ? x : x - 1;
        int a = std::max(0, (int)std::ceil(cds[k])), e = std::min(GH, (int)std::floor(cde[k]));
        for (int y = a; y < e; y++) put(y * GW + k, '|', P_EDGE, light(cd[k], k, y, cs[k], chx[k], chy[k]) + .25f);
    }
    // ---- billboard sprites, far to near, depth-tested against the wall distance per column ----
    gFocusS = nullptr;
    struct Item { Sprite* s; float ty, tx; };
    vector<Item> list;
    for (auto& s : SP) {
        float rx = s.x - px, ry = s.y - py, ty = rx * dx + ry * dy;
        if (ty > .15f) list.push_back({&s, ty, (dx * ry - dy * rx) / gPL});
    }
    std::sort(list.begin(), list.end(), [](const Item& a, const Item& b) { return a.ty > b.ty; });
    for (auto& it : list) {
        const auto& tex = TEX[it.s->tex];
        int th = (int)tex.size(), tw = (int)tex[0].size();
        float rows = GH / it.ty * it.s->h, cols = rows * ((float)tw / th) * ((float)CH / CW);
        float sxc = GW / 2.0f * (1 + it.tx / it.ty), bot = GH / 2.0f + GH / it.ty / 2, top = bot - rows;
        float x0 = sxc - cols / 2, x1 = sxc + cols / 2;
        for (int x = std::max(0, (int)std::floor(x0)); x < std::min(GW, (int)std::ceil(x1)); x++) {
            if (it.ty > cd[x]) continue;
            int u = std::clamp((int)((x - x0) / (x1 - x0) * tw), 0, tw - 1);
            for (int y = std::max(0, (int)std::floor(top)); y < std::min(GH, (int)std::ceil(bot)); y++) {
                int v = std::clamp((int)((y - top) / rows * th), 0, th - 1);
                char pxl = tex[v][u];
                if (pxl != ' ') {
                    Glyph g = glyphFor(pxl, *it.s);
                    put2(y * GW + x, g.c, g.col, std::max(.58f, light(it.ty, x, y, 0, it.s->x, it.s->y) + .25f));
                }
            }
        }
        if (it.s->talk && it.ty < 2.6f && std::fabs(sxc - GW / 2.0f) < std::max(8.0f, cols / 2) && it.ty < cd[GW / 2] + .01f)
            gFocusS = it.s;
    }
}

// A real monospace TTF makes the grid crisp. Falls back to raylib's built-in font if none is found.
// To ship your own, drop a .ttf at assets/mono.ttf next to the exe.
static Font loadMono(int size) {
    const char* paths[] = {"assets/mono.ttf", "C:/Windows/Fonts/consolab.ttf", "C:/Windows/Fonts/consola.ttf",
                           "C:/Windows/Fonts/lucon.ttf", "C:/Windows/Fonts/cour.ttf",
                           "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
                           "/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf"};
    for (const char* p : paths) {
        if (!FileExists(p)) continue;
        int cps[95 + 6], n = 0;
        for (int c = 32; c < 127; c++) cps[n++] = c;
        for (int c : {0x2588, 0x2593, 0x2592, 0x2591, 0x2584, 0x2580}) cps[n++] = c;   // block elements
        Font f = LoadFontEx(p, size, cps, n);
        if (f.texture.id != 0) { SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR); gBlockOK = true; return f; }
    }
    return GetFontDefault();
}

static void setupWorld() {
    // ---- world setup ----
    for (int i = 2; i < 14; i += 3) { gDeco[0 * MW + i] = 1; gDeco[14 * MW + i] = 2; }
    for (int y = 3; y <= 9; y += 3) { gDeco[y * MW + 0] = 1; gDeco[y * MW + 15] = 2; }
    auto add = [](Sprite sp, Color main, Color accent) { sp.main = main; sp.accent = accent; SP.push_back(sp); };
    add(mk("vol", "Mrs. Volkova", T_OLD, 0, .8f, 5.5f, 1.5f, true), Color{96, 92, 112, 255}, Color{150, 80, 70, 255});
    add(mk("pav", "Pavel (janitor)", T_MAN, 0, .9f, 14.5f, 9.5f, true), Color{78, 112, 140, 255}, Color{226, 170, 60, 255});
    Sprite igor = mk("igor", "Man in red scarf", T_MAN, 0, .95f, 14.5f, 1.5f, true);
    igor.speed = .8f; igor.wi = 1; igor.wp = {{14.5f, 1.5f}, {14.5f, 13.5f}, {1.5f, 13.5f}, {1.5f, 1.5f}};
    add(igor, Color{64, 68, 78, 255}, Color{196, 52, 40, 255});
    Sprite cat = mk("cat", "Boris the cat", T_CAT, 0, .36f, 6.5f, 13.5f, true);
    cat.speed = .5f; cat.wi = 1; cat.wp = {{4.5f, 13.5f}, {11.5f, 13.5f}};
    add(cat, Color{220, 140, 60, 255}, WHITE);
    add(mk("", "", T_PLANT, 0, .4f, 1.5f, 1.5f, false), WHITE, WHITE);
    add(mk("", "", T_PLANT, 0, .4f, 14.5f, 13.5f, false), WHITE, WHITE);
    add(mk("", "", T_PLANT, 0, .4f, 9.5f, 1.5f, false), WHITE, WHITE);
    add(mk("", "", T_BIN, 0, .3f, 3.5f, 13.5f, false), Color{130, 140, 150, 255}, WHITE);
    add(mk("", "", T_BIN, 0, .3f, 10.5f, 1.5f, false), Color{130, 140, 150, 255}, WHITE);
    add(mk("", "", T_CRATE, 0, .3f, 12.5f, 13.5f, false), Color{150, 112, 76, 255}, WHITE);
    add(mk("", "", T_CRATE, 0, .3f, 1.5f, 9.5f, false), Color{150, 112, 76, 255}, WHITE);

    // ceiling lamps along the hallway ring (each one is a light pool)
    for (float lx : {3.5f, 7.5f, 11.5f}) { LAMPS.push_back({lx, 1.5f}); LAMPS.push_back({lx, 13.5f}); }
    for (float ly : {4.5f, 9.5f}) { LAMPS.push_back({1.5f, ly}); LAMPS.push_back({14.5f, ly}); }
}

int main() {
    InitWindow(SCR_W, SCR_H, "Block 7 - ASCII Detective (raylib)");
    SetTargetFPS(60);
    Font font = loadMono(CH - 1);
    setupWorld();

    struct Drop { float x, y, v; };
    vector<Drop> drops;
    for (int i = 0; i < 200; i++)
        drops.push_back({(float)GetRandomValue(0, GW), (float)GetRandomValue(0, GH), 18.0f + GetRandomValue(0, 16)});

    say("Block 7. Rain on the stairwell glass. Someone died in 106.", COL_SYS);
    say("Watch AI: \"Case loaded. Don't expect me to enjoy it.\"", COL_WATCH);

    float px = 2.5f, py = 1.5f, ang = 0, flickT = 0;
    bool rain = true;

    while (!WindowShouldClose()) {
        float dt = std::min(0.05f, GetFrameTime());

        // ---- input ----
        float turn = (float)((IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) - (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)));
        float mv = (float)((IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) - (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)));
        ang += turn * 2.2f * dt;
        if (mv != 0) {
            float mdx = std::cos(ang) * mv, mdy = std::sin(ang) * mv, s = 2.6f * dt;
            float sgx = mdx > 0 ? 1.f : (mdx < 0 ? -1.f : 0.f), sgy = mdy > 0 ? 1.f : (mdy < 0 ? -1.f : 0.f);
            if (tileAt((int)(px + mdx * s + sgx * .22f), (int)py) == '.') px += mdx * s;
            if (tileAt((int)px, (int)(py + mdy * s + sgy * .22f)) == '.') py += mdy * s;
        }
        if (IsKeyPressed(KEY_T)) gStyle = (gStyle + 1) % 3;
        if (IsKeyPressed(KEY_R)) rain = !rain;
        if (IsKeyPressed(KEY_F)) gTorch = !gTorch;
        if (IsKeyPressed(KEY_C)) {
            string all;
            for (auto& c : gClueOrder) all += (all.empty() ? "" : " | ") + c;
            say("Case file: " + (all.empty() ? string("nothing yet") : all), COL_SYS);
        }

        // ---- update ----
        if (flickT > 0) { flickT -= dt; gFlick = 0.6f + GetRandomValue(0, 15) / 100.0f; }
        else { gFlick = 0.94f + GetRandomValue(0, 6) / 100.0f; if (GetRandomValue(0, 999) < 5) flickT = 0.1f + GetRandomValue(0, 15) / 100.0f; }
        for (auto& d : drops) {
            d.y += d.v * dt; d.x -= d.v * 0.3f * dt;
            if (d.y >= GH) { d.y = -1; d.x = (float)GetRandomValue(0, GW * 13 / 10); }
            if (d.x < 0) d.x += GW;
        }
        for (size_t li = 0; li < LAMPS.size(); li++) LF[li] = 1.0f;
        LF[3] = hash2((int)(GetTime() * 12), 3) > .8f ? .15f : .95f;   // buzzing lamp
        LF[6] = .2f;                                                    // dying lamp
        for (auto& s : SP) {
            if (s.wp.empty()) continue;
            if (s.pause > 0) { s.pause -= dt; continue; }
            Vector2 w = s.wp[s.wi];
            float ex = w.x - s.x, ey = w.y - s.y, d = std::hypot(ex, ey);
            if (d < .1f) s.wi = (s.wi + 1) % (int)s.wp.size();
            else { s.x += ex / d * s.speed * dt; s.y += ey / d * s.speed * dt; }
        }

        // ---- render world into the char grid ----
        char focusTile = '#'; float focusDist = 99;
        renderWorld(px, py, ang, (int)(GetTime() * 8), focusTile, focusDist);
        if (rain)
            for (auto& d : drops) {
                int xi = (int)d.x, yi = (int)d.y;
                if (xi >= 0 && xi < GW && yi >= 0 && yi < GH) put(yi * GW + xi, '/', P_RAIN, .7f);
            }

        // ---- interact ----
        int doorN = (focusTile >= '1' && focusTile <= '8' && focusDist < 1.9f) ? focusTile - '0' : 0;
        if (IsKeyPressed(KEY_E) && !gDone) {
            if (gFocusS) talk(*gFocusS);
            else if (doorN) knock(doorN);
        }

        // ---- draw ----
        BeginDrawing();
        ClearBackground(BLACK);
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++) {
                int i = y * GW + x;
                if (gCh[i] == ' ') continue;
                DrawTextCodepoint(font, toCodepoint(gCh[i]), Vector2{(float)(x * CW), (float)(y * CH)}, (float)(CH - 1), cellColor(i));
            }
        // minimap
        for (int y = 0; y < MH; y++)
            for (int x = 0; x < MW; x++) {
                char c = tileAt(x, y);
                Color col = c == '#' ? Color{43, 56, 67, 200} : c == '.' ? Color{62, 79, 70, 200} : Color{232, 176, 74, 220};
                if (c != '#' && c != '.' && gKnocked.count(c - '0')) col = Color{111, 163, 111, 220};
                DrawTextCodepoint(font, c == '#' ? '#' : c == '.' ? '.' : c, Vector2{(float)((2 + x) * CW), (float)((1 + y) * CH)}, (float)(CH - 1), col);
            }
        for (auto& s : SP)
            if (s.talk)
                DrawTextCodepoint(font, '*', Vector2{(2 + s.x) * CW - CW / 2.0f, (1 + s.y) * CH - CH / 2.0f}, (float)(CH - 1),
                                  s.key == "igor" ? Color{240, 80, 80, 255} : Color{232, 176, 74, 255});
        DrawTextCodepoint(font, '@', Vector2{(2 + px) * CW - CW / 2.0f, (1 + py) * CH - CH / 2.0f}, (float)(CH - 1), WHITE);

        // prompt
        string prompt;
        if (!gDone) {
            if (gFocusS) prompt = "[E] Talk to " + gFocusS->name;
            else if (doorN) prompt = string("[E] ") + (doorN == 6 ? "Inspect" : "Knock on") + " Apt " + DOORS[doorN].apt;
        }
        if (!prompt.empty()) {
            int tw = MeasureText(prompt.c_str(), 18);
            DrawRectangle(SCR_W / 2 - tw / 2 - 8, GH * CH - 50, tw + 16, 26, Color{0, 0, 0, 200});
            DrawText(prompt.c_str(), SCR_W / 2 - tw / 2, GH * CH - 46, 18, COL_ME);
        }

        // log panel
        int top = GH * CH;
        DrawLine(0, top, SCR_W, top, Color{28, 36, 43, 255});
        string stat = "Clues " + std::to_string(gClues.size()) + "/5  Style " + STYLE_NAMES[gStyle] + "  Torch " +
                      (gTorch ? "on" : "off") + "   [WASD move] [E act] [T style] [R rain] [F torch] [C case file]";
        DrawText(stat.c_str(), 8, top + 4, 14, COL_SYS);
        int lines = 5, first = std::max(0, (int)gLog.size() - lines);
        for (int i = first; i < (int)gLog.size(); i++)
            DrawText(gLog[i].first.c_str(), 8, top + 26 + (i - first) * 17, 14, gLog[i].second);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
