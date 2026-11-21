// Level.cpp - turns an ASCII floor plan into a grid of cells, lays staircases into it, and answers world queries.
//
// HOW A FLOOR PLAN BECOMES A WORLD
//   Every character of a LevelDef row is one TILE; every tile becomes 2x2 CELLS (so a stair step can be half a
//   tile deep). Each cell has its own floor height F and ceiling height C - that is all the renderer needs.
//   Tile legend:
//     #  wall            W  window wall      .  walkable floor      k  stair handrail wall
//     m  metal wall (lift cabin)             E  lift doors          L  lift cabin (walkable)
//     1-8 apartment door (101..108 on the ground floor)             u / d  staircase tile (just for reading the plan:
//                                                                  the real steps come from the FlightDefs)
//
// HOW STAIRS STACK
//   All floors share the same footprint. A staircase between two storeys is described ONCE (a FlightDef) and is laid
//   into both floors' grids at the same tiles: on the lower floor it climbs from F=0 up to F=2, on the upper floor
//   the same steps run from F=-2 up to F=0. Walking onto the last steps switches to the other floor (a short fade).
#include "Level.h"

const Cell& Level::cellAt(int cx, int cy) const {
    if (cx < 0 || cy < 0 || cx >= cellsW || cy >= cellsH) return solid;   // outside the map is solid
    return grid[(size_t)cy * cellsW + cx];
}

void Level::setTileDoor(int tx, int ty, int kind) {
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 2; i++) {
            Cell& c = cellRef(tx * 2 + i, ty * 2 + j);
            c.type = C_DOOR; c.door = (uint8_t)kind; c.stair = false; c.F = 0; c.C = WALL_H;
        }
}
void Level::setTileWall(int tx, int ty) {
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 2; i++) { Cell& c = cellRef(tx * 2 + i, ty * 2 + j); c.type = C_WALL; c.stair = false; }
}

void Level::build(const LevelDef& def, int floorIndex, const vector<FlightDef>& flights, int floorCount) {
    index = floorIndex; name = def.name;
    tilesY = (int)def.rows.size(); tilesX = (int)def.rows[0].size();
    cellsW = tilesX * 2; cellsH = tilesY * 2;
    grid.assign((size_t)cellsW * cellsH, Cell());
    elevTx = elevTy = carTx = carTy = -1;
    for (int ty = 0; ty < tilesY; ty++)
        for (int tx = 0; tx < tilesX; tx++) {
            char ch = def.rows[(size_t)ty][(size_t)tx];
            if (ch == 'E') { elevTx = tx; elevTy = ty; }
            if (ch == 'L') { carTx = tx; carTy = ty; }
            for (int j = 0; j < 2; j++)
                for (int i = 0; i < 2; i++) {
                    int cx = tx * 2 + i, cy = ty * 2 + j;
                    Cell c;
                    c.sx = (uint8_t)(cx & 1); c.sy = (uint8_t)(cy & 1);
                    switch (ch) {
                        case '.': case 'u': case 'd': case 'L': c.type = C_OPEN; break;
                        case 'W': c.type = C_WINDOW; break;
                        case 'k': c.type = C_RAIL; break;
                        case 'm': c.type = C_WALL; c.mat = MAT_METAL; break;
                        case 'E': c.type = C_DOOR; c.door = DOOR_ELEVATOR; break;
                        case '#': c.type = C_WALL; break;
                        default:
                            if (ch >= '1' && ch <= '8') { c.type = C_DOOR; c.door = (uint8_t)(ch - '0'); }
                            break;
                    }
                    cellRef(cx, cy) = c;
                }
        }

    // ---- staircases (see the header comment: one FlightDef is laid into both floors it connects)
    for (const FlightDef& f : flights) {
        bool up = f.lowFloor == floorIndex, down = f.lowFloor + 1 == floorIndex;
        if (!up && !down) continue;
        int dx = (f.dir == DIR_E) - (f.dir == DIR_W), dy = (f.dir == DIR_S) - (f.dir == DIR_N);
        bool alongX = dx != 0;
        int along = dx + dy;                                         // +1 if the climb runs toward +x/+y, else -1
        int steps = STAIR_TILES * 2;
        float off = down ? -WALL_H : 0.0f;                           // seen from the upper floor everything is one storey lower
        bool linked = f.lowFloor >= 0 && f.lowFloor + 1 < floorCount;
        int startEdge = alongX ? f.tx * 2 : f.ty * 2;
        float b = along > 0 ? -(float)startEdge : (float)startEdge + 2.0f;   // measure s = distance climbed = along*coord + b
        for (int k = 0; k < steps; k++)
            for (int i = 0; i < 2; i++) {
                int cx, cy;
                switch (f.dir) {
                    case DIR_E: cx = f.tx * 2 + k;     cy = f.ty * 2 + i; break;
                    case DIR_W: cx = f.tx * 2 + 1 - k; cy = f.ty * 2 + i; break;
                    case DIR_S: cx = f.tx * 2 + i;     cy = f.ty * 2 + k; break;
                    default:    cx = f.tx * 2 + i;     cy = f.ty * 2 + 1 - k; break;
                }
                if (cx < 0 || cy < 0 || cx >= cellsW || cy >= cellsH) continue;
                Cell& c = cellRef(cx, cy);
                c.type = C_OPEN; c.stair = true; c.stairAxis = alongX ? 0 : 1; c.stairSlope = (int8_t)along;
                c.F = STEP_H * (float)(k + 1) + off;
                c.C = up ? c.F + WALL_H : WALL_H;                    // climbing: the ceiling follows the slope; below the hall: flat
                c.rf0 = STEP_H * (b + 1.0f) + off;
                c.rfx = alongX ? STEP_H * (float)along : 0.0f;
                c.rfy = alongX ? 0.0f : STEP_H * (float)along;
                c.link = 0;
                if (linked && up && k >= steps - 2) c.link = 1;      // the last steps lead up
                if (linked && down && k <= 1) c.link = -1;           // the first steps lead down
            }
        // the end of the flight that is not a landing: a plain wall, or a locked door to the basement / roof
        int ex = up ? f.tx + dx * STAIR_TILES : f.tx - dx, ey = up ? f.ty + dy * STAIR_TILES : f.ty - dy;
        if (ex >= 0 && ey >= 0 && ex < tilesX && ey < tilesY) {
            if (up && f.lowFloor + 1 >= floorCount) setTileDoor(ex, ey, DOOR_ROOF);
            else if (down && f.lowFloor < 0) setTileDoor(ex, ey, DOOR_BOILER);
            else setTileWall(ex, ey);
        }
    }

    // ---- lights: ceiling bulbs from the design, then one cold light inside every window (dark except in lightning)
    lamps.clear();
    for (const Bulb& bl : def.bulbs) { Lamp l; l.x = bl.x * 2; l.y = bl.y * 2; l.mode = bl.mode; lamps.push_back(l); }
    for (int cy = 0; cy < cellsH; cy++)
        for (int cx = 0; cx < cellsW; cx++) {
            const Cell& w = cellAt(cx, cy);
            if (w.type != C_WINDOW || w.sx || w.sy) continue;          // one lamp per window tile
            float ox = 0, oy = 0;
            if (cellAt(cx, cy - 1).type == C_OPEN) oy = -1; else if (cellAt(cx, cy + 2).type == C_OPEN) oy = 1;
            else if (cellAt(cx - 1, cy).type == C_OPEN) ox = -1; else if (cellAt(cx + 2, cy).type == C_OPEN) ox = 1;
            Lamp l; l.x = cx + 1.0f + ox * 1.6f; l.y = cy + 1.0f + oy * 1.6f; l.k = 0; l.r = 5.0f; l.win = true;
            lamps.push_back(l);
        }
    for (float& a : doorAmt) a = 0;
    for (float& a : doorTgt) a = 0;
    if (hasElevator()) setElevatorOpen(false);
}

