// Game.cpp - the state machine: walking, knocking, talking, the journal, stairs, the lift and the ending.
#include "Game.h"

#include "Ui.h"

Game::Game() : ui(std::make_unique<Ui>()) {}
Game::~Game() = default;

static_assert((int)M_WALK == (int)AM_WALK && (int)M_KNOCK == (int)AM_KNOCK && (int)M_TALK == (int)AM_TALK && (int)M_JOURNAL == (int)AM_JOURNAL &&
              (int)M_END == (int)AM_END && (int)M_FADE == (int)AM_FADE && (int)M_RIDE == (int)AM_RIDE && (int)M_PEEP == (int)AM_PEEP,
              "Game's Mode and Audio's AudioMode must stay in the same order");

// A snapshot of what the sound manager needs to know this frame (Audio.h): where you are, who is moving, what is happening.
AudioScene Game::makeAudioScene(float dt) const {
    AudioScene s;
    s.dt = dt; s.time = (double)time; s.mode = (int)mode;
    s.floor = building.currentIndex(); s.px = player.x; s.py = player.y; s.yaw = player.yaw;
    s.building = &building;
    s.tab = journal.tab; s.spread = journal.spread;
    s.boltT = weather.boltT; s.boltAz = weather.boltAz; s.toastT = toastT;
    if (mode == M_PEEP && peepFlat >= 0 && peepFlat < (int)flatKeys.size()) { s.peepFloor = flatKeys[(size_t)peepFlat].first; s.peepDoor = flatKeys[(size_t)peepFlat].second; }
    for (const auto& c : cast)
        if (!c->hidden && !c->isResident()) s.movers.push_back(AudioMover{c.get(), c->floor, c->x, c->y, c->id == "cat"});
    return s;
}

// The lift only serves floors that have a lift cabin (the roof has none): the next such floor, going round.
int Game::nextLiftFloor() const {
    int n = building.count(), cur = building.currentIndex();
    for (int i = 1; i < n; i++) {
        int f = (cur + i) % n;
        if (building.level(f).hasElevator()) return f;
    }
    return cur;
}

void Game::init() {
    def = designBuilding();
    building.build(def);
    renderer.buildTextures(building.count());
    book.loadCase();
    cast = buildCast(def);
    props = buildProps(def);
    for (const FlatDef& fd : def.flats) {                                  // the rooms behind the peepholes
        Level lv; lv.build(fd.plan, 0, {}, 1);
        vector<Billboard> bills;
        for (const PropDef& d : fd.props) {
            Billboard s; s.x = d.at.x * 2; s.y = d.at.y * 2; s.yaw = d.yaw; s.h = d.h; s.art = d.art; s.main = d.main; s.acc = d.main; s.floorH = 0;
            bills.push_back(s);
        }
        flats.push_back(std::move(lv)); flatBills.push_back(std::move(bills)); flatKeys.push_back({fd.floor, fd.door});
    }
    player.floor = building.currentIndex();
    player.x = def.start.x * 2; player.y = def.start.y * 2; player.yaw = def.startYaw;
    say("Block 7. The stairs are in the middle, the lift is in the corner.", COL_SYS);
}

Character* Game::residentAt(int floor, int door) {
    for (auto& c : cast) if (c->isResident() && c->floor == floor && static_cast<Resident*>(c.get())->door == door) return c.get();
    return nullptr;
}

// ---------------------------------------------------------------- what is on screen
vector<Billboard> Game::collectBillboards() {
    vector<Billboard> out; owners.clear();
    const Level& L = building.current();
    int fl = building.currentIndex();
    auto cellFloor = [&](const Entity& e) { return L.cellAt((int)std::floor(e.x), (int)std::floor(e.y)).F; };
    for (auto& c : cast) {
        if (c->floor != fl || c->hidden) continue;
        if (c->isResident() && !static_cast<Resident*>(c.get())->inDoorway) continue;
        Billboard b; b.x = c->x; b.y = c->y; b.yaw = c->yaw; b.h = c->h; b.lit = c->lit; b.art = c->art; b.main = c->main; b.acc = c->acc;
        b.floorH = c->floorOverride >= 0 ? c->floorOverride : cellFloor(*c);
        b.who = c.get();
        out.push_back(b); owners.push_back(c.get());
    }
    for (auto& p : props) {
        if (p->floor != fl) continue;
        Billboard b; b.x = p->x; b.y = p->y; b.yaw = p->yaw; b.h = p->h; b.art = p->art; b.main = p->main; b.floorH = cellFloor(*p);
        out.push_back(b); owners.push_back(nullptr);
    }
    return out;
}

