// Audio.cpp - the mixer, the sound manager and the sound-card glue. See Audio.h for the big picture.
#include "Audio.h"
#include "AudioFiles.h"
#include <algorithm>
#include <cmath>
#include <raylib.h>

namespace {
const float TAU = 6.28318530718f;
float alphaFor(float fc) { return 1.0f - std::exp(-TAU * fc / (float)SR); }   // one-pole low-pass coefficient for a cutoff in Hz
enum Ev { EV_THUNDER = 1, EV_MEOW, EV_SLAM, EV_PIPE };
}  // namespace

// ================================================================================================================== Mixer
int Mixer::start(const Buf& b, bool loop, float gL, float gR, float cut, float rate, float smooth) {
    if (!b || b->empty()) return -1;
    for (int i = 0; i < MAX_VOICES; i++) {
        Voice& x = v[i];
        if (x.state.load(std::memory_order_acquire) != 0) continue;
        x.keep = b; x.data = b->data(); x.n = b->size(); x.loop = loop; x.smooth = smooth;
        x.pos = 0; x.lp = 0; x.cA = cut;
        x.cL = loop ? 0.0f : gL; x.cR = loop ? 0.0f : gR;               // loops fade in; one-shots start at their level
        x.gL.store(gL); x.gR.store(gR); x.cut.store(cut); x.rate.store(rate); x.stopReq.store(false);
        x.state.store(1, std::memory_order_release);
        return i;
    }
    return -1;
}
void Mixer::set(int id, float gL, float gR, float cut, float rate) {
    if (id < 0 || id >= MAX_VOICES) return;
    Voice& x = v[id];
    x.gL.store(gL); x.gR.store(gR); x.cut.store(cut);
    if (rate > 0) x.rate.store(rate);
}
void Mixer::stop(int id) { if (id >= 0 && id < MAX_VOICES) v[id].stopReq.store(true); }
int Mixer::activeVoices() const { int n = 0; for (const Voice& x : v) if (x.state.load(std::memory_order_acquire)) n++; return n; }

void Mixer::render(float* out, int frames) {
    std::fill(out, out + 2 * (size_t)frames, 0.0f);
    for (Voice& x : v) {
        if (x.state.load(std::memory_order_acquire) == 0) continue;
        bool stopping = x.stopReq.load();
        float tL = stopping ? 0.0f : x.gL.load(), tR = stopping ? 0.0f : x.gR.load(), tA = x.cut.load(), rate = std::max(0.05f, x.rate.load());
        bool finished = false;
        for (int i = 0; i < frames; i++) {
            size_t i0 = (size_t)x.pos;
            if (i0 >= x.n) { if (x.loop) { x.pos -= (double)x.n; i0 = (size_t)x.pos; } else { finished = true; break; } }
            size_t i1 = i0 + 1; if (i1 >= x.n) i1 = x.loop ? 0 : i0;
            float fr = (float)(x.pos - (double)i0);
            float s = x.data[i0] + (x.data[i1] - x.data[i0]) * fr;
            x.cA += (tA - x.cA) * 0.01f;
            x.lp += x.cA * (s - x.lp);
            x.cL += (tL - x.cL) * x.smooth; x.cR += (tR - x.cR) * x.smooth;
            out[2 * i] += x.lp * x.cL; out[2 * i + 1] += x.lp * x.cR;
            x.pos += rate;
        }
        if (stopping && x.cL < 1e-4f && x.cR < 1e-4f) finished = true;
        if (finished) { x.state.store(0, std::memory_order_release); }
    }
    float m = master.load();
    for (int i = 0; i < frames; i++) {
        masterNow += (m - masterNow) * 0.0008f;
        out[2 * i] = std::tanh(out[2 * i] * masterNow); out[2 * i + 1] = std::tanh(out[2 * i + 1] * masterNow);    // soft limiter: never clips
    }
}

