// DialogueData.cpp - EVERYTHING PEOPLE SAY. Edit this file to change the dialogue.
//
// Each character has a Script (see Dialogue.h):
//     greet        first thing they say when you meet     again        what they say when you come back
//     openLine     narration while they open their door   impression   first line about them in your journal
//     topics       the questions you can ask them
// A topic is  T(question, answer, clue it gives, {clues you need first}, journal note, real name learned)
//     - the clue id must exist in ClueBook.cpp ("" = gives nothing)
//     - a topic only appears in the menu once every clue in its {requirements} is in your journal
// The character ids ("volkova", "scarf", ...) match the cast list in CharacterArt.cpp.
#include "Dialogue.h"

namespace {
Topic T(const char* ask, const char* reply, const char* gives = "", vector<string> req = {}, const char* note = "", const char* learn = "") {
    Topic t; t.ask = ask; t.reply = reply; t.gives = gives; t.req = req; t.note = note; t.learn = learn;
    return t;
}
}

Script scriptFor(const string& id) {
    Script s;
    if (id == "volkova") {
        s.greet = "Who is it? ... Oh. Police. Make it quick, the kettle is on.";
        s.again = "You again. The kettle is cold now, thank you very much.";
        s.openLine = "A chain slides. The door opens a hand's width, then wider.";
        s.impression = "Misses nothing through that chain.";
        s.topics = {T("Did you hear anything last night?", "Shouting. Two a.m., give or take. From 106. A man's voice, deep as a well. And Anton shouted back. Anton never shouts.", "argue"),
                    T("What was Anton like?", "Quiet. Cards on Fridays. Paid his debts on time... until last month.", "", {}, "Anton: quiet, cards on Fridays."),
                    T("Who was he arguing with?", "I did not see. I keep the chain on. But Anton owed money, and to someone who frightened him.", "debt", {"argue"}),
                    T("Anyone else I should talk to?", "The boy in 104. Never sleeps, that one. Sits at his window like a little owl. Ask him.", "", {"argue"}, "Says: ask the boy in 104.")};
    } else if (id == "gromov") {
        s.greet = "Mm? The match is on. Make it fast.";
        s.again = "The match is still on, officer.";
        s.openLine = "A TV roars, then drops to a mutter. The door opens.";
        s.impression = "Eyes on the match. No use.";
        s.topics = {T("Hear anything last night?", "Headphones. Always headphones. It could have been the Last Judgement, I would not know."),
                    T("Did you know Anton?", "Borrowed my ladder once. Never returned it.", "", {}, "Anton never returned his ladder.")};
    } else if (id == "misha") {
        s.greet = "Mama's asleep. ... Are you really a detective?";
        s.again = "You came back! Did you catch someone?";
        s.openLine = "Small feet. A chain rattles. The door opens just enough.";
        s.impression = "Ten, maybe. Sharp. Sits up all night.";
        s.topics = {T("Shouldn't you be in bed?", "Mama thinks I sleep. I do my best work at night.", "", {}, "Does his best work at night."),
                    T("Did you see anyone near 106 last night?", "A big man. Red scarf, like a fireman. He knocked and Anton let him in. Then the shouting started. I put my head under the pillow.", "scarf", {"argue"}),
                    T("Would you know him again?", "He walks the hallway at night. Round and round, like he is looking for something.", "", {"scarf"}, "The scarf man walks the hallway ring at night.")};
    } else if (id == "petrova") {
        s.greet = "Yes? I have cabbage on the stove.";
        s.again = "Still cabbage.";
        s.openLine = "The smell arrives first. Then the door.";
        s.impression = "Wants her door shut.";
        s.topics = {T("Did you hear anything last night?", "I keep my door shut and my nose out of other people's business."),
                    T("Know the man in 106?", "I know his rubbish. Bottles. So many bottles.", "", {}, "Anton's bin: bottles.")};
    } else if (id == "scarf") {
        s.greet = "Detective? I was home all night.";
        s.impression = "Big. Calm. Too calm.";
        s.topics = {T("Who are you?", "Igor Sidorov. Apartment 203. I am taking some air. Is that a crime now?", "", {}, "Says he lives in 203.", "Igor Sidorov"),
                    T("Where were you last night?", "Home. All night. Ask anyone. Nobody will say otherwise, because nobody was there.", "", {}, "Home all night. Alone. Convenient."),
                    T("Neighbours heard an argument from 106.", "People hear what they want to hear. I hardly knew the man.", "", {"argue"}, "'Hardly knew' Anton."),
                    T("What size are your boots?", "Forty-four. So what? Half the block is forty-four.", "", {"boot"}, "Boots: size 44.")};
        Topic c; c.ask = "CONFRONT"; c.confront = true;
        c.reply = "You lay it out slowly: the argument at two, the debt, the scarf at the boy's window, the boot print, the wool on the cat, the stairs at ten past two. "
                  "Igor's jaw works. He looks at his boots, then at you. 'Anton laughed at me,' he says. 'Fourteen thousand, and he laughed.' He holds out his wrists.";
        s.topics.push_back(c);
    } else if (id == "cat") {
        s.greet = "Boris looks up at you with flat yellow eyes.";
        s.impression = "Sleeps by 106. Hisses at everyone.";
        s.topics = {T("Check his collar", "Boris hisses, then allows it. A red wool thread is snagged on the buckle. Fresh. He has been sleeping by 106.", "wool"),
                    T("Scratch behind his ears", "Boris tolerates it for exactly three seconds.")};
    } else if (id == "pavel") {
        s.greet = "Nobody comes up here after ten. Mind the stairs.";
        s.impression = "Sees everything, says little.";
        s.topics = {T("Anything odd at night?", "The light by the landing has been dying for weeks. Other than that, quiet. Mostly.", "", {}, "Landing light: dying for weeks."),
                    T("A big man in a red scarf. Seen him?", "On the stairs. Ten past two, near enough. Heading down fast, boots caked in mud. I do not ask questions at that hour.", "stairs", {"scarf"})};
    } else if (id == "tenant") {
        s.greet = "Is that the police? About time.";
        s.impression = "Has opinions about the lift.";
        s.topics = {T("Everything all right up here?", "The lift is switched off at ten every night. The caretaker's economy drive. After that we all climb the stairs like goats.", "elev")};
    }
    return s;
}

// What you hear when you knock and nobody opens, by door number (1..8). Edit freely.
const char* noAnswerLine(int /*floor*/, int door) {
    static const char* L[8] = {"A chain rattles. No one answers.", "A TV drones behind the door.", "A tired voice: \"Night shift. Not home.\"",
                               "A boy through the gap: \"Big guy came by. Red scarf.\"", "Mail piles up on the mat.",
                               "Crime scene tape across the frame. A muddy boot print by the hinge.", "Boiled cabbage smell. The door stays shut.",
                               "A radio plays softly. No answer."};
    return L[std::clamp(door, 1, 8) - 1];
}