string Game::prompt() const {
    if (mode != M_WALK) return "";
    const Level& L = building.current();
    int nb = renderer.nearBill;
    if (nb >= 0 && nb < (int)owners.size() && owners[(size_t)nb]) return "[E] " + owners[(size_t)nb]->verb + " " + owners[(size_t)nb]->name;
    if (L.inCar(player.x, player.y)) return "[E] Ride to the " + building.level(nextLiftFloor()).name;
    const Focus& f = renderer.focus;
    if (f.valid && f.type == C_DOOR && f.t < 3.6f) {
        DoorInfo d = L.doorInfoAt(f.cx, f.cy);
        if (d.id == DOOR_ELEVATOR) return L.doorTgt[DOOR_ELEVATOR] > 0.5f ? "" : "[E] Call the lift";
        if (d.id == DOOR_BOILER) return "[E] Try the boiler-room door";
        if (d.id == DOOR_ROOF) return "[E] Try the roof door";
        if (d.floor == 0 && d.id == 6) return "[E] Examine the sealed door (106)   [P] Peephole";
        return "[E] Knock on Apt " + L.doorLabel(d.id) + "   [P] Peephole";
    }
    return "";
}

// ---------------------------------------------------------------- one frame
void Game::update(float dt, const Input& in) {
    struct AudioFeed {                                                      // whichever way update() returns, the sound manager sees this frame
        Game& g; float dt;
        ~AudioFeed() { if (g.audio.ready()) g.audio.update(g.makeAudioScene(dt)); }
    } audioFeed{*this, dt};
    if (mode == M_JOURNAL) { journalInput(in); return; }                   // the world holds still while you read
    if (in.journal && (mode == M_WALK || mode == M_TALK)) { openJournal(); return; }
    time += dt; clock += dt * 0.5f;
    toastT = std::max(0.0f, toastT - dt); shake = std::max(0.0f, shake - dt * 3.0f); capT = std::max(0.0f, capT - dt);
    linkCooldown = std::max(0.0f, linkCooldown - dt);
    bars += ((mode == M_WALK || mode == M_FADE || mode == M_RIDE || mode == M_PEEP ? 0.0f : 1.0f) - bars) * (1.0f - std::exp(-dt * 6.0f));
    { string t = book.takeToast(); if (!t.empty()) { toast = t; toastT = 4.5f; } }
    weather.update(dt, time, player.yaw);

    Level& L = building.current();
    L.updateLamps(time, weather.flash);

    // the player
    if (mode == M_WALK) updateWalk(in, dt);
    else {
        player.moving = false;
        if (mode == M_KNOCK) {                                            // step up to the door and look at it
            player.easeTo(L, knockDoor.tcx + knockDoor.nx * 2.5f, knockDoor.tcy + knockDoor.ny * 2.5f, dt);
            player.lookAt(knockDoor.tcx + knockDoor.nx * 0.55f, knockDoor.tcy + knockDoor.ny * 0.55f, dt);
        } else if (mode == M_TALK && talkWho) {
            Character& c = *talkWho;
            if (c.isResident()) player.easeTo(L, knockDoor.tcx + knockDoor.nx * 2.5f, knockDoor.tcy + knockDoor.ny * 2.5f, dt);
            else {                                                         // a hallway person walks up and faces you
                float ex = player.x - c.x, ey = player.y - c.y, d = std::hypot(ex, ey);
                if (d > 1.75f) { float st = std::min(d - 1.75f, 3.2f * dt); c.x += ex / d * st; c.y += ey / d * st; }
                c.yaw = std::atan2(ey, ex);
            }
            float dist = std::max(1.0f, std::hypot(player.x - c.x, player.y - c.y));   // aim at the face (or the middle of something small)
            float aim = c.h * WALL_H * (c.h < 0.5f ? 0.5f : 0.8f);
            player.lookAt(c.x, c.y, dt, std::clamp((aim - EYE_H) * RH / dist * 0.6f, -0.2f * RH, 0.1f * RH));
        }
    }
    player.settle(L, dt);

    for (auto& c : cast) c->update(dt);                                    // patrols
    for (int i = 0; i < building.count(); i++) {                           // doors swing open and shut
        Level& lv = building.level(i);
        for (int d = 0; d < DOOR_KINDS; d++) {
            float diff = lv.doorTgt[d] - lv.doorAmt[d], st = 2.2f * dt;
            lv.doorAmt[d] += std::clamp(diff, -st, st);
        }
    }
    updateElevatorDoors();
    for (auto& c : cast)                                                   // a resident leaves once the door is nearly shut
        if (c->isResident()) {
            Resident& r = static_cast<Resident&>(*c);
            if (r.inDoorway && !r.talking && building.level(r.floor).doorAmt[r.door] < 0.3f) r.inDoorway = false;
        }

    // draw the world
    vector<Billboard> bills = collectBillboards();
    CamState cs{player.x, player.y, player.feet, player.yaw, player.pitch, player.bob};
    if (mode == M_PEEP && peepFlat >= 0 && peepFlat < (int)flats.size()) {   // through the peephole: the room behind the door, fisheye, a little unsteady
        flats[(size_t)peepFlat].updateLamps(time, weather.flash);
        CamState pc{2.05f, 5.0f, 0.0f, peepYaw + std::sin(time * 0.9f) * 0.02f, 0.0f, 0.0f};
        pc.fish = true;
        renderer.render(flats[(size_t)peepFlat], pc, flatBills[(size_t)peepFlat], weather, time);
    } else renderer.render(L, cs, bills, weather, time);

    // mode-specific updates (they may switch the mode)
    switch (mode) {
        case M_KNOCK: updateKnock(dt); break;
        case M_TALK: updateTalk(dt, in); break;
        case M_FADE: updateFade(dt); break;
        case M_RIDE: updateRide(dt); break;
        case M_PEEP:
            peepT += dt;
            peepYaw = std::clamp(peepYaw + in.turn * 0.9f * dt, -0.45f, 0.45f);
            if (in.esc || in.interact || in.peep) { mode = M_WALK; peepFlat = -1; }
            break;
        case M_END:
            endT += dt;
            if (in.esc) quit = true;
            else if (endT > 4.0f && (in.enter || in.interact)) {            // back to the hallway; he has been taken away
                for (auto& c : cast) if (c->id == "scarf") { c->hidden = true; c->talking = false; }
                talkWho = nullptr; mode = M_WALK;
                say("The hallway is quiet. The rain is not.", COL_SYS);
            }
            break;
        case M_WALK:
            if (in.esc) quit = true;
            if (in.interact) interact();
            else if (in.peep) tryPeephole();
            break;
        default: break;
    }
}

