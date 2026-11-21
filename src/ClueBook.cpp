// ClueBook.cpp - the clue database (edit the wording here) and the dependency-graph logic.
#include "ClueBook.h"

void ClueBook::add(const char* id, const char* title, const char* text, const char* source, const char* hint, vector<string> req, bool autoF) {
    Clue c; c.id = id; c.title = title; c.text = text; c.source = source; c.hint = hint; c.req = req; c.autoFound = autoF;
    clues[id] = c; ids.push_back(id);
}

// ======================================================================= THE CASE: EDIT THE CLUES HERE
// add(id, title, diary text, where it came from, hint shown on the board, {clues that must come first}, isDeduction)
// Dialogue lines that GIVE a clue refer to it by id (see DialogueData.cpp).
void ClueBook::loadCase() {
    clues.clear(); ids.clear(); found.clear(); topo.clear();
    add("argue", "THE ARGUMENT", "Raised voices from 106 around 2 a.m. A man's voice, deep. Anton shouted back - and Anton never shouts.",
        "Mrs. Volkova, apt 101", "Somebody on this floor must have heard something. Start with the nosy one.");
    add("debt", "THE DEBT", "Anton owed money to someone he was afraid of. Paid on time for years, then stopped last month.",
        "Mrs. Volkova, apt 101", "Mrs. Volkova knows more. Ask who Anton was arguing with.", {"argue"});
    add("scarf", "RED SCARF", "The boy in 104 watched a big man in a red scarf knock at 106. Anton let him in. Then the shouting.",
        "Misha, apt 104", "Mrs. Volkova says the boy in 104 never sleeps.", {"argue"});
    add("boot", "BOOT PRINT", "Fresh muddy print by the hinge of 106. Size 44, heavy tread. Someone left in a hurry.",
        "Door of apt 106", "The sealed door itself. Look closer.");
    add("wool", "RED WOOL", "Boris the cat has a red wool thread on his collar. He sleeps by 106. Whoever called there wore wool.",
        "Boris, ground-floor hallway", "The cat roams the ground-floor hallway. See what he is wearing.");
    add("elev", "LIFT OFF AT NIGHT", "The lift is cut off at ten every night. At 2 a.m. the stairs were the only way out of the block.",
        "Old tenant, second floor", "Someone upstairs has complaints about the lift.");
    add("stairs", "ON THE STAIRS", "Pavel saw the red-scarf man on the stairs at about 02:10. Heading down fast, boots caked in mud.",
        "Pavel, janitor, second floor", "Whoever left 106 had to use the stairs. The janitor works upstairs.", {"scarf"});
    add("match", "THE PICTURE", "Size-44 boots. Red wool. A big man in a red scarf. Three loose threads, one man. He walks the hallway like he is waiting for me.",
        "Deduction", "Boot, wool, scarf. Lay them side by side.", {"boot", "wool", "scarf"}, true);
    buildTopo();
}

bool ClueBook::has(const string& id) const { auto it = clues.find(id); return it != clues.end() && it->second.found; }
bool ClueBook::available(const Clue& c) const { for (const string& r : c.req) if (!has(r)) return false; return true; }
bool ClueBook::canConfront() const { return count() >= CONFRONT_CLUES && has("scarf"); }
string ClueBook::takeToast() { string t; t.swap(toast); return t; }

void ClueBook::give(const string& id, int nowMinute) {
    auto it = clues.find(id);
    if (it == clues.end() || it->second.found) return;
    Clue& c = it->second;
    c.found = true; c.fresh = true; c.minute = nowMinute;
    found.push_back(id);
    toast = "Journal updated: " + c.title;
    for (bool again = true; again;) {              // deductions that have just become possible
        again = false;
        for (const string& oid : ids) {
            Clue& d = clues[oid];
            if (d.found || !d.autoFound || !available(d)) continue;
            d.found = true; d.fresh = true; d.minute = nowMinute; found.push_back(oid);
            toast = "Journal updated: " + d.title; again = true;
        }
    }
}

void ClueBook::buildTopo() {                       // Kahn's algorithm
    std::unordered_map<string, int> indeg;
    std::unordered_map<string, vector<string>> kids;
    for (const string& id : ids) { indeg[id] = (int)clues[id].req.size(); for (const string& r : clues[id].req) kids[r].push_back(id); }
    std::deque<string> q;
    for (const string& id : ids) if (indeg[id] == 0) q.push_back(id);
    topo.clear();
    while (!q.empty()) {
        string u = q.front(); q.pop_front(); topo.push_back(u);
        for (const string& v : kids[u]) {
            clues[v].depth = std::max(clues[v].depth, clues[u].depth + 1);
            if (--indeg[v] == 0) q.push_back(v);
        }
    }
}
