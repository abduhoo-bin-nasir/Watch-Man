// Dialogue.cpp - the conversation runtime (menu, typing effect, clue hand-out). The words are in DialogueData.cpp.
#include "Dialogue.h"
#include "ClueBook.h"
#include "Entities.h"

void Conversation::begin(Character& c, ClueBook& b, int nowMinute) {
    who = &c; book = &b; minute = nowMinute; confrontPending = false;
    speaker = c.name;
    text = (c.met && !c.script.again.empty()) ? c.script.again : c.script.greet;
    c.met = true; reveal = 0; sel = 0;
    rebuild();
}

void Conversation::rebuild() {
    menu.clear();
    if (!who) return;
    if (confrontPending) { menu.push_back({"(Continue)", -3, false}); sel = 0; return; }
    const vector<Topic>& ts = who->script.topics;
    for (size_t i = 0; i < ts.size(); i++) {
        const Topic& t = ts[i];
        bool ok = true;
        for (const string& r : t.req) if (!book->has(r)) ok = false;
        if (!ok) continue;
        if (t.confront) {
            if (book->count() == 0) continue;
            if (book->canConfront()) menu.push_back({"[CONFRONT] Lay out everything you have.", (int)i, false});
            else menu.push_back({"[CONFRONT] (not enough yet: I need 4+ clues that point at him)", -2, true});
            continue;
        }
        menu.push_back({t.ask, (int)i, t.done});
    }
    menu.push_back({"That's all. Thank you.", -1, false});
    sel = std::clamp(sel, 0, (int)menu.size() - 1);
}

Conversation::Result Conversation::choose(int idx) {
    if (idx < 0 || idx >= (int)menu.size() || !who) return R_NONE;
    int ti = menu[(size_t)idx].topic;
    if (ti == -1) return R_LEAVE;
    if (ti == -3) return R_ENDING;
    if (ti == -2) { speaker = "You"; text = "Not yet. I need more in the journal before I can pin this on him."; reveal = 0; return R_NONE; }
    Topic& t = who->script.topics[(size_t)ti];
    t.done = true; speaker = who->name; text = t.reply; reveal = 0;
    if (!t.learn.empty()) { who->name = t.learn; speaker = who->name; }
    if (!t.note.empty() && std::find(who->notes.begin(), who->notes.end(), t.note) == who->notes.end()) who->notes.push_back(t.note);
    if (!t.gives.empty()) book->give(t.gives, minute);
    if (t.confront) confrontPending = true;
    rebuild();
    return R_NONE;
}

Conversation::Result Conversation::update(float dt, const Input& in, int nowMinute) {
    minute = nowMinute;
    float total = (float)text.size();
    if (reveal < total) reveal += dt * 60.0f;
    int n = (int)menu.size();
    if (n == 0) return R_NONE;
    if (in.hover >= 0 && in.hover < n) sel = in.hover;
    if (in.up) sel = (sel + n - 1) % n;
    if (in.down) sel = (sel + 1) % n;
    int pick = -1;
    if (in.num >= 1 && in.num <= n) pick = in.num - 1;
    if (in.pick >= 0 && in.pick < n) pick = in.pick;
    if (in.enter || in.interact) { if (reveal < total) reveal = total; else pick = sel; }   // first press skips the typing
    if (in.esc && !confrontPending) return R_LEAVE;
    return pick >= 0 ? choose(pick) : R_NONE;
}
