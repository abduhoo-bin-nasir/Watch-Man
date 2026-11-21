// logic_test.cpp - headless tests of the game rules (no window needed).
// Plays the game through code: stairs both ways, the lift, door knocks, every interview, the whole case to the ending,
// then 40,000 steps of random input checking the player never ends up inside a wall.
// Build target: logic_test (see CMakeLists.txt). Prints "ok:" / "FAIL:" lines and a summary; exit code 1 on failure.
#include "Game.h"
#include <cstdio>
#include <cstdlib>
static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf("FAIL: %s (line %d)\n", msg, __LINE__); fails++; } else std::printf("ok:   %s\n", msg); } while (0)
static Game* G;
static void tickN(int n, Input in = Input()) { for (int i = 0; i < n; i++) { Input x = in; if (i > 0) { x.interact = false; x.enter = false; x.esc = false; x.journal = false; x.num = 0; x.pick = -1; } G->update(1.0f / 60, x); } }
static void place(float x, float y, float yaw, int floor = -1) { if (floor >= 0) { G->building.setCurrent(floor); G->player.floor = floor; } G->player.x = x; G->player.y = y; G->player.yaw = yaw; G->player.pitch = 0; }
static void press(bool Input::*f) { Input in; in.*f = true; G->update(1.0f / 60, in); }
static bool ask(const string& sub) {
    for (int t = 0; t < 400 && G->talk.reveal < (float)G->talk.text.size(); t++) tickN(1);
    for (size_t i = 0; i < G->talk.menu.size(); i++) if (G->talk.menu[i].label.find(sub) != string::npos) { Input in; in.pick = (int)i; G->update(1.0f / 60, in); tickN(2); return true; }
    return false;
}
static void knock(float x, float y, float yaw) { place(x, y, yaw); tickN(3); press(&Input::interact); tickN(260); }
static const float PI_ = 3.14159265f;