// ================================================================================================================== device
struct Audio::Device { AudioStream stream{}; bool open = false; };
static Mixer* gMixer = nullptr;
static void audioCallback(void* buffer, unsigned int frames) {
    if (gMixer) gMixer->render((float*)buffer, (int)frames);
    else std::fill((float*)buffer, (float*)buffer + 2 * (size_t)frames, 0.0f);
}
bool Audio::openDevice() {
    if (!dev) dev = std::make_unique<Device>();
    if (dev->open) return true;
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return false;
    SetAudioStreamBufferSizeDefault(2048);
    dev->stream = LoadAudioStream(SR, 32, 2);                                  // 32 bit = floats, stereo
    gMixer = &mixer;
    SetAudioStreamCallback(dev->stream, audioCallback);
    PlayAudioStream(dev->stream);
    dev->open = true;
    return true;
}
void Audio::closeDevice() {
    if (builder.joinable()) builder.join();
    if (dev && dev->open) {
        gMixer = nullptr;
        StopAudioStream(dev->stream); UnloadAudioStream(dev->stream); CloseAudioDevice();
        dev->open = false;
    }
}

Audio::Audio() = default;
Audio::~Audio() { if (builder.joinable()) builder.join(); }
void Audio::buildAsync() { if (!builder.joinable() && !ready()) builder = std::thread([this] { sounds = generateSounds(); loadFiles(); built.store(true, std::memory_order_release); }); }
void Audio::loadFiles() { if (!useFiles) return; for (const std::string& l : loadSoundFiles(sounds, findAudioRoot())) TraceLog(LOG_INFO, "%s", l.c_str()); }
void Audio::buildNow() { if (!ready()) { sounds = generateSounds(); built.store(true, std::memory_order_release); } }
void Audio::toggleMute() { mute = !mute; mixer.setMaster(mute ? 0.0f : 0.9f); }

// ================================================================================================================== world knowledge
void Audio::startVoices() {
    const float slow = 0.00004f, mid = 0.0004f, quick = 0.002f;
    vo.rainWin = mixer.start(sounds.rainWindow, true, 0, 0, 1, 1, slow);
    vo.rainOpen = mixer.start(sounds.rainOpen, true, 0, 0, 1, 1, slow);
    vo.wind = mixer.start(sounds.wind, true, 0, 0, 1, 1, slow);
    vo.musicHall = mixer.start(sounds.musicHall, true, 0, 0, 1, 1, 0.00002f);
    vo.musicRoof = mixer.start(sounds.musicRoof, true, 0, 0, 1, 1, 0.00002f);
    vo.buzz = mixer.start(sounds.lampBuzz, true, 0, 0, 1, 1, mid);
    vo.purr = mixer.start(sounds.purr, true, 0, 0, 1, 1, mid);
    vo.lift = mixer.start(sounds.liftHum, true, 0, 0, 1, 1, mid);
    ambients.clear();
    for (const AmbientDef& d : ambientDefs()) {
        Buf b = sounds.byName(d.sound);
        if (!b) continue;
        Amb a; a.voice = mixer.start(b, true, 0, 0, 1, 1, quick); a.buf = b; a.x = a.y = 0; a.floor = d.floor; a.door = d.door; a.gain = d.gain; a.radius = d.radius; a.name = d.sound;
        ambients.push_back(a);
    }
    voicesStarted = true;
}

