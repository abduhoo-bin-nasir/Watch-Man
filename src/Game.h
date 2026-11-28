// Game.h - the game itself: owns every module and runs the state machine.
//
//   main() -> Game::run()
//     init()      builds the Building from LevelData.cpp, the cast from CharacterArt.cpp / DialogueData.cpp, the case
//     per frame   gather Input -> update() -> Renderer draws the world -> Ui draws the screen
//
//   Modes:  WALK (exploring)  KNOCK (door sequence)  TALK (conversation)  JOURNAL (reading)  END (case closed)
//           FADE (changing floor by the stairs)  RIDE (in the lift)
// Public data members are read by Ui.cpp, which only draws.
#pragma once
#include "Audio.h"
#include "ClueBook.h"
#include "Dialogue.h"
#include "Entities.h"
#include "Level.h"
#include "Renderer.h"

class Ui;

enum Mode { M_WALK, M_KNOCK, M_TALK, M_JOURNAL, M_END, M_FADE, M_RIDE, M_PEEP };

struct LogLine { string text; Color col; };
struct JournalState { int tab = 0, spread = 0; vector<string> seen; };   // tab: 0 case, 1 notes, 2 people, 3 board

class Game {
public:
    Game();
    ~Game();
    void init();                                  // build the world (needs no window - used by the tests)
    int run();                                    // open the window and play until quit
    void update(float dt, const Input& in);       // advance the game by dt seconds

    string prompt() const;                        // what [E] would do right now ("" = nothing)
    int minutes() const { return (int)clock % (24 * 60); }
    void say(const string& t, Color c) { log.push_back({t, c}); }
    Character* residentAt(int floor, int door);

    // ---- the world
    BuildingDef def;
    Building building;
    Player player;
    ClueBook book;
    Weather weather;
    Renderer renderer;
    vector<std::unique_ptr<Character>> cast;
    vector<std::unique_ptr<Prop>> props;
    Conversation talk;

    // ---- state the Ui draws
    Mode mode = M_WALK, prevMode = M_WALK;
    float time = 0, clock = 14 * 60 + 5;          // clock: in-game minutes after midnight (one game minute per two seconds)
    float modeT = 0, shake = 0, bars = 0, endT = 0;
    float fade = 0;                               // 0..1 black overlay (floor changes, lift rides)
    string caption; float capT = 0;
    string toast; float toastT = 0;
    vector<LogLine> log;
    JournalState journal;
    bool quit = false, solved = false, lookOn = false;

private:
    vector<Billboard> collectBillboards();
    void updateWalk(const Input& in, float dt);
    void interact();
    void startKnock(const DoorInfo& d);
    void updateKnock(float dt);
    void startTalk(Character& c);
    void endTalk();
    void updateTalk(float dt, const Input& in);
    void startEnding();
    void examineSealed();
    void startFade(int link);
    void updateFade(float dt);
    void callElevator();
    void startRide();
    void updateRide(float dt);
    void updateElevatorDoors();
    void openJournal();
    void closeJournal();
    void journalInput(const Input& in);

    vector<Character*> owners;                    // parallel to the billboards of the last frame (who is who)
    DoorInfo knockDoor;
    Character* knockWho = nullptr;
    int knockBeat = 0; bool knockSaid = false;
    Character* talkWho = nullptr;
    int fadeLink = 0; bool fadeSwapped = false;
    int nextLiftFloor() const;
    Audio audio;                                  // everything you hear (Audio.h); silent until run() opens the sound card
    AudioScene makeAudioScene(float dt) const;
    // peepholes: each empty flat is a tiny level of its own, seen through a fisheye lens from just inside the door
    vector<Level> flats; vector<vector<Billboard>> flatBills; vector<std::pair<int, int>> flatKeys;   // flatKeys[i] = (floor, door) of flats[i]
    int peepFlat = -1; float peepYaw = 0, peepT = 0;
    int flatFor(int floor, int door) const;
    void tryPeephole();
    int rideTarget = 0; bool rideSwapped = false, rideOpened = false;
    float linkCooldown = 0, liftIdle = 0;
    std::unique_ptr<Ui> ui;
};