int main() {
    Game g; G = &g; g.init();
    // ---------- architecture
    CHECK(g.building.count() == 2, "two floors");
    const Level& L0 = g.building.level(0); const Level& L1 = g.building.level(1);
    CHECK(L0.cellsW == L1.cellsW && L0.cellsH == L1.cellsH, "both floors have the identical rectangular footprint");
    // every cell of the linked flight: the upper floor is exactly one storey lower than the lower floor
    int flightCells = 0, mismatch = 0;
    for (int y = 0; y < L0.cellsH; y++) for (int x = 0; x < L0.cellsW; x++) {
        const Cell& a = L0.cellAt(x, y); const Cell& b = L1.cellAt(x, y);
        if (a.stair && b.stair && x >= 16 && x <= 17) { flightCells++; if (std::fabs((a.F - WALL_H) - b.F) > 1e-4f) mismatch++; }
    }
    CHECK(flightCells == 20 && mismatch == 0, "the stairs between floors are one physical flight (heights agree cell by cell)");
    CHECK(L0.hasElevator() && L1.hasElevator(), "a lift on every floor, same corner");
    // the stairwell is in the centre of the footprint
    CHECK(L0.cellAt(17, 10).stair && L0.cellAt(21, 18).stair, "ground floor: up flight (x=8) and down flight (x=10) in the middle");
    CHECK(L1.cellAt(17, 10).stair && L1.cellAt(21, 10).stair, "second floor: down flight and up flight in the middle");
    // no dustbins/plants any more
    CHECK(g.props.size() == 3, "only crates remain as props");

    // ---------- up the stairs
    place(17.0f, 4.6f, PI_ / 2, 0); tickN(5);
    { Input in; in.fw = 1; bool went = false; for (int i = 0; i < 700 && !went; i++) { g.update(1.0f / 60, in); if (g.building.currentIndex() == 1) went = true; }
      CHECK(went, "walking up the flight takes you to floor 2"); }
    tickN(60);
    CHECK(g.mode == M_WALK && g.fade == 0, "fade finished, walking again");
    { const Level& L = g.building.current(); const Cell& c = L.cellAt((int)std::floor(g.player.x), (int)std::floor(g.player.y));
      std::printf("      arrived at (%.2f,%.2f) feet %.2f cell F %.2f stair=%d\n", g.player.x, g.player.y, g.player.feet, c.F, (int)c.stair);
      CHECK(c.type == C_OPEN && std::fabs(g.player.feet - c.F) < 0.3f, "arrived on the stairs of floor 2, at the right height"); }
    { Input in; in.fw = 1; for (int i = 0; i < 200; i++) g.update(1.0f / 60, in); }
    CHECK(g.building.currentIndex() == 1 && g.player.y > 16.0f, "walked on out of the stairwell into the second-floor corridor");
    // ---------- down again
    place(g.player.x, g.player.y, -PI_ / 2); tickN(3);
    { Input in; in.fw = 1; bool went = false; for (int i = 0; i < 900 && !went; i++) { g.update(1.0f / 60, in); if (g.building.currentIndex() == 0) went = true; } CHECK(went, "walking down the flight takes you back to the ground floor"); }
    tickN(60);
    { const Level& L = g.building.current(); const Cell& c = L.cellAt((int)std::floor(g.player.x), (int)std::floor(g.player.y)); CHECK(c.type == C_OPEN && std::fabs(g.player.feet - c.F) < 0.3f, "arrived back at the right height"); }

    // ---------- locked doors at the ends of the other two flights
    place(21.0f, 26.0f - 4.0f, -PI_ / 2, 0); tickN(3);   // down flight on the ground floor: walk down to the boiler door
    { Input in; in.fw = 1; for (int i = 0; i < 900; i++) g.update(1.0f / 60, in); }
    CHECK(g.building.currentIndex() == 0 && g.player.y > 12.0f && g.player.y < 15.5f, "the down flight ends at a locked door (no floor change, no clipping)");
    press(&Input::interact);
    CHECK(g.log.back().text.find("BOILER") != std::string::npos, "E on the boiler door: padlocked");

    // ---------- the lift
    place(3.0f, 9.0f, 0.0f, 0); tickN(3);
    CHECK(g.prompt().find("lift") != std::string::npos || g.prompt().find("Call") != std::string::npos, "prompt: call the lift");
    press(&Input::interact); tickN(120);
    CHECK(g.building.level(0).doorAmt[DOOR_ELEVATOR] > 0.99f, "the lift doors slide open");
    { Input in; in.fw = 1; for (int i = 0; i < 60; i++) g.update(1.0f / 60, in); }
    CHECK(g.building.level(0).inCar(g.player.x, g.player.y), "you can walk into the cabin");
    press(&Input::interact);
    CHECK(g.mode == M_RIDE, "E inside the cabin starts the ride");
    tickN(280);
    CHECK(g.building.currentIndex() == 1 && g.mode == M_WALK, "the lift carried you to floor 2");
    CHECK(g.building.current().inCar(g.player.x, g.player.y) && g.building.current().doorAmt[DOOR_ELEVATOR] > 0.99f, "still in the cabin, doors open");
    { Input in; in.fw = -1; for (int i = 0; i < 90; i++) g.update(1.0f / 60, in); }
    { const Cell& c = g.building.current().cellAt((int)std::floor(g.player.x), (int)std::floor(g.player.y)); CHECK(c.type == C_OPEN && !g.building.current().inCar(g.player.x, g.player.y), "walked out of the lift into the corridor"); }
    tickN(400);
    CHECK(g.building.current().doorAmt[DOOR_ELEVATOR] < 0.05f, "the lift doors close after you walk away");

    // ---------- doors and conversations (as before, new coordinates)
    place(0, 0, 0, 0);
    knock(11.0f, 3.0f, PI_ / 2);
    CHECK(g.mode == M_TALK && g.talk.who && g.talk.who->id == "volkova", "knock on 101: Mrs. Volkova answers");
    CHECK(g.building.level(0).doorAmt[1] > 0.95f, "the door is open");
    { float d = std::hypot(g.player.x - g.talk.who->x, g.player.y - g.talk.who->y); std::printf("      distance %.2f\n", d); CHECK(d < 2.4f, "she stands right in front of you"); }
    CHECK(ask("Did you hear anything") && g.book.has("argue"), "clue ARGUE in the journal");
    CHECK(ask("Who was he arguing") && g.book.has("debt"), "clue DEBT");
    ask("That's all"); tickN(60);
    CHECK(g.mode == M_WALK && g.building.level(0).doorAmt[1] < 0.05f, "she closes the door");
    place(11.0f, 3.0f, PI_ / 2);
    { Input in; in.fw = 1; for (int i = 0; i < 300; i++) g.update(1.0f / 60, in); }
    CHECK(g.player.y < 3.7f, "you can never walk into the apartment");
    knock(3.0f, 15.0f, 0.0f);
    CHECK(g.mode == M_WALK && g.log.back().text.find("Night shift") != std::string::npos, "103: no answer");
    place(35.0f, 19.0f, PI_); tickN(3); press(&Input::interact);
    CHECK(g.book.has("boot"), "examine sealed 106: boot print");
    knock(35.0f, 11.0f, PI_);
    CHECK(g.mode == M_TALK && g.talk.who->id == "misha" && ask("Did you see anyone") && g.book.has("scarf"), "Misha: RED SCARF");
    ask("That's all"); tickN(60);
    for (auto& c : g.cast) if (c->id == "cat") { static_cast<Wanderer*>(c.get())->setRoute({}, 0); c->x = 12; c->y = 27; }
    place(8.0f, 27.0f, 0.0f, 0); tickN(3); press(&Input::interact);
    CHECK(g.mode == M_TALK && g.talk.who->id == "cat" && ask("Check his collar") && g.book.has("wool") && g.book.has("match"), "Boris: RED WOOL -> THE PICTURE deduction");
    ask("That's all"); tickN(10);
    for (auto& c : g.cast) if (c->id == "scarf") { static_cast<Wanderer*>(c.get())->setRoute({}, 0); c->x = 12; c->y = 3; }
    place(8.0f, 3.0f, 0.0f); tickN(3); press(&Input::interact);
    CHECK(g.mode == M_TALK && g.talk.who->id == "scarf" && g.book.canConfront(), "scarf man, enough evidence");
    ask("CONFRONT"); ask("Continue");
    CHECK(g.mode == M_END, "case closed");

    // ---------- fuzz with sanitizers
    Game f; G = &f; f.init(); std::srand(11);
    for (int it = 0; it < 40000; it++) {
        Input in;
        in.fw = (float)(std::rand() % 3 - 1); in.st = (float)(std::rand() % 3 - 1); in.turn = (float)(std::rand() % 3 - 1);
        in.interact = std::rand() % 30 == 0; in.enter = std::rand() % 50 == 0; in.up = std::rand() % 30 == 0; in.down = std::rand() % 30 == 0;
        in.left = std::rand() % 60 == 0; in.right = std::rand() % 60 == 0; in.journal = std::rand() % 400 == 0; in.esc = std::rand() % 700 == 0;
        in.num = std::rand() % 6 == 0 ? std::rand() % 10 : 0; f.quit = false;
        in.pick = std::rand() % 90 == 0 ? std::rand() % 12 - 2 : -1; in.hover = std::rand() % 90 == 0 ? std::rand() % 12 : -1;
        if (it % 1200 == 0) { int fl = std::rand() % 2; f.building.setCurrent(fl); f.player.floor = fl; f.player.x = 2.0f + std::rand() % 30 + 0.5f; f.player.y = 3.0f; f.player.yaw = (float)(std::rand() % 628) / 100.0f; }
        f.update(1.0f / 60, in);
        const Cell& c = f.building.current().cellAt((int)std::floor(f.player.x), (int)std::floor(f.player.y));
        if (c.type != C_OPEN && f.mode != M_JOURNAL) { std::printf("FAIL: player inside a wall at (%.2f,%.2f) floor %d mode %d\n", f.player.x, f.player.y, f.building.currentIndex(), (int)f.mode); fails++; break; }
    }
    std::printf("fuzz done (floor %d, mode %d)\n\n%s (%d failures)\n", f.building.currentIndex(), (int)f.mode, fails ? "SOME CHECKS FAILED" : "ALL CHECKS PASSED", fails);
    return fails ? 1 : 0;
}
