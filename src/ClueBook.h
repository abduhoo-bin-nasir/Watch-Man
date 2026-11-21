// ClueBook.h - the detective's case: every clue is a node in a dependency graph (a DAG). A clue can only be found
// once the clues it depends on are already in the journal. The database is a hash map keyed by clue id; Kahn's
// algorithm gives the topological order and each clue's depth (the case board uses it for its layout).
#pragma once
#include "Common.h"

struct Clue {
    string id, title, text, source, hint;
    vector<string> req;               // clues that must be in the journal first
    bool autoFound = false;           // a deduction: appears by itself once all its requirements are in the journal
    bool found = false, fresh = false;
    int minute = 0;                   // when it was written down (minutes after midnight)
    int depth = 0;                    // longest chain of requirements above it
};

class ClueBook {
public:
    void loadCase();                                   // the clues themselves: ClueBook.cpp (edit them there)
    bool has(const string& id) const;
    bool available(const Clue& c) const;               // are all of its requirements found?
    void give(const string& id, int nowMinute);        // write a clue (and any deduction it unlocks) into the journal
    int count() const { return (int)found.size(); }
    bool canConfront() const;                          // enough on the scarf man to make an arrest?
    string takeToast();                                // "Journal updated: ..." once, then empty
    const Clue& clue(const string& id) const { return clues.at(id); }
    Clue& clue(const string& id) { return clues.at(id); }

    vector<string> ids;                                // declaration order (stable for layout)
    vector<string> found;                              // ids in the order they were written down
    vector<string> topo;                               // topological order of the whole graph
    static constexpr int CONFRONT_CLUES = 4;           // clues needed before you can confront him

private:
    void add(const char* id, const char* title, const char* text, const char* source, const char* hint, vector<string> req = {}, bool autoF = false);
    void buildTopo();
    std::unordered_map<string, Clue> clues;            // the clue database (hash map)
    string toast;
};