void Level::updateLamps(float time, float flash) {
    int n = 0;
    for (Lamp& l : lamps) {
        n++;
        if (l.win) l.k = flash * 1.1f;
        else if (l.mode == 1) l.k = hs((int)(time * 12), n) > 0.8f ? 0.15f : 0.95f;     // buzzing
        else if (l.mode == 2) l.k = 0.28f;                                              // dying
        else l.k = 1.0f;
    }
}

bool Level::passable(float x, float y, float curF) const {
    const float r = 0.4f;
    for (int j = -1; j <= 1; j += 2)
        for (int i = -1; i <= 1; i += 2) {
            const Cell& c = cellAt((int)std::floor(x + i * r), (int)std::floor(y + j * r));
            if (c.type != C_OPEN || c.F > curF + 0.25f || c.C - c.F < 1.6f) return false;   // walls, tall steps, low ceilings
        }
    return true;
}

DoorInfo Level::doorInfoAt(int cx, int cy) const {
    DoorInfo d; const Cell& c = cellAt(cx, cy);
    d.id = c.door; d.floor = index;
    int ox = cx & ~1, oy = cy & ~1;
    d.tcx = ox + 1.0f; d.tcy = oy + 1.0f;
    if (cellAt(ox, oy - 1).type == C_OPEN) { d.nx = 0; d.ny = -1; }
    else if (cellAt(ox, oy + 2).type == C_OPEN) { d.nx = 0; d.ny = 1; }
    else if (cellAt(ox - 1, oy).type == C_OPEN) { d.nx = -1; d.ny = 0; }
    else { d.nx = 1; d.ny = 0; }
    return d;
}

string Level::doorLabel(int doorId) const { return std::to_string(index + 1) + "0" + std::to_string(doorId); }

bool Level::inCar(float x, float y) const {
    return carTx >= 0 && x >= carTx * 2 && x < carTx * 2 + 2 && y >= carTy * 2 && y < carTy * 2 + 2;
}

void Level::setElevatorOpen(bool open) {
    if (!hasElevator()) return;
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 2; i++) {
            Cell& c = cellRef(elevTx * 2 + i, elevTy * 2 + j);
            c.type = open ? C_OPEN : C_DOOR; c.door = DOOR_ELEVATOR; c.F = 0; c.C = WALL_H;
        }
}

void Building::build(const BuildingDef& def) {
    floors.assign(def.floors.size(), Level());
    for (size_t i = 0; i < def.floors.size(); i++) floors[i].build(def.floors[i], (int)i, def.flights, (int)def.floors.size());
    cur = std::clamp(def.startFloor, 0, count() - 1);
}