void Game::updateWalk(const Input& in, float dt) {
    Level& L = building.current();
    player.steer(in, dt);
    player.walk(L, in.fw, in.st, dt);
    const Cell& here = L.cellAt((int)std::floor(player.x), (int)std::floor(player.y));
    if (here.link != 0 && linkCooldown <= 0.0f && building.count() > 1) startFade(here.link);   // the top/bottom steps of a flight
    // the lift doors close by themselves a few seconds after you step away from the doorway (never while you are in it)
    if (L.hasElevator() && L.doorTgt[DOOR_ELEVATOR] > 0.5f && L.doorAmt[DOOR_ELEVATOR] > 0.99f && !L.inCar(player.x, player.y)) {
        V2 dc = L.elevatorDoorCentre();
        bool clear = std::max(std::fabs(player.x - dc.x), std::fabs(player.y - dc.y)) >= 1.4f;   // not standing in the doorway
        liftIdle = clear ? liftIdle + dt : 0.0f;
        if (liftIdle > 5.0f) { L.doorTgt[DOOR_ELEVATOR] = 0; liftIdle = 0; }
    } else liftIdle = 0;
}

void Game::updateElevatorDoors() {
    for (int i = 0; i < building.count(); i++) {
        Level& lv = building.level(i);
        if (lv.hasElevator()) lv.setElevatorOpen(lv.doorAmt[DOOR_ELEVATOR] >= 0.999f);   // fully open = a walkable doorway
    }
}

