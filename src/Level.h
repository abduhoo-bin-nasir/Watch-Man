// Level.h - the building: every floor is a Level (a grid of cells with floor/ceiling heights), and the Building
// stacks them. This file declares the world model AND the plain-data structures that describe a level design.
// The designs themselves (ASCII floor plans, lights, who stands where) live in LevelData.cpp.
#pragma once
#include "Common.h"

enum CellType : uint8_t { C_OPEN, C_WALL, C_DOOR, C_WINDOW, C_RAIL, C_PARAPET };
enum WallMaterial : uint8_t { MAT_PLASTER, MAT_METAL };
// door ids: 1..8 are apartment doors; the rest are special doors
enum DoorKind { DOOR_APT_MAX = 8, DOOR_ELEVATOR = 9, DOOR_BOILER = 10, DOOR_ROOF = 11, DOOR_KINDS = 12 };
enum Dir { DIR_N, DIR_E, DIR_S, DIR_W };

struct Cell {
    CellType type = C_WALL;
    WallMaterial mat = MAT_PLASTER;
    bool stair = false;
    uint8_t stairAxis = 0;      // 0 = steps run along x, 1 = along y
    int8_t stairSlope = 0;      // +1/-1: the floor rises toward +coordinate / -coordinate (the low edge carries the nosing)
    int8_t link = 0;            // +1: stepping here leads up to the next floor, -1: down to the previous one
    uint8_t door = 0;           // DoorKind for C_DOOR cells
    uint8_t sx = 0, sy = 0;     // which half of its tile this cell is (texture mapping)
    float F = 0, C = WALL_H;    // floor and ceiling height
    float rf0 = 0, rfx = 0, rfy = 0;   // smooth slope through the step edges: refF = rf0 + rfx*x + rfy*y (stair cells)
};

struct Lamp { float x = 0, y = 0, k = 1; float r = LAMP_R; bool win = false; int mode = 0; };  // cells; k = brightness

struct DoorInfo { int floor = 0, id = 0; float tcx = 0, tcy = 0, nx = 0, ny = 0; };   // door tile centre (cells) + outward normal

// ======================================================================== level design data
struct Bulb { float x, y; int mode = 0; };                  // tile units. mode: 0 steady, 1 buzzing, 2 dying, 3 lightning lamp (lights up when it strikes; invisible), 4 weak steady glow (invisible)
struct LevelDef { string name; vector<string> rows; vector<Bulb> bulbs; bool outdoor = false, flat = false; };   // outdoor: ',' tiles are open sky above; flat: a small apartment interior (wooden floor)
// A flight is one physical staircase between two storeys. It rises in direction `dir` from the first step tile
// (tx,ty) over STAIR_TILES tiles. lowFloor -1 = from the basement, floor count-1 = up to the roof.
struct FlightDef { int tx, ty; Dir dir; int lowFloor; };
struct Placement { string id; int floor; V2 at; vector<V2> patrol; float speed = 0; float facing = 0; };   // tile units
struct HomeDef { string id; int floor; int door; };         // who answers which apartment door
struct PropDef { int art; int floor; V2 at; float h; RGB main; float yaw; };
struct FlatDef { int floor, door; LevelDef plan; vector<PropDef> props; };   // what you see through the peephole of one apartment door (tile units; the door is the west wall)
struct BuildingDef {
    vector<LevelDef> floors;
    vector<FlightDef> flights;
    vector<FlatDef> flats;
    int startFloor = 0; V2 start{0, 0}; float startYaw = 0;   // where you begin (tile units)
    vector<Placement> characters;
    vector<HomeDef> homes;
    vector<PropDef> props;
};
BuildingDef designBuilding();                                // defined in LevelData.cpp

// ======================================================================== runtime
class Level {
public:
    void build(const LevelDef& def, int floorIndex, const vector<FlightDef>& flights, int floorCount);

    const Cell& cellAt(int cx, int cy) const;
    bool passable(float x, float y, float curF) const;      // can a body of radius 0.4 stand here?
    DoorInfo doorInfoAt(int cx, int cy) const;
    string doorLabel(int doorId) const;                      // "101" for apartment 1 on the ground floor
    void updateLamps(float time, float flash);               // flicker, dying bulbs, lightning through the windows

    bool hasElevator() const { return elevTx >= 0; }
    bool inCar(float x, float y) const;                      // standing inside the lift cabin
    void setElevatorOpen(bool open);                         // the doorway becomes walkable when fully open
    V2 elevatorDoorCentre() const { return {elevTx * 2 + 1.0f, elevTy * 2 + 1.0f}; }

    int index = 0;
    string name;
    bool flat = false;          // an apartment interior seen through a peephole
    bool outdoor = false;       // a rooftop: ',' cells have no ceiling, the sky is drawn instead, rain falls
    int tilesX = 0, tilesY = 0, cellsW = 0, cellsH = 0;
    vector<Lamp> lamps;                                      // ceiling bulbs first, then cold window lights
    float doorAmt[DOOR_KINDS] = {};                          // how far each door is open (0..1)
    float doorTgt[DOOR_KINDS] = {};                          // where it is heading

private:
    Cell& cellRef(int cx, int cy) { return grid[(size_t)cy * cellsW + cx]; }
    void setTileDoor(int tx, int ty, int kind);
    void setTileWall(int tx, int ty);
    vector<Cell> grid;
    Cell solid;
    int elevTx = -1, elevTy = -1, carTx = -1, carTy = -1;
};

class Building {
public:
    void build(const BuildingDef& def);
    Level& level(int i) { return floors[(size_t)i]; }
    const Level& level(int i) const { return floors[(size_t)i]; }
    Level& current() { return floors[(size_t)cur]; }
    const Level& current() const { return floors[(size_t)cur]; }
    int count() const { return (int)floors.size(); }
    int currentIndex() const { return cur; }
    void setCurrent(int i) { cur = std::clamp(i, 0, count() - 1); }
private:
    vector<Level> floors;
    int cur = 0;
};
