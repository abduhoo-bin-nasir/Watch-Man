// Renderer.h - draws the world into a 320x180 pixel buffer. No raylib calls: it only fills memory, which makes it
// easy to test; Game uploads the buffer to the screen.
//
// TECHNIQUE: a column renderer in the style of Doom/Build. Every grid cell has its own floor and ceiling height. For
// each screen column a ray walks the grid cell by cell (DDA) and keeps a "window" of rows that are still undrawn. Per
// cell it draws the ceiling span, the floor span, then the face between this cell and the next one (a wall, a stair
// riser or a ceiling step). Stairs are therefore real steps - there is no special case for them in the renderer.
// Characters are billboards (see Entities.h) drawn afterwards, depth-tested against the wall distance of each column.
#pragma once
#include "Common.h"
#include "Level.h"

class Character;

struct CamState { float x = 0, y = 0, feet = 0, yaw = 0, pitch = 0, bob = 0; };

// One thing to draw this frame (a character or a prop), already resolved to world numbers.
struct Billboard {
    float x = 0, y = 0, yaw = 0, h = 0;      // cells, radians, height as a fraction of WALL_H
    float floorH = 0;                        // height of the floor it stands on
    float lit = 0;                           // extra warm light
    int art = 0; RGB main{200, 200, 200}, acc{200, 60, 60};
    const Character* who = nullptr;          // set for people you can talk to
};

struct Focus { bool valid = false; CellType type = C_WALL; int door = 0, cx = 0, cy = 0; float t = 99; };   // what the screen centre points at

// Lightning and flicker state, shared by the sky seen through windows and the lamps.
class Weather {
public:
    void update(float dt, float time, float playerYaw);
    float flash = 0;                                         // lightning brightness 0..1
    float boltT = -1.0f, boltAz = 0, boltSeed = 0;           // running bolt: time since start (-1 = none), where in the sky, its shape
private:
    float nextBolt = 4.0f; uint32_t bolts = 0;
};

struct Tex { int w = 0, h = 0; vector<uint8_t> d; };

class Renderer {
public:
    Renderer();
    void buildTextures(int floorCount);
    void render(const Level& level, const CamState& cam, const vector<Billboard>& bills, const Weather& weather, float time);
    const uint32_t* pixels() const { return pix; }           // RW*RH, RGBA8 (R lowest byte)

    Focus focus;                                             // set by render(): what the centre column hit
    int nearBill = -1;                                       // set by render(): index of the talkable billboard in front of you, or -1

private:
    struct Cam { float px, py, eye, hz, dx, dy, plx, ply; };
    void putPix(int x, int y, float r, float g, float b, float L, float f);
    void putFlat(int x, int y, float r, float g, float b);
    void addLight(int x, int y, float r, float g, float b);
    float lampL(float wx, float wy);
    void drawCeilRow(const Cam& c, int x, int y, const Cell& cell, float rx, float ry);
    void drawFloorRow(const Cam& c, int x, int y, const Cell& cell, float rx, float ry);
    RGB outsideView(float rx, float ry, float hx, float hy, int nside, float h, float sl, float az);
    RGB roomView(float u, float v, int n);
    RGB cabinView(float u, float v);
    void drawWallRows(const Cam& c, int x, int y0, int y1, float tFar, float hx, float hy, float u, int nside, bool flip, const Cell& cur, const Cell& wall);
    void drawRiserRows(int x, int y0, int y1, int topRow, int botRow, float tFar, float hx, float hy, float u);
    void renderColumn(int x, const Cam& c);
    void drawGlows(const Cam& c);
    void drawBillboards(const Cam& c, const vector<Billboard>& bills);

    const Level* lvl = nullptr;
    const Weather* wx = nullptr;
    float time_ = 0;
    uint32_t pix[RW * RH];
    float vig[RW * RH];
    float zbuf[RW];
    uint32_t frame = 0;
    float cold = 0, bulb = 0;
    Tex tWall, tRail, tFloor, tCeil, tStair, tRiser, tMetal, tLift, tMetalDoor;
    vector<std::array<Tex, DOOR_KINDS>> doorTex;             // [floor][door id]
};
