// Synth.h - every sound in the game is generated here, in code, at start-up. No audio files, no licences.
// All sounds are mono, 44100 Hz, floats in -1..1. Loops are made seamless (the end flows into the start).
#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

constexpr int SR = 44100;
using Buf = std::shared_ptr<const std::vector<float>>;

struct Sounds {
    // beds (loops)
    Buf rainWindow;      // rain heard through glass: soft, with taps on the pane
    Buf rainOpen;        // rain outdoors on the roof: bright, with puddle plinks
    Buf wind;
    Buf musicHall;       // ambient pad for the building
    Buf musicRoof;       // sparser, colder, for the roof
    Buf lampBuzz;        // a bad fluorescent tube
    Buf liftHum;
    Buf purr;
    // things behind doors (loops)
    Buf tvMatch, radioChess, piano, opera, keyboard, fan, crtHum;
    // one-shots
    std::vector<Buf> thunder;          // 3 variants
    std::vector<Buf> creakOpen, creakClose;   // 3 variants each
    std::vector<Buf> footTile, footStairs, footWet;   // 4 variants each
    std::vector<Buf> meow;             // 3 variants
    Buf knock, latch, liftDing, doorSlide, pageTurn, cluePing, pipeKnock, slam, lens;

    Buf byName(const std::string& name) const;     // for the data file: "tvMatch", "piano", ...
};

Sounds generateSounds();                           // takes a moment (well under a second on a laptop)
std::vector<std::pair<std::string, Buf>> allSoundsNamed(const Sounds& s);   // for tests and the WAV preview
bool isLoopSound(const std::string& name);
