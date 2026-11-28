// Audio.h - the mixer and the Audio manager: what you hear depends on where you stand.
//
//  Mixer   a small software mixer. Voices play a buffer (looping or once) with a left/right gain, a low-pass filter (muffling) and a
//          playback rate. It runs on the audio thread; the game thread only writes target values into atomics.
//  Audio   once per frame it is given a snapshot of the world (AudioScene) and works out: how loud the rain is (louder near windows),
//          how far away each sound is, whether walls muffle it, which side it comes from, and which events just happened (a door
//          opened, a bolt of lightning, a step) - and starts the one-shot sounds for them. Things that happen LATER (thunder after the
//          lightning, a meow in twenty seconds) go through the event scheduler (Scheduler.h, a hand-written min-heap).
// It never touches raylib except in openDevice()/closeDevice(), so all of the logic is testable without a sound card.
#pragma once
#include "Level.h"
#include "Scheduler.h"
#include "Synth.h"
#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

enum AudioMode { AM_WALK, AM_KNOCK, AM_TALK, AM_JOURNAL, AM_END, AM_FADE, AM_RIDE, AM_PEEP };   // same order as Game's Mode

// ---------------------------------------------------------------------------------------------------------------- mixer
class Mixer {
public:
    static constexpr int MAX_VOICES = 64;
    // Starts a voice. gL/gR: gain, cut: low-pass coefficient (1 = open, small = muffled), smooth: how fast gain changes follow targets
    // (per sample; 0.002 quick, 0.00004 slow). Returns the voice number, or -1 if all voices are busy.
    int start(const Buf& b, bool loop, float gL, float gR, float cut, float rate, float smooth);
    void set(int id, float gL, float gR, float cut, float rate = -1.0f);
    void stop(int id);                                  // fades out, then frees
    void render(float* out, int frames);                // interleaved stereo; called by the audio thread
    int activeVoices() const;
    void setMaster(float g) { master.store(g); }
private:
    struct Voice {
        std::atomic<int> state{0};                      // 0 free, 1 playing
        std::atomic<float> gL{0}, gR{0}, cut{1}, rate{1};   // targets (game thread writes)
        std::atomic<bool> stopReq{false};
        Buf keep; const float* data = nullptr; size_t n = 0; bool loop = false; float smooth = 0.002f;   // set while state == 0
        double pos = 0; float cL = 0, cR = 0, cA = 1, lp = 0;                                            // audio thread only
    };
    Voice v[MAX_VOICES];
    std::atomic<float> master{0.9f};
    float masterNow = 0.9f;
};

// ---------------------------------------------------------------------------------------------------------------- scene
struct AudioMover { const void* key; int floor; float x, y; bool isCat; };   // a person or the cat who walks around

struct AudioScene {
    float dt = 0.016f; double time = 0;
    int mode = AM_WALK;
    int floor = 0; float px = 0, py = 0, yaw = 0;            // the listener
    const Building* building = nullptr;
    int tab = 0, spread = 0;                                  // journal page
    float boltT = -1, boltAz = 0;                             // lightning: seconds since it started (-1 = none), direction
    float toastT = 0;                                         // "journal updated" banner timer
    int peepFloor = -1, peepDoor = -1;                        // the flat being looked into (mode AM_PEEP)
    std::vector<AudioMover> movers;
};

// ---------------------------------------------------------------------------------------------------------------- manager
struct AmbientDef { int floor, door; const char* sound; float gain, radius; };   // a sound that lives behind an apartment door (AudioData.cpp)
const std::vector<AmbientDef>& ambientDefs();

class Audio {
public:
    Audio();
    ~Audio();
    void buildAsync();                                        // generate all sounds on a background thread (about 2 s)
    void buildNow();                                          // the same, waiting for it (tests)
    bool ready() const { return built.load(std::memory_order_acquire); }
    bool openDevice();                                        // the real sound card; false: the game stays silent
    void closeDevice();
    void update(const AudioScene& s);                         // once per frame
    void toggleMute();
    bool muted() const { return mute; }
    Mixer mixer;

    struct Debug {                                            // what the last update decided; tests read these
        float rainWindow = 0, rainOpen = 0, wind = 0, musicHall = 0, musicRoof = 0, buzz = 0, purr = 0, lift = 0;
        std::map<std::string, float> ambient;                 // loudness of each behind-the-door sound
        int footsteps = 0, npcSteps = 0, creaks = 0, knocks = 0, thunders = 0, meows = 0, slams = 0, pipes = 0, pages = 0, pings = 0, dings = 0, slides = 0, lenses = 0;
        double lastThunderDelay = 0; float lastFootCut = 1, lastFootGain = 0;
    } dbg;

private:
    struct Spatial { float gL, gR, alpha, dist, occ; };
    Spatial spatial(const AudioScene& s, float sx, float sy, int sfloor, float base, float radius, float cutFactor = 1.0f, float gainFactor = 1.0f) const;
    float occlusion(const Building& b, int floor, float x0, float y0, float x1, float y1) const;
    void shotAt(const AudioScene& s, const Buf& b, float x, float y, int floor, float base, float radius, float rate, float cutFactor = 1.0f);
    void shotFlat(const AudioScene& s, const Buf& b, float gain, float pan, float cut, float rate);
    void footstep(const AudioScene& s, const Level& lvl, float x, float y, int floor, bool isListener, bool cat);
    void startVoices();
    void loadFiles();                                         // sound files in assets/audio replace generated sounds (AudioFiles.h)
    void learnWorld(const Building& b);                       // window and door positions, once
    int pick(int n) { return (int)(rng() % (uint32_t)n); }
    float randf(float a, float b) { return a + (b - a) * (float)(rng() & 0xFFFF) / 65535.0f; }
    uint32_t rng() { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return seed; }

    Sounds sounds;
    std::atomic<bool> built{false};
    std::thread builder;
    bool useFiles = true;
    bool voicesStarted = false, worldLearned = false, mute = false, firstFrame = true;
    uint32_t seed = 2463534242u;
    struct Voices { int rainWin = -1, rainOpen = -1, wind = -1, musicHall = -1, musicRoof = -1, buzz = -1, purr = -1, lift = -1; } vo;
    struct Amb { int voice; Buf buf; float x, y; int floor, door; float gain, radius; std::string name; };
    std::vector<Amb> ambients;
    std::vector<std::vector<std::pair<float, float>>> windows;      // per floor: window cell centres
    std::vector<std::vector<std::pair<float, float>>> doorPos;      // per floor, per door id: centre of the door (x, y) (or -1,-1)
    EventScheduler sched;
    bool scheduled = false;
    // previous-frame memory, to notice things that just happened
    int prevMode = AM_WALK, prevTab = 0, prevSpread = 0, prevFloor = -1;
    float prevBoltT = -1, prevToast = 0, prevPx = 0, prevPy = 0, stepAcc = 0; bool stepLeft = false;
    std::vector<std::vector<float>> prevTgt;
    struct Walker { float x, y, acc; int floor; bool seen; };
    std::map<const void*, Walker> walkers;
    struct Device; std::unique_ptr<Device> dev;
};
