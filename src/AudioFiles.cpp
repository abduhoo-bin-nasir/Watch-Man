// AudioFiles.cpp - loads wav / mp3 / ogg / flac files over the generated sounds. See AudioFiles.h.
#include "AudioFiles.h"
#include <cmath>
#include <cstdio>
#include <raylib.h>

std::string soundFolder(const std::string& n) {
    static const char* ambience[] = {"rainWindow", "rainOpen", "wind", "lampBuzz", "liftHum", "purr"};
    static const char* music[] = {"musicHall", "musicRoof"};
    static const char* rooms[] = {"tvMatch", "radioChess", "piano", "opera", "keyboard", "fan", "crtHum"};
    static const char* events[] = {"knock", "latch", "liftDing", "doorSlide", "pageTurn", "cluePing", "pipeKnock", "slam", "lens"};
    for (const char* x : ambience) if (n == x) return "ambience";
    for (const char* x : music) if (n == x) return "music";
    for (const char* x : rooms) if (n == x) return "rooms";
    for (const char* x : events) if (n == x) return "events";
    auto starts = [&](const char* p) { return n.rfind(p, 0) == 0; };
    if (starts("footTile") || starts("footStairs") || starts("footWet")) return "footsteps";
    if (starts("thunder") || starts("creakOpen") || starts("creakClose") || starts("meow")) return "events";
    return "";
}

std::string findSoundFile(const std::string& root, const std::string& name) {
    std::string folder = soundFolder(name);
    if (folder.empty()) return "";
    static const char* exts[] = {".wav", ".mp3", ".ogg", ".flac"};
    for (const char* e : exts) {
        std::string p = root + "/" + folder + "/" + name + e;
        if (FileExists(p.c_str())) return p;
    }
    return "";
}

std::string findAudioRoot() {
    std::string app = GetApplicationDirectory();
    const std::string cands[] = {"assets/audio", app + "assets/audio", app + "../assets/audio", app + "../../assets/audio"};
    for (const std::string& c : cands) if (DirectoryExists(c.c_str())) return c;
    return "";
}

namespace {
Buf* slot(Sounds& s, const std::string& n, bool& found) {
    found = true;
    static Buf dummy;
    struct One { const char* name; Buf Sounds::*p; };
    static const One one[] = {
        {"rainWindow", &Sounds::rainWindow}, {"rainOpen", &Sounds::rainOpen}, {"wind", &Sounds::wind}, {"musicHall", &Sounds::musicHall},
        {"musicRoof", &Sounds::musicRoof}, {"lampBuzz", &Sounds::lampBuzz}, {"liftHum", &Sounds::liftHum}, {"purr", &Sounds::purr},
        {"tvMatch", &Sounds::tvMatch}, {"radioChess", &Sounds::radioChess}, {"piano", &Sounds::piano}, {"opera", &Sounds::opera},
        {"keyboard", &Sounds::keyboard}, {"fan", &Sounds::fan}, {"crtHum", &Sounds::crtHum}, {"knock", &Sounds::knock},
        {"latch", &Sounds::latch}, {"liftDing", &Sounds::liftDing}, {"doorSlide", &Sounds::doorSlide}, {"pageTurn", &Sounds::pageTurn},
        {"cluePing", &Sounds::cluePing}, {"pipeKnock", &Sounds::pipeKnock}, {"slam", &Sounds::slam}, {"lens", &Sounds::lens}};
    for (const One& o : one) if (n == o.name) return &(s.*(o.p));
    struct Many { const char* prefix; std::vector<Buf> Sounds::*p; };
    static const Many many[] = {{"thunder", &Sounds::thunder}, {"creakOpen", &Sounds::creakOpen}, {"creakClose", &Sounds::creakClose},
                                {"meow", &Sounds::meow}, {"footTile", &Sounds::footTile}, {"footStairs", &Sounds::footStairs}, {"footWet", &Sounds::footWet}};
    for (const Many& m : many) {
        std::string pre = m.prefix;
        if (n.size() > pre.size() && n.compare(0, pre.size(), pre) == 0) {
            std::string num = n.substr(pre.size());
            if (num.find_first_not_of("0123456789") != std::string::npos) continue;
            size_t i = (size_t)std::stoul(num);
            std::vector<Buf>& v = s.*(m.p);
            if (i < v.size()) return &v[i];
        }
    }
    found = false;
    return &dummy;
}

// Decodes any file raylib knows into mono, 44100 Hz floats.
Buf decode(const std::string& path, std::string& err) {
    Wave w = LoadWave(path.c_str());
    if (!IsWaveValid(w)) { err = "could not decode"; return nullptr; }
    WaveFormat(&w, SR, 32, 1);                       // sample rate, 32-bit float, mono (stereo files are mixed down)
    if (w.frameCount == 0 || !w.data || w.sampleSize != 32 || w.channels != 1) { UnloadWave(w); err = "unsupported format"; return nullptr; }
    auto v = std::make_shared<std::vector<float>>((float*)w.data, (float*)w.data + w.frameCount);
    UnloadWave(w);
    for (float& x : *v) if (!std::isfinite(x)) x = 0.0f;
    return v;
}
}  // namespace

std::vector<std::string> loadSoundFiles(Sounds& s, const std::string& root) {
    std::vector<std::string> log;
    if (root.empty()) { log.push_back("audio: no assets/audio folder found - using the generated sounds"); return log; }
    for (const auto& p : allSoundsNamed(s)) {
        std::string path = findSoundFile(root, p.first);
        if (path.empty()) continue;
        bool found; Buf* dst = slot(s, p.first, found);
        if (!found) continue;
        std::string err;
        Buf b = decode(path, err);
        if (b) { *dst = b; log.push_back("audio: loaded " + path); }
        else log.push_back("audio: FAILED " + path + " (" + err + ") - using the generated sound");
    }
    return log;
}
