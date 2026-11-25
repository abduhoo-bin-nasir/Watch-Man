// Entities.h - everything that stands somewhere in the building.
//
//   Entity            position, heading, floor
//    |-- Prop         a thing (a crate): drawn, never talks
//    |-- Character    a person: name, look (pixel design + colours), a dialogue Script, a journal "met" state
//    |     |-- Wanderer   walks a patrol route round the hallways (or just stands in the hall)
//    |     `-- Resident   lives behind a numbered door and appears in the doorway when you knock
//    `-- Player       you: walk, turn, look, collide with the level
//
// HOW CHARACTERS ARE RENDERED (details in CharacterArt.cpp)
//   A character is a flat "billboard" made of two crossed vertical planes: one shows the FRONT or BACK design (whichever
//   side you are on), the other shows the SIDE design. The designs are small ASCII-art grids in CharacterArt.cpp; the
//   look of a character = which design (ArtId) + two colours (main, accent) + a height. Change any of those in the
//   cast list (CharacterArt.cpp) and the person changes everywhere: in the world, in the dialogue and in the journal.
#pragma once
#include "Common.h"
#include "Dialogue.h"
#include "Level.h"

// ---------------------------------------------------------------- pixel art (CharacterArt.cpp)
struct SpriteArt { int w = 0, h = 0; vector<char> c; vector<int8_t> s; };   // colour key + shade (-1 dark, 0, +1 light) per pixel
struct SpriteSet { SpriteArt front, back, side; };
const SpriteSet& spriteSet(int art);                                         // the three views of a design
constexpr int SPRITE_VIEWS = 16;                                            // directions each design is pre-rendered from
const SpriteArt& spriteFrame(int art, int k);                               // the 3D model of a design seen from direction k (0 = from the front, turning one way)
bool spriteColor(char key, RGB main, RGB acc, RGB& out);                    // colour key -> colour (main/accent come from the character)

// ---------------------------------------------------------------- the cast (CharacterArt.cpp)
struct CastDef { string id, name, role, verb; int art; RGB main, acc; float h; };   // h = height as a fraction of the wall height
const vector<CastDef>& castList();

// ---------------------------------------------------------------- classes
class Entity {
public:
    virtual ~Entity() = default;
    virtual void update(float /*dt*/) {}
    float x = 0, y = 0;     // position in cells
    float yaw = 0;          // heading in radians
    int floor = 0;
};

class Prop : public Entity {
public:
    int art = ART_CRATE; float h = 0.3f; RGB main{200, 200, 200};
};

class Character : public Entity {
public:
    string id, name, role, verb = "Talk to";
    int art = ART_MAN; RGB main{200, 200, 200}, acc{200, 60, 60}; float h = 0.9f;
    Script script;                 // what they say (DialogueData.cpp); `done` flags are per playthrough
    vector<string> notes;          // lines scribbled about them on the People page
    bool met = false;              // have you spoken to them?
    bool talking = false;          // in conversation: stands still and faces you
    bool hidden = false;           // taken away (after the arrest)
    float lit = 0;                 // extra warm light on them (room light behind a doorway)
    float floorOverride = -1;      // stand on this floor height instead of the cell's (-1 = none)
    virtual bool isResident() const { return false; }
};

class Wanderer : public Character {
public:
    void setRoute(const vector<V2>& tilePoints, float tilesPerSec);   // route in tile units
    void update(float dt) override;
private:
    vector<V2> route; size_t next = 0; float speed = 0;
};

class Resident : public Character {
public:
    bool isResident() const override { return true; }
    int door = 0;                  // apartment door number (1..8) on `floor`
    bool inDoorway = false;
    void appearAt(const DoorInfo& d, const Level& level);   // step into the doorway, facing out into the hallway
    void leave() { inDoorway = false; talking = false; }
};

class Player : public Entity {
public:
    float feet = 0, pitch = 0, bob = 0, phase = 0;   // feet = height above the floor level of the current Level
    bool moving = false;
    void steer(const Input& in, float dt);                           // turn + look (keys and mouse)
    void walk(const Level& level, float fw, float st, float dt);     // move with wall sliding
    void easeTo(const Level& level, float tx, float ty, float dt);   // glide to a spot, never through a wall
    void lookAt(float tx, float ty, float dt, float pitchTarget = 0);
    void settle(const Level& level, float dt);                       // feet follow the floor, head bob
};

// ---------------------------------------------------------------- creating the world's inhabitants from a BuildingDef
vector<std::unique_ptr<Character>> buildCast(const BuildingDef& def);
vector<std::unique_ptr<Prop>> buildProps(const BuildingDef& def);