void Audio::learnWorld(const Building& b) {
    windows.assign((size_t)b.count(), {});
    doorPos.assign((size_t)b.count(), std::vector<std::pair<float, float>>(DOOR_KINDS, {-1.0f, -1.0f}));
    for (int f = 0; f < b.count(); f++) {
        const Level& L = b.level(f);
        for (int y = 0; y < L.cellsH; y++)
            for (int x = 0; x < L.cellsW; x++) {
                const Cell& c = L.cellAt(x, y);
                if (c.type == C_WINDOW) windows[(size_t)f].push_back({x + 0.5f, y + 0.5f});
                else if (c.type == C_DOOR && c.door < DOOR_KINDS && doorPos[(size_t)f][c.door].first < 0) {
                    // the sound of a door is placed just OUTSIDE it, in the hallway: the door itself then does not block the line
                    // to the sound (whether it is shut or open is handled separately, once)
                    DoorInfo di = L.doorInfoAt(x, y);
                    doorPos[(size_t)f][c.door] = {di.tcx + di.nx * 1.3f, di.tcy + di.ny * 1.3f};
                }
            }
    }
    for (Amb& a : ambients)
        if (a.floor >= 0 && a.floor < b.count() && a.door >= 0 && a.door < DOOR_KINDS) {
            auto p = doorPos[(size_t)a.floor][(size_t)a.door];
            a.x = p.first; a.y = p.second;
            if (p.first < 0) a.gain = 0;                                       // no such door: stay silent
        }
    prevTgt.assign((size_t)b.count(), std::vector<float>(DOOR_KINDS, 0.0f));
    worldLearned = true;
}

// How much wall is between two points on one floor: 0 = clear line, 1 = one wall or closed door, 3 = a lot. Open doors and rails are free.
float Audio::occlusion(const Building& b, int floor, float x0, float y0, float x1, float y1) const {
    const Level& L = b.level(std::clamp(floor, 0, b.count() - 1));
    float dx = x1 - x0, dy = y1 - y0, d = std::hypot(dx, dy);
    int steps = std::min(200, (int)(d / 0.35f) + 1), lx = -9999, ly = -9999;
    float occ = 0;
    for (int i = 1; i < steps; i++) {
        float t = (float)i / (float)steps;
        int cx = (int)std::floor(x0 + dx * t), cy = (int)std::floor(y0 + dy * t);
        if (cx == lx && cy == ly) continue;
        lx = cx; ly = cy;
        const Cell& c = L.cellAt(cx, cy);
        if (c.type == C_WALL) occ += 1.0f;
        else if (c.type == C_DOOR) { if (L.doorAmt[std::min<int>(c.door, DOOR_KINDS - 1)] < 0.5f) occ += 1.0f; }
        else if (c.type == C_WINDOW) occ += 0.5f;
        if (occ >= 3.0f) return 3.0f;
    }
    return occ;
}

// Loudness, side and muffling of a sound at (sx, sy): it fades with distance, walls make it quieter and duller, another floor
// more so, and it is placed left or right of where you are looking.
Audio::Spatial Audio::spatial(const AudioScene& s, float sx, float sy, int sfloor, float base, float radius, float cutFactor, float gainFactor) const {
    float dx = sx - s.px, dy = sy - s.py, d = std::hypot(dx, dy);
    float occ = sfloor == s.floor ? occlusion(*s.building, s.floor, s.px, s.py, sx, sy) : 0.0f;
    float att = base / (1.0f + (d / radius) * (d / radius));
    att *= std::pow(0.55f, occ) * gainFactor;
    float fc = std::clamp(9000.0f - 520.0f * d, 800.0f, 9000.0f) * std::pow(0.4f, occ) * cutFactor;
    if (sfloor != s.floor) { att *= 0.4f; fc *= 0.3f; }
    float rx = -std::sin(s.yaw), ry = std::cos(s.yaw);                       // screen-right
    float lateral = (dx * rx + dy * ry) / std::max(d, 0.4f);
    float pan = std::clamp(lateral, -1.0f, 1.0f) * 0.85f * std::min(1.0f, d / 1.2f);   // right next to you = centred
    float ang = (pan + 1.0f) * 0.78539816f;
    return {std::cos(ang) * att, std::sin(ang) * att, alphaFor(std::max(fc, 200.0f)), d, occ};
}
void Audio::shotAt(const AudioScene& s, const Buf& b, float x, float y, int floor, float base, float radius, float rate, float cutFactor) {
    Spatial sp = spatial(s, x, y, floor, base, radius, cutFactor);
    if (sp.gL + sp.gR < 0.004f) return;
    mixer.start(b, false, sp.gL, sp.gR, sp.alpha, rate, 0.01f);
}
void Audio::shotFlat(const AudioScene&, const Buf& b, float gain, float pan, float cut, float rate) {
    float ang = (std::clamp(pan, -1.0f, 1.0f) + 1.0f) * 0.78539816f;
    mixer.start(b, false, std::cos(ang) * gain, std::sin(ang) * gain, cut, rate, 0.01f);
}
void Audio::footstep(const AudioScene& s, const Level& lvl, float x, float y, int floor, bool isListener, bool cat) {
    const Cell& c = lvl.cellAt((int)std::floor(x), (int)std::floor(y));
    bool wet = lvl.outdoor && c.C >= SKY_H - 1.0f;
    const std::vector<Buf>& set = wet ? sounds.footWet : c.stair ? sounds.footStairs : sounds.footTile;
    const Buf& b = set[(size_t)pick((int)set.size())];
    float rate = randf(0.94f, 1.06f) * (cat ? 1.55f : 1.0f);
    if (isListener) {
        stepLeft = !stepLeft;
        float g = (wet ? 0.55f : 0.5f) * (s.mode == AM_TALK ? 0.8f : 1.0f);
        shotFlat(s, b, g, stepLeft ? -0.12f : 0.12f, 1.0f, rate);
        dbg.footsteps++; dbg.lastFootGain = g; dbg.lastFootCut = 1.0f;
    } else {
        float dxy = std::hypot(x - s.px, y - s.py);
        if (floor != s.floor && dxy > 10.0f) return;                        // too far above or below to hear
        Spatial sp = spatial(s, x, y, floor, cat ? 0.28f : 0.62f, 3.0f);
        if (sp.gL + sp.gR >= 0.004f) mixer.start(b, false, sp.gL, sp.gR, sp.alpha, rate, 0.01f);
        dbg.npcSteps++; dbg.lastFootGain = sp.gL + sp.gR; dbg.lastFootCut = sp.alpha;
    }
}

