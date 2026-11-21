// Entities.cpp - behaviour of the things that stand in the building.
#include "Entities.h"

void Wanderer::setRoute(const vector<V2>& tilePoints, float tilesPerSec) {
    route.clear();
    for (V2 p : tilePoints) route.push_back({p.x * 2, p.y * 2});
    speed = tilesPerSec * 2;
    next = route.size() > 1 ? 1 : 0;
    if (route.size() > 1) yaw = std::atan2(route[1].y - route[0].y, route[1].x - route[0].x);
}

void Wanderer::update(float dt) {
    if (route.empty() || talking || hidden) return;       // not while someone is talking to you
    V2 w = route[next];
    float ex = w.x - x, ey = w.y - y, d = std::hypot(ex, ey);
    if (d < 0.2f) next = (next + 1) % route.size();
    else { x += ex / d * speed * dt; y += ey / d * speed * dt; yaw = std::atan2(ey, ex); }
}

void Resident::appearAt(const DoorInfo& d, const Level& level) {
    x = d.tcx + d.nx * 0.55f; y = d.tcy + d.ny * 0.55f;                 // just inside the door frame
    yaw = std::atan2(d.ny, d.nx);                                        // facing out into the hallway
    floorOverride = level.cellAt((int)std::floor(d.tcx + d.nx * 2.5f), (int)std::floor(d.tcy + d.ny * 2.5f)).F;
    lit = 0.55f; inDoorway = true; talking = true;
}

// ------------------------------------------------------------------ player
void Player::steer(const Input& in, float dt) {
    yaw += in.turn * 2.2f * dt + in.mouseX * 0.0025f;
    pitch -= in.mouseY * 0.35f;                      // mouse up (negative delta) = look up
    pitch += in.pitchKey * 70.0f * dt;
    if (in.recenter) pitch = 0;
    pitch = std::clamp(pitch, -PITCH_MAX, PITCH_MAX);
}

void Player::walk(const Level& level, float fw, float st, float dt) {
    float dx = std::cos(yaw), dy = std::sin(yaw);
    float mvx = dx * fw - dy * st, mvy = dy * fw + dx * st, sp = 4.8f * dt;
    moving = (fw != 0 || st != 0);
    if (!moving) return;
    phase += dt * 9;
    float curF = level.cellAt((int)std::floor(x), (int)std::floor(y)).F;
    float nx = x + mvx * sp, ny = y + mvy * sp;
    if (level.passable(nx, y, curF)) x = nx;         // slide along walls: test each axis separately
    if (level.passable(x, ny, curF)) y = ny;
}

void Player::easeTo(const Level& level, float tx, float ty, float dt) {
    float k = 1.0f - std::exp(-dt * 5.0f), curF = level.cellAt((int)std::floor(x), (int)std::floor(y)).F;
    float nx = x + (tx - x) * k, ny = y + (ty - y) * k;
    if (level.passable(nx, y, curF)) x = nx;
    if (level.passable(x, ny, curF)) y = ny;
}

void Player::lookAt(float tx, float ty, float dt, float pitchTarget) {
    float want = std::atan2(ty - y, tx - x), d = want - yaw;
    d -= 6.2831853f * std::floor(d / 6.2831853f + 0.5f);           // shortest way round
    yaw += d * (1.0f - std::exp(-dt * 7.0f));
    pitch += (pitchTarget - pitch) * (1.0f - std::exp(-dt * 6.0f));
}

void Player::settle(const Level& level, float dt) {
    const Cell& here = level.cellAt((int)std::floor(x), (int)std::floor(y));
    feet += (here.F - feet) * (1.0f - std::exp(-dt * 16.0f));      // smooth camera over the steps
    bob = moving ? std::sin(phase) * (here.stair ? 2.2f : 1.1f) : 0.0f;
}

// ------------------------------------------------------------------ creating the cast
vector<std::unique_ptr<Character>> buildCast(const BuildingDef& def) {
    vector<std::unique_ptr<Character>> out;
    auto fill = [](Character& c, const CastDef& d) {
        c.id = d.id; c.name = d.name; c.role = d.role; c.verb = d.verb; c.art = d.art; c.main = d.main; c.acc = d.acc; c.h = d.h;
        c.script = scriptFor(d.id);
    };
    for (const CastDef& d : castList()) {
        for (const Placement& p : def.characters)
            if (p.id == d.id) {
                auto w = std::make_unique<Wanderer>();
                fill(*w, d);
                w->floor = p.floor; w->x = p.at.x * 2; w->y = p.at.y * 2; w->yaw = p.facing;
                if (!p.patrol.empty()) w->setRoute(p.patrol, p.speed);
                out.push_back(std::move(w));
            }
        for (const HomeDef& h : def.homes)
            if (h.id == d.id) {
                auto r = std::make_unique<Resident>();
                fill(*r, d);
                r->floor = h.floor; r->door = h.door;
                out.push_back(std::move(r));
            }
    }
    return out;
}

vector<std::unique_ptr<Prop>> buildProps(const BuildingDef& def) {
    vector<std::unique_ptr<Prop>> out;
    for (const PropDef& d : def.props) {
        auto p = std::make_unique<Prop>();
        p->art = d.art; p->floor = d.floor; p->x = d.at.x * 2; p->y = d.at.y * 2; p->h = d.h; p->main = d.main; p->yaw = d.yaw;
        out.push_back(std::move(p));
    }
    return out;
}