int Game::flatFor(int floor, int door) const {
    for (size_t i = 0; i < flatKeys.size(); i++) if (flatKeys[i].first == floor && flatKeys[i].second == door) return (int)i;
    return -1;
}
// [P] at an apartment door: look through the peephole. Empty flats show their room; people who are home just notice the lens.
void Game::tryPeephole() {
    const Level& L = building.current();
    const Focus& f = renderer.focus;
    if (!(f.valid && f.type == C_DOOR && f.t < 3.6f)) return;
    DoorInfo d = L.doorInfoAt(f.cx, f.cy);
    if (d.id < 1 || d.id > DOOR_APT_MAX) { say("No peephole on this one.", COL_ME); return; }
    if (d.floor == 0 && d.id == 6) { say("The lens is sealed under crime-scene tape.", COL_ME); return; }
    if (d.floor == 1 && d.id == 4) { say("Foil. Somebody has covered the lens with foil. Of course.", COL_ME); return; }
    if (residentAt(d.floor, d.id)) { say("Something moves behind the lens. Knock, and somebody will answer.", COL_ME); return; }
    int k = flatFor(d.floor, d.id);
    if (k < 0) { say("Too dark to see anything through the lens.", COL_ME); return; }
    peepFlat = k; peepYaw = 0; peepT = 0; mode = M_PEEP; modeT = 0;
}
void Game::interact() {
    const Level& L = building.current();
    int nb = renderer.nearBill;
    if (nb >= 0 && nb < (int)owners.size() && owners[(size_t)nb]) { startTalk(*owners[(size_t)nb]); return; }
    if (L.inCar(player.x, player.y)) { startRide(); return; }
    const Focus& f = renderer.focus;
    if (!(f.valid && f.type == C_DOOR && f.t < 3.6f)) return;
    DoorInfo d = L.doorInfoAt(f.cx, f.cy);
    if (d.id == DOOR_ELEVATOR) callElevator();
    else if (d.id == DOOR_BOILER) say("Padlocked. A hand-painted sign: BOILER ROOM. STAFF ONLY.", COL_ME);
    else if (d.id == DOOR_ROOF) say("A steel door, chained shut. ROOF ACCESS. The chain looks new.", COL_ME);
    else if (d.floor == 0 && d.id == 6) examineSealed();
    else startKnock(d);
}

void Game::examineSealed() {
    if (!book.has("boot")) say("Tape across the frame. A fresh muddy boot print by the hinge: size 44, I'd say.", COL_ME);
    else say("Tape, seals, the boot print. Nothing new. Forensics want the door untouched.", COL_SYS);
    book.give("boot", minutes());
}

// ---------------------------------------------------------------- knocking
void Game::startKnock(const DoorInfo& d) {
    knockDoor = d; mode = M_KNOCK; modeT = 0; knockBeat = 0; knockSaid = false; caption.clear();
    knockWho = residentAt(d.floor, d.id);
}