// ================================================================================================================== per frame
void Audio::update(const AudioScene& s) {
    if (!ready() || !s.building || s.building->count() == 0) return;
    const Building& B = *s.building;
    if (!voicesStarted) startVoices();
    if (!worldLearned) learnWorld(B);
    int fl = std::clamp(s.floor, 0, B.count() - 1);
    const Level& L = B.level(fl);

    // ---- where am I
    bool outdoors = L.outdoor && L.cellAt((int)std::floor(s.px), (int)std::floor(s.py)).C >= SKY_H - 1.0f;
    bool inHut = L.outdoor && !outdoors;
    bool peeping = s.mode == AM_PEEP, journal = s.mode == AM_JOURNAL, riding = s.mode == AM_RIDE;
    float world = journal ? 0.55f : s.mode == AM_TALK ? 0.85f : 1.0f;          // the world gets quieter while you write or talk

    if (firstFrame) {                                                          // remember how things are now, so nothing "just happened"
        prevMode = s.mode; prevTab = s.tab; prevSpread = s.spread; prevBoltT = s.boltT; prevToast = s.toastT; prevFloor = fl; prevPx = s.px; prevPy = s.py;
        for (int f = 0; f < B.count(); f++) for (int d = 0; d < DOOR_KINDS; d++) prevTgt[(size_t)f][(size_t)d] = B.level(f).doorTgt[d];
        firstFrame = false;
    }
    if (!scheduled) { sched.schedule(s.time + 14.0, EV_MEOW); sched.schedule(s.time + 50.0, EV_SLAM); sched.schedule(s.time + 75.0, EV_PIPE); scheduled = true; }

    // ---- rain: louder the closer you are to a window
    float prox = 0, panAcc = 0, panW = 0;
    float rx = -std::sin(s.yaw), ry = std::cos(s.yaw);
    if (!L.outdoor)
        for (const auto& w : windows[(size_t)fl]) {
            float dx = w.first - s.px, dy = w.second - s.py, d = std::hypot(dx, dy), k = 1.0f / (1.0f + (d / 2.2f) * (d / 2.2f));
            prox = std::max(prox, k);                                            // the NEAREST window decides how loud (many windows in a ring must not add up)
            panAcc += k * k * std::clamp((dx * rx + dy * ry) / std::max(d, 0.4f), -1.0f, 1.0f); panW += k * k;
        }
    float panR = panW > 0.001f ? std::clamp(panAcc / panW, -1.0f, 1.0f) * 0.6f : 0.0f;
    float rw = L.outdoor ? 0.0f : 0.06f + 0.62f * prox;
    if (peeping) rw = std::max(rw, 0.32f);                                      // the window of the room you are looking into
    float ro = outdoors ? 0.85f : inHut ? 0.22f : 0.0f, wd = outdoors ? 0.55f : inHut ? 0.18f : 0.03f;
    float aWin = alphaFor(peeping ? 2500.0f : 7000.0f), aHut = alphaFor(inHut ? 800.0f : 9000.0f);

    auto stereo = [](float g, float pan, float& l, float& r) { float ang = (std::clamp(pan, -1.0f, 1.0f) + 1.0f) * 0.78539816f; l = std::cos(ang) * g; r = std::sin(ang) * g; };
    float l, r;
    stereo(rw * world, panR, l, r);            mixer.set(vo.rainWin, l, r, aWin);
    stereo(ro * world, 0.0f, l, r);            mixer.set(vo.rainOpen, l, r, aHut);
    stereo(wd * world, 0.0f, l, r);            mixer.set(vo.wind, l, r, alphaFor(inHut ? 600.0f : 4000.0f));
    dbg.rainWindow = rw * world; dbg.rainOpen = ro * world; dbg.wind = wd * world;

    // ---- music: the building has one pad, the roof another; quieter when you look through a peephole, a little up while you write
    float mScale = peeping ? 0.5f : journal ? 0.9f : s.mode == AM_TALK ? 0.8f : s.mode == AM_END ? 1.2f : 1.0f;
    float mh = L.outdoor ? 0.0f : 0.20f * mScale, mr = L.outdoor ? 0.22f * mScale : 0.0f, aMus = alphaFor(peeping ? 900.0f : 12000.0f);
    stereo(mh, 0, l, r); mixer.set(vo.musicHall, l, r, aMus);
    stereo(mr, 0, l, r); mixer.set(vo.musicRoof, l, r, aMus);
    dbg.musicHall = mh; dbg.musicRoof = mr;

    // ---- the bad bulb nearest to you (buzzing or dying); it dips with the flicker
    {
        const Lamp* bestLamp = nullptr; float best = 0;
        for (const Lamp& lp : L.lamps) {
            if (lp.win || (lp.mode != 1 && lp.mode != 2)) continue;
            float d = std::hypot(lp.x - s.px, lp.y - s.py);
            float g = (lp.mode == 1 ? 0.30f : 0.18f) / (1.0f + (d / 2.4f) * (d / 2.4f));
            if (g > best) { best = g; bestLamp = &lp; }
        }
        if (bestLamp && !peeping) {
            float base = (bestLamp->mode == 1 ? 0.30f : 0.18f) * (0.4f + 0.6f * std::min(1.0f, bestLamp->k / 0.9f));
            Spatial sp = spatial(s, bestLamp->x, bestLamp->y, fl, base, 2.4f);
            mixer.set(vo.buzz, sp.gL * world, sp.gR * world, sp.alpha);
            dbg.buzz = (sp.gL + sp.gR) * 0.5f * world;
        } else { mixer.set(vo.buzz, 0, 0, 1); dbg.buzz = 0; }
    }

    // ---- the lift: a hum at the cabin, loud while you ride in it
    if (riding) { mixer.set(vo.lift, 0.5f, 0.5f, alphaFor(2500.0f)); dbg.lift = 0.5f; }
    else if (L.hasElevator() && !peeping) {
        auto c = L.elevatorDoorCentre(); Spatial sp = spatial(s, c.x, c.y, fl, 0.26f, 3.0f);
        mixer.set(vo.lift, sp.gL * world, sp.gR * world, sp.alpha); dbg.lift = (sp.gL + sp.gR) * 0.5f * world;
    } else { mixer.set(vo.lift, 0, 0, 1); dbg.lift = 0; }

    // ---- Boris: purring while you are near
    {
        const AudioMover* cat = nullptr; for (const AudioMover& m : s.movers) if (m.isCat) cat = &m;
        if (cat && cat->floor == fl && !peeping) { Spatial sp = spatial(s, cat->x, cat->y, fl, 0.55f, 2.0f); mixer.set(vo.purr, sp.gL * world, sp.gR * world, sp.alpha); dbg.purr = (sp.gL + sp.gR) * 0.5f * world; }
        else { mixer.set(vo.purr, 0, 0, 1); dbg.purr = 0; }
    }

    // ---- things behind doors: muffled while the door is shut, clear when it opens, clearest through the peephole
    for (Amb& a : ambients) {
        float gL = 0, gR = 0, alpha = 1;
        if (a.gain > 0) {
            bool inside = peeping && s.peepFloor == a.floor && s.peepDoor == a.door;
            if (inside) { stereo(a.gain * 1.6f, 0.0f, gL, gR); alpha = alphaFor(7000.0f); }
            else {
                const Level& lv = B.level(std::clamp(a.floor, 0, B.count() - 1));
                bool open = lv.doorAmt[a.door] > 0.5f;
                Spatial sp = spatial(s, a.x, a.y, a.floor, a.gain, a.radius, open ? 1.0f : 0.40f, (open ? 1.0f : 0.7f) * world);
                gL = sp.gL; gR = sp.gR; alpha = sp.alpha;
            }
        }
        mixer.set(a.voice, gL, gR, alpha);
        dbg.ambient[a.name] = (gL + gR) * 0.5f;
    }

    // ---- footsteps: yours, then everyone else's (they fade in and out as they pass; steps overhead are dull thuds)
    if (s.mode == AM_WALK && prevFloor == fl) {
        float d = std::hypot(s.px - prevPx, s.py - prevPy);
        if (d < 1.5f) { stepAcc += d; if (stepAcc >= 1.35f) { stepAcc -= 1.35f; footstep(s, L, s.px, s.py, fl, true, false); } }
    }
    for (auto& w : walkers) w.second.seen = false;
    for (const AudioMover& m : s.movers) {
        auto it = walkers.find(m.key);
        if (it == walkers.end()) { walkers[m.key] = {m.x, m.y, 0, m.floor, true}; continue; }
        Walker& w = it->second; w.seen = true;
        float d = std::hypot(m.x - w.x, m.y - w.y);
        if (m.floor == w.floor && d < 1.5f && d / std::max(s.dt, 1e-4f) < 6.0f) {
            w.acc += d;
            float stride = m.isCat ? 0.6f : 1.15f;
            if (w.acc >= stride) { w.acc -= stride; footstep(s, B.level(std::clamp(m.floor, 0, B.count() - 1)), m.x, m.y, m.floor, false, m.isCat); }
        }
        w.x = m.x; w.y = m.y; w.floor = m.floor;
    }

    // ---- doors that started to open or close
    for (int f = 0; f < B.count(); f++) {
        const Level& lv = B.level(f);
        for (int d = 0; d < DOOR_KINDS; d++) {
            float tgt = lv.doorTgt[d], prev = prevTgt[(size_t)f][(size_t)d];
            bool opened = tgt > 0.5f && prev <= 0.5f, closed = tgt <= 0.5f && prev > 0.5f;
            prevTgt[(size_t)f][(size_t)d] = tgt;
            if (!opened && !closed) continue;
            auto p = doorPos[(size_t)f][(size_t)d];
            if (p.first < 0) continue;
            if (d == DOOR_ELEVATOR) {
                shotAt(s, sounds.doorSlide, p.first, p.second, f, 0.7f, 4.0f, 1.0f); dbg.slides++;
                if (opened && riding) { shotFlat(s, sounds.liftDing, 0.5f, 0.0f, 1.0f, 1.0f); dbg.dings++; }
            } else {
                const Buf& b = opened ? sounds.creakOpen[(size_t)pick(3)] : sounds.creakClose[(size_t)pick(3)];
                shotAt(s, b, p.first, p.second, f, 0.75f, 4.0f, randf(0.95f, 1.05f)); dbg.creaks++;
            }
        }
    }

    // ---- mode changes and other one-off moments
    if (s.mode == AM_KNOCK && prevMode != AM_KNOCK) { shotAt(s, sounds.knock, s.px + std::cos(s.yaw) * 1.4f, s.py + std::sin(s.yaw) * 1.4f, fl, 0.9f, 3.0f, 1.0f); dbg.knocks++; }
    if ((s.mode == AM_PEEP) != (prevMode == AM_PEEP)) { shotFlat(s, sounds.lens, 0.35f, 0.0f, 1.0f, s.mode == AM_PEEP ? 1.0f : 0.85f); dbg.lenses++; }
    if ((s.mode == AM_JOURNAL) != (prevMode == AM_JOURNAL) || (journal && (s.tab != prevTab || s.spread != prevSpread))) {
        shotFlat(s, sounds.pageTurn, 0.45f, randf(-0.2f, 0.2f), 1.0f, randf(0.95f, 1.08f)); dbg.pages++;
    }
    if (s.toastT > prevToast + 0.5f) { shotFlat(s, sounds.cluePing, 0.4f, 0.0f, 1.0f, 1.0f); dbg.pings++; }
    if (s.boltT >= 0 && prevBoltT < 0) {                                         // lightning: the thunder follows after a delay
        double delay = 0.3 + 2.6 * (double)randf(0, 1);
        sched.schedule(s.time + delay, EV_THUNDER, (float)pick(3), s.boltAz);
        dbg.lastThunderDelay = delay;
    }

    // ---- scheduled events: thunder, meows, slams, knocking in the pipes
    SchedEvent e;
    while (sched.popDue(s.time, e)) {
        if (e.type == EV_THUNDER) {
            float g = outdoors ? 1.0f : inHut ? 0.8f : 0.55f, cut = outdoors ? alphaFor(9000.0f) : inHut ? alphaFor(1800.0f) : alphaFor(750.0f);
            shotFlat(s, sounds.thunder[(size_t)std::clamp((int)e.a, 0, 2)], g * (journal ? 0.7f : 1.0f), std::sin(e.b - s.yaw) * 0.6f, cut, 1.0f);
            dbg.thunders++;
        } else if (e.type == EV_MEOW) {
            for (const AudioMover& m : s.movers)
                if (m.isCat && m.floor == fl && std::hypot(m.x - s.px, m.y - s.py) < 14.0f) { shotAt(s, sounds.meow[(size_t)pick(3)], m.x, m.y, m.floor, 0.8f, 3.5f, randf(0.95f, 1.05f)); dbg.meows++; }
            sched.schedule(s.time + randf(18.0f, 45.0f), EV_MEOW);
        } else if (e.type == EV_SLAM) {
            if (!outdoors) { shotFlat(s, sounds.slam, 0.22f * world, randf(-0.8f, 0.8f), alphaFor(1400.0f), randf(0.85f, 1.05f)); dbg.slams++; }
            sched.schedule(s.time + randf(40.0f, 90.0f), EV_SLAM);
        } else if (e.type == EV_PIPE) {
            if (!outdoors) { shotFlat(s, sounds.pipeKnock, 0.18f * world, randf(-0.8f, 0.8f), alphaFor(2200.0f), 1.0f); dbg.pipes++; }
            sched.schedule(s.time + randf(55.0f, 130.0f), EV_PIPE);
        }
    }

    prevMode = s.mode; prevTab = s.tab; prevSpread = s.spread; prevBoltT = s.boltT; prevToast = s.toastT; prevFloor = fl; prevPx = s.px; prevPy = s.py;
}
