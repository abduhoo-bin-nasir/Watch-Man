// export_sounds - writes every generated sound as a 16-bit mono 44.1 kHz WAV into  <out>/<folder>/<name>.wav
// Usage:  export_sounds assets/audio        (does not overwrite existing files unless you pass --force)
// Needs no window and no sound card. The game then loads these files instead of generating the sounds.
#include "AudioFiles.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

static void writeWav(const std::string& path, const std::vector<float>& d) {
    std::ofstream f(path, std::ios::binary);
    auto u32 = [&](uint32_t v) { f.write((const char*)&v, 4); };
    auto u16 = [&](uint16_t v) { f.write((const char*)&v, 2); };
    uint32_t bytes = (uint32_t)d.size() * 2;
    f.write("RIFF", 4); u32(36 + bytes); f.write("WAVEfmt ", 8); u32(16); u16(1); u16(1); u32(SR); u32(SR * 2); u16(2); u16(16);
    f.write("data", 4); u32(bytes);
    for (float x : d) { int16_t s = (int16_t)(std::clamp(x, -1.0f, 1.0f) * 32767.0f); f.write((const char*)&s, 2); }
}

int main(int argc, char** argv) {
    std::string out = argc > 1 ? argv[1] : "assets/audio";
    bool force = argc > 2 && !std::strcmp(argv[2], "--force");
    Sounds s = generateSounds();
    int written = 0, skipped = 0;
    for (const auto& p : allSoundsNamed(s)) {
        std::string dir = out + "/" + soundFolder(p.first);
        std::filesystem::create_directories(dir);
        std::string path = dir + "/" + p.first + ".wav";
        if (!force && std::filesystem::exists(path)) { skipped++; continue; }
        writeWav(path, *p.second); written++;
    }
    std::printf("export_sounds: wrote %d files, skipped %d existing, into %s\n", written, skipped, out.c_str());
}
