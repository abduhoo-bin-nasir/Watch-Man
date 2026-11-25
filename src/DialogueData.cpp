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
    } else if (id == "fedorov") {
        s.greet = "State your business. Briefly. I am mid-game and losing to a radio.";
        s.again = "Still losing, Detective.";
        s.openLine = "A chessboard clacks shut. The door opens in two stages.";
        s.impression = "Fights a radio at chess. The radio is winning.";
        s.topics = {T("Who are you playing against?", "The radio. It announces the moves. Badly. Some nights it cheats and I say nothing. A good officer knows which battles to lose.", "", {}, "Plays chess against the radio."),
                    T("Hear anything last night?", "Two in the morning, a disturbance down the hall. I assumed domestic. In my day we called that Tuesday.", "", {}, "Heard 'a disturbance' at 2 a.m. Shrugged."),
                    T("What do you make of the new watches?", "A machine on the wrist that talks back? I served forty years. I know insubordination when I hear it.", "", {}, "Calls the AI watch insubordinate."),
                    T("Any advice for a detective?", "Control the centre. Sacrifice nothing you cannot afford. And never play on another man's board.")};
    } else if (id == "dasha") {
        s.greet = "One sec - no, a real cop, I'll call you back. Hi!";
        s.again = "Back already? Dramatic.";
        s.impression = "Teen. Permanently on a call that is not real.";
        s.topics = {T("Who are you talking to?", "Nobody. It is a recording. If I look busy nobody asks me to take out the rubbish.", "", {}, "Fake phone calls to dodge chores."),
                    T("See anything last night?", "I sleep with noise-cancelling on. Block 7 is much easier to live in at minus forty decibels.", "", {}, "Noise-cancelling at night. Useless witness."),
                    T("What do you think of the watch AI?", "Mine tells me to drink water. Yours swears at you? Honestly, relatable.")};
    } else if (id == "lev") {
        s.greet = "Eleven thousand, two hundred and six. ... No. Hush. You made me lose count.";
        s.again = "Where was I? Never mind. Forty million is a good round number.";
        s.impression = "Watches the rain. Keeps a tally.";
        s.topics = {T("What are you counting?", "Raindrops on this pane. Started in March. If I stop, who will know how many there were?", "", {}, "Counting raindrops on the north window."),
                    T("Do you see much at night?", "At night I count stars. There are none. Block 7 has a very short count.", "", {}, "Counts stars at night. Stars: zero."),
                    T("Does the window ever open?", "Sealed in 2026. The only honest thing in the building. It lets nothing in and nothing out.")};
    } else if (id == "tamara") {
        s.greet = "Darling! Finally, an audience.";
        s.again = "An encore! I knew it.";
        s.openLine = "A scale - do, re, mi - breaks off mid-note. The door opens with a flourish.";
        s.impression = "Retired opera singer. Everything is a stage.";
        s.topics = {T("Did you hear anything last night?", "I sang Tosca in the bath at midnight. Anyone screaming after that was a critic.", "", {}, "Sings in the bath at midnight."),
                    T("About the man in the red scarf...", "Igor? Heavy tread, light tipper. He once clapped at my Aida. Out of time, but he clapped.", "", {"scarf"}, "Igor once clapped at her Aida."),
                    T("How do you cope without the lift at night?", "It switches off at ten like a bad curtain call. I take the stairs. Dramatically.")};
    } else if (id == "kolya") {
        s.greet = "Chat, say hi to the police. ... No, you cannot be on stream, sorry officer.";
        s.again = "Chat says you came back. Chat is never wrong.";
        s.openLine = "Chair wheels. A ring light clicks off. The door opens.";
        s.impression = "Streams to an invisible crowd.";
        s.topics = {T("Were you streaming last night?", "Until four. Eleven viewers, one bot. Ten of them could vouch for me but they are in Brazil.", "", {}, "Streams until 4 a.m."),
                    T("Hear anything through the floor?", "Chat says there was a thump around two. I called it lag. Was it not lag? Should I be scared?", "", {}, "Heard a thump about 2 a.m. Called it lag."),
                    T("What do you stream?", "Walkthroughs of this building. 'Block 7, no commentary.' Surprisingly popular. Six viewers.")};
    } else if (id == "beketov") {
        s.greet = "Show me your hands. Now the wrist. ... That watch is listening.";
        s.again = "Still wearing it? Your funeral.";
        s.openLine = "Three locks, a pause, a fourth lock. The door opens on a chain.";
        s.impression = "Convinced every watch in Block 7 is listening.";
        s.topics = {T("Why do you distrust my watch?", "It has a microphone, a speaker, and opinions. Those are the three ingredients of a hostage taker.", "", {}, "Thinks the watch is a hostage taker."),
                    T("Did you see anything last night?", "I saw nothing. I put foil over the peephole. Peepholes are how they get your face.", "", {}, "Foil over the peephole. Saw nothing."),
                    T("Any advice for the case?", "Write everything on paper. Paper has never been hacked. Paper once caught fire, but it was honest about it.", "", {}, "Says: keep your notes on paper.")};
    } else if (id == "zoya") {
        s.greet = "I'm sleeping. Whatever it is, it can be whispered.";
        s.again = "Still awake. Unfortunately.";
        s.openLine = "A yawn through the wood. The door opens a crack, then a little more.";
        s.impression = "Night-shift nurse. Dead on her feet.";
        s.topics = {T("When did you get home last night?", "Seven this morning, straight off the ward. The lift was off, so the stairs. Wet and muddy, as always.", "", {}, "Home at 7 a.m. Stairs: wet, muddy."),
                    T("Seen Anton lately?", "He waved at the end of the month. Looked grey. I told him to see a doctor. He told me to see a better one.", "", {}, "Anton looked grey lately."),
                    T("Tips for a long shift?", "Sleep when you can. Eat what you find. Never say 'it will be quiet tonight' out loud.")};
    }
    return s;
}

// What you hear when you knock and nobody opens, by floor and door number (1..8). Edit freely.
const char* noAnswerLine(int floor, int door) {
    static const char* G[8] = {"A chain rattles. No one answers.", "A TV drones behind the door.", "A tired voice: \"Night shift. Not home.\"",
                               "A boy through the gap: \"Big guy came by. Red scarf.\"", "Mail piles up on the mat.",
                               "Crime scene tape across the frame. A muddy boot print by the hinge.", "Boiled cabbage smell. The door stays shut.",
                               "A radio plays softly. No answer."};
    static const char* U[8] = {"Somebody inside is humming opera. No answer.", "A keyboard clatters, then stops. Nobody comes.", "Silence. Not even a clock ticking.",
                               "Three locks click, then nothing.", "A neighbour coughs somewhere else. Nobody here.", "A fan hums. Nobody comes.",
                               "A fridge hums. Nothing else.", "Faint piano exercises from another flat. This door stays shut."};
    int i = std::clamp(door, 1, 8) - 1;
    return floor == 0 ? G[i] : U[i];
}