void Game::updateKnock(float dt) {
    modeT += dt;
    float t = modeT;
    static const float B[3] = {0.35f, 0.85f, 1.35f};
    while (knockBeat < 3 && t >= B[knockBeat]) {
        shake = knockBeat == 2 ? 1.0f : 0.6f; knockBeat++;
        caption = knockBeat == 1 ? "Knock." : knockBeat == 2 ? "Knock. Knock." : "Knock. Knock. KNOCK.";
        capT = 2.4f;
    }
    Level& L = building.current();
    if (!knockWho) {                                                      // nobody comes
        if (t > 2.6f) { say("Knock knock, Apt " + L.doorLabel(knockDoor.id) + ". " + noAnswerLine(knockDoor.floor, knockDoor.id), COL_ME); mode = M_WALK; }
        return;
    }
    if (!knockSaid && t > 2.2f) { say(knockWho->script.openLine, COL_SYS); knockSaid = true; }
    if (t > 2.9f) {                                                       // the door opens and the resident steps into the doorway
        static_cast<Resident*>(knockWho)->appearAt(knockDoor, L);
        L.doorTgt[knockDoor.id] = 1;
        caption.clear();
        startTalk(*knockWho);
    }
}

// ---------------------------------------------------------------- talking
void Game::startTalk(Character& c) {
    talkWho = &c; c.talking = true; mode = M_TALK; modeT = 0;
    if (!c.isResident()) c.yaw = std::atan2(player.y - c.y, player.x - c.x);     // a hallway person turns to face you
    talk.begin(c, book, minutes());
}

void Game::endTalk() {
    if (talkWho) {
        talkWho->talking = false;
        if (talkWho->isResident()) {
            Resident& r = static_cast<Resident&>(*talkWho);
            building.level(r.floor).doorTgt[r.door] = 0;
            say("The door closes. A chain slides back into place.", COL_SYS);
        }
    }
    mode = M_WALK; talkWho = nullptr;
}

void Game::updateTalk(float dt, const Input& in) {
    Conversation::Result r = talk.update(dt, in, minutes());
    if (r == Conversation::R_LEAVE) endTalk();
    else if (r == Conversation::R_ENDING) startEnding();
}

void Game::startEnding() { mode = M_END; endT = 0; solved = true; say("Case closed.", COL_ME); }

// ---------------------------------------------------------------- the stairs: a short fade to the other floor
void Game::startFade(int link) { mode = M_FADE; modeT = 0; fadeLink = link; fadeSwapped = false; }

void Game::updateFade(float dt) {
    modeT += dt;
    const float out = 0.18f, in = 0.30f;
    if (modeT < out) fade = modeT / out;
    else {
        if (!fadeSwapped) {                                               // same x,y on the other floor; the floor is one storey away
            int to = std::clamp(building.currentIndex() + fadeLink, 0, building.count() - 1);
            player.feet -= (float)fadeLink * WALL_H;
            building.setCurrent(to); player.floor = to;
            fadeSwapped = true; linkCooldown = 0.5f;
        }
        fade = 1.0f - (modeT - out) / in;
        if (modeT >= out + in) { fade = 0; mode = M_WALK; }
    }
}

// ---------------------------------------------------------------- the lift
void Game::callElevator() {
    Level& L = building.current();
    if (L.doorTgt[DOOR_ELEVATOR] > 0.5f) return;
    L.doorTgt[DOOR_ELEVATOR] = 1;
    say("The lift hums, a bell rings, the doors slide open.", COL_SYS);
}

void Game::startRide() {
    mode = M_RIDE; modeT = 0; rideSwapped = false; rideOpened = false;
    rideTarget = nextLiftFloor();
    building.current().doorTgt[DOOR_ELEVATOR] = 0;                        // doors close with you inside
    say("You press the button. The doors close.", COL_SYS);
}

void Game::updateRide(float dt) {
    modeT += dt;
    float t = modeT;
    Level* L = &building.current();
    if (t > 0.9f && t < 1.0f) shake = 0.5f;
    fade = t < 0.9f ? 0.0f : t < 1.5f ? (t - 0.9f) / 0.6f : t < 1.6f ? 1.0f : t < 2.2f ? 1.0f - (t - 1.6f) / 0.6f : 0.0f;
    if (t >= 1.6f && !rideSwapped) {                                      // the cabin is in the same place on every floor
        building.setCurrent(rideTarget); player.floor = rideTarget; player.feet = 0;
        L = &building.current();
        L->doorAmt[DOOR_ELEVATOR] = 0; L->doorTgt[DOOR_ELEVATOR] = 0; L->setElevatorOpen(false);
        rideSwapped = true;
        caption = "Ding."; capT = 1.6f;
        say("Ding. " + L->name + ".", COL_ME);
    }
    if (t >= 2.4f && !rideOpened) { L->doorTgt[DOOR_ELEVATOR] = 1; rideOpened = true; }
    if (t >= 3.3f) { fade = 0; mode = M_WALK; }
}

