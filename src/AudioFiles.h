// AudioFiles.h - optional sound FILES that replace the built-in generated sounds.
// For every sound name (see assets/audio/README.txt) the game looks for  assets/audio/<folder>/<name>.wav|mp3|ogg|flac .
// A file that exists replaces the generated sound; a missing file leaves the generated one, so you can swap sounds one at a time.
#pragma once
#include "Synth.h"
#include <string>
#include <vector>

// Folder that holds the sounds of this name (e.g. "thunder0" -> "events"). Empty for an unknown name.
std::string soundFolder(const std::string& name);

// Finds <root>/<folder>/<name>.<ext> for the first extension that exists. Returns "" if there is none.
std::string findSoundFile(const std::string& root, const std::string& name);

// Looks for the assets/audio folder (current directory, then next to the executable and up to two levels above it).
std::string findAudioRoot();

// Replaces generated sounds by files found under `root`. Returns what it did, one line per file loaded or failed
// (the game prints these to the console). Safe to call from the background thread; needs no window and no sound card.
std::vector<std::string> loadSoundFiles(Sounds& s, const std::string& root);
