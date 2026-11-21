// Dialogue.h - what people say and how a conversation works.
//   * Topic / Script: plain data. One Script per character, written in DialogueData.cpp (that is the file to edit).
//   * Conversation: the runtime. It builds the question menu from the topics you may ask, types out the answers,
//     writes clues into the journal and reports when you leave or make an arrest.
#pragma once
#include "Common.h"

class Character;
class ClueBook;

struct Topic {
    string ask, reply;       // the question in the menu, and the answer
    string gives;            // clue id written into the journal when asked ("" = none)
    string learn;            // the person's real name, learned when asked ("" = none)
    string note;             // a line scribbled on the People page ("" = none)
    vector<string> req;      // clues that must already be in the journal before this question is offered
    bool done = false;       // already asked
    bool confront = false;   // the final accusation (needs ClueBook::canConfront)
};

struct Script {
    string greet, again;     // first meeting / meeting again
    string openLine;         // narration while a resident opens the door
    string impression;       // the first line about them on the People page
    vector<Topic> topics;
};

Script scriptFor(const string& characterId);              // DialogueData.cpp
const char* noAnswerLine(int floor, int door);            // what you hear at a door nobody opens (DialogueData.cpp)

struct MenuItem { string label; int topic; bool dim; };   // topic: >= 0 a topic, -1 leave, -2 locked confront, -3 continue

class Conversation {
public:
    enum Result { R_NONE, R_LEAVE, R_ENDING };
    void begin(Character& who, ClueBook& book, int nowMinute);
    Result update(float dt, const Input& in, int nowMinute);

    // state the UI draws
    string speaker, text;
    float reveal = 0;                  // characters of `text` typed out so far
    vector<MenuItem> menu;
    int sel = 0;
    bool confrontPending = false;
    Character* who = nullptr;

private:
    void rebuild();
    Result choose(int idx);
    ClueBook* book = nullptr;
    int minute = 0;
};