// ---------------------------------------------------------------- the journal
void Game::openJournal() {
    prevMode = mode; mode = M_JOURNAL; journal.spread = 0;
    for (const string& id : book.found) if (book.clue(id).fresh) { journal.tab = 1; break; }
}
void Game::closeJournal() {
    for (const string& id : journal.seen) book.clue(id).fresh = false;
    journal.seen.clear();
    mode = prevMode;
}
void Game::journalInput(const Input& in) {
    if (in.journal || in.esc) { closeJournal(); return; }
    if (in.left) { journal.tab = (journal.tab + 3) % 4; journal.spread = 0; }
    if (in.right) { journal.tab = (journal.tab + 1) % 4; journal.spread = 0; }
    if (in.num >= 1 && in.num <= 4) { journal.tab = in.num - 1; journal.spread = 0; }
    if (in.tabPick >= 0 && in.tabPick < 4) { journal.tab = in.tabPick; journal.spread = 0; }
    if (in.up) journal.spread = std::max(0, journal.spread - 1);
    if (in.down) journal.spread++;                                        // the drawing code clamps this to the last spread
}

// ---------------------------------------------------------------- window + main loop
int Game::run() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1100, 740, "Block 7 - pixel raycaster (raylib)");
    SetWindowMinSize(640, 480);
    SetExitKey(KEY_NULL);                       // ESC backs out of menus instead of closing the game
    SetTargetFPS(60);
    init();
    ui->init();
    audio.buildAsync();                         // the sounds are made on a second thread (about 2 s); the game starts at once and fades them in
    audio.openDevice();
    while (!WindowShouldClose() && !quit) {
        Input in;
        bool walk = mode == M_WALK;
        bool look = walk || mode == M_PEEP;                       // turning is allowed while walking and while looking through a peephole
        in.turn = look ? (float)(IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)) : 0;
        if (mode == M_PEEP) in.turn += (float)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
        in.fw = walk ? (float)((IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) - (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))) : 0;
        in.st = walk ? (float)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A)) : 0;
        in.interact = IsKeyPressed(KEY_E);
        in.peep = IsKeyPressed(KEY_P);
        in.enter = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
        in.esc = IsKeyPressed(KEY_ESCAPE);
        in.journal = IsKeyPressed(KEY_J);
        if (IsKeyPressed(KEY_M)) audio.toggleMute();       // M: all sound on/off
        in.up = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_PAGE_UP);
        in.down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_PAGE_DOWN);
        in.left = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
        in.right = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
        for (int n = 1; n <= 9; n++) if (IsKeyPressed(KEY_ONE + n - 1) || IsKeyPressed(KEY_KP_1 + n - 1)) in.num = n;
        if (walk && IsKeyPressed(KEY_TAB)) lookOn = !lookOn;
        bool hide = lookOn && walk;
        if (hide && !IsCursorHidden()) DisableCursor();
        if (!hide && IsCursorHidden()) EnableCursor();
        if (IsCursorHidden()) { in.mouseX = GetMouseDelta().x; in.mouseY = GetMouseDelta().y; }
        if (walk) { in.pitchKey = (float)(IsKeyDown(KEY_PAGE_UP) - IsKeyDown(KEY_PAGE_DOWN)); in.recenter = IsKeyPressed(KEY_HOME); in.up = in.down = in.left = in.right = false; }
        ui->pointerInput(*this, in);
        update(std::min(0.05f, GetFrameTime()), in);

        BeginDrawing();
        ui->draw(*this, GetScreenWidth(), GetScreenHeight());
        EndDrawing();
    }
    audio.closeDevice();
    ui->shutdown();
    CloseWindow();
    return 0;
}
