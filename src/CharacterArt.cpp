// CharacterArt.cpp - THE CAST AND THEIR DESIGNS. Edit this file to change how people look.
//
// HOW A CHARACTER IS DRAWN
//   Each design is a small grid of text (one character = one pixel) in three views: FRONT, BACK and SIDE (the side view
//   always faces LEFT in the grid). In the world a character is a flat billboard made of two crossed vertical planes:
//   one plane shows FRONT or BACK (whichever side the camera is on), the other shows SIDE (mirrored when seen from the
//   other side). So to redesign someone you just redraw these grids - and keep all three views the same size.
//
//   Pixel keys (the letter picks the colour):
//     C / K  main colour of the character        Q  darker main colour       D  much darker main colour
//     R      accent colour of the character      S  skin                     H  hair / hat grey
//     P      trousers                            B  boots                    T  brown (wood, leather)
//     W  white    Y  yellow/brass    G  green     Z  metal grey               N  pink (nose, ears)
//     F  glowing eyes (cat)    E  dark eye        O  dark outline detail      .  empty (transparent)
//   UPPER-CASE letters are shaded automatically (light from the top left: lit edges are brighter, edges facing away
//   are darker) and an outline is added around the whole shape. lower-case letters (e, o, y, f, n, w) are FLAT details
//   painted on top: no shading, used for eyes, buttons, seams.
//
//   The colours "main" and "accent" and the height come from the cast list at the bottom of this file, so one design
//   can be reused for many people (Volkova and Petrova share the old-woman design in different colours).
#include "Entities.h"

namespace {

// ================================================================== DESIGNS (redraw these)
const vector<string> MAN_FRONT = {
    "..................",
    ".....HHHHHHHH.....",
    ".....HHHHHHHH.....",
    ".....HHHHHHHH.....",
    "....HHHHHHHHHH....",
    "....HHHHHHHHHH....",
    "....HHSSSSSSHH....",
    "....HHSeSSeSHH....",
    "....HHSSSSSSHH....",
    "....HHSSooSSHH....",
    ".....RRRRRRRR.....",
    ".....RRRRRRRR.....",
    "....CRRRRRRRRC....",
    "..CCCCCCCRRRCCCC..",
    "..CCCCCCCRRRCCCC..",
    "..CCCCCyCRRRCCCC..",
    "..CCCCCCCRRRCCCC..",
    "..CCCCCCCRRRCCCC..",
    "..CCCCCyCRRRCCCC..",
    "..CCCCCCCRRRCCCC..",
    "..CCCCCCCRRRCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..CCCooCoCCooCCC..",
    "..CCCCCCoCCCCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..SSCCCCoCCCCCSS..",
    "..SSCCCCoCCCCCSS..",
    ".....PPP..PPP.....",
    ".....PPP..PPP.....",
    ".....PPP..PPP.....",
    ".....PPP..PPP.....",
    "....BBBB..BBBB....",
    ".................."};

const vector<string> MAN_BACK = {
    "..................",
    ".....HHHHHHHH.....",
    ".....HHHHHHHH.....",
    ".....HHHHHHHH.....",
    "....HHHHHHHHHH....",
    "....HHHHHHHHHH....",
    "....HHHHHHHHHH....",
    "....HHHHHHHHHH....",
    "....HHHHHHHHHH....",
    "....HHHHHHHHHH....",
    ".....RRRRRRRR.....",
    ".....RRRRRRRR.....",
    "....CRRRRRRRRC....",
    "..CCCCCCoCCCCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..CCCCCCooCCCCCC..",
    "..CCCCCCoCCCCCCC..",
    "..SSCCCCoCCCCCSS..",
    "..SSCCCCoCCCCCSS..",
    ".....PPP..PPP.....",
    ".....PPP..PPP.....",
    ".....PPP..PPP.....",
    ".....PPP..PPP.....",
    "....BBBB..BBBB....",
    ".................."};

const vector<string> MAN_SIDE = {
    "..............",
    "...HHHHHHHH...",
    "...HHHHHHHH...",
    "...HHHHHHHH...",
    "...HHHHHHHH...",
    "...HHHHHHHH...",
    "....SSSHHH....",
    "....SeSHHH....",
    "...SSSSHHH....",
    "....SSSHHH....",
    "...RRRRRRRR...",
    "...RRRRRRRR...",
    "...RRRRRRRR...",
    "...CCCCCCCRR..",
    "...CCCCCCCRR..",
    "...CCCCCCCRR..",
    "...CCCCCCCRR..",
    "...CCCCCCCRR..",
    "...CCCCCCCRR..",
    "...CCCCCCCRR..",
    "...CCCCCCCRR..",
    "...CCCCCCCC...",
    "...CCooCCCC...",
    "...CCCCCCCC...",
    "...CCCCCCCC...",
    "...CCCCCCCC...",
    "...CCSSSSCC...",
    "...CCSSSSCC...",
    "....PPPPPP....",
    "....PPPPPP....",
    "....PPPPPP....",
    "....PPPPPP....",
    "..BBBBBBBBB...",
    ".............."};

const vector<string> OLD_FRONT = {
    "......................",
    "......................",
    ".......RRRRRRRR.......",
    ".......RRRRRRRR.......",
    ".......RRRRRRRR.......",
    ".......RWSSSSWR.......",
    ".......RSSSSSSR.......",
    ".......RSeSSeSR.......",
    ".......RSSSSSSR.......",
    ".......RSSooSSR.......",
    "......RRRRRRRRRR......",
    "......RRRRRRRRRR......",
    ".....CRRRRRRRRRRC.....",
    ".....CRRRRRRRRRRC.....",
    ".....CCRRRCCCCCCC.....",
    ".....CCRRRCCCCCCC.....",
    ".....CCRRRCCCCCCC.....",
    ".....CCCCCCCCCCCC.....",
    ".....CCCCCCCCCCCC.....",
    ".....CCCCCCCCCCCC.....",
    ".....CCCCCCCCCCCC.....",
    ".....CCPPPPPPPPCC.....",
    ".....SSPPPPPPPPSS.....",
    ".....SSPPPPPPPPSSoo...",
    "......PPPPPPPPPP.TTT..",
    "......PPPPPPPPPP.TTT..",
    "......PPPPPPPPPP.TTT..",
    ".......BBB..BBB..TTT..",
    ".......BBB..BBB..TTT..",
    "......................"};

const vector<string> OLD_BACK = {
    "......................",
    "......................",
    ".......RRRRRRRR.......",
    ".......RRRRRRRR.......",
    ".......RRRoRRRR.......",
    ".......RRRRRRRR.......",
    ".......RRRRRRRR.......",
    ".......RRRRRRRR.......",
    ".......RRRRRRRR.......",
    ".......RRRRRRRR.......",
    "......RRRWRRWRRR......",
    "......RRRRRRRRRR......",
    ".....CRRRRRRRRRRC.....",
    ".....CRRRRRRRRRRC.....",
    ".....CCCCCCCCCCCC.....",
    ".....CCCCCCCCCCCC.....",
    ".....CCCCCCCCCCCC.....",
    ".....CCCCCCCCCCCC.....",
    ".....CCCCCCCCCCCC.....",
    ".....CCCCCCCCCCCC.....",
    ".....CCCCCCCCCCCC.....",
    ".....CCPPPPPPPPCC.....",
    ".....SSPPPPPPPPSS.....",
    "...ooSSPPPPPPPPSS.....",
    "..TTT.PPPPPPPPPP......",
    "..TTT.PPPPPPPPPP......",
    "..TTT.PPPPPPPPPP......",
    "..TTT..BBB..BBB.......",
    "..TTT..BBB..BBB.......",
    "......................"};

const vector<string> OLD_SIDE = {
    "..............",
    "..............",
    "...RRRRRRRR...",
    "...RRRRRRRR...",
    "...RRRRRRRR...",
    "....SSRRRRR...",
    "....SSRRRRR...",
    "....SeRRRRR...",
    "...SSSRRRRR...",
    "....SSRRRRR...",
    "...RRRRRRRRR..",
    "...RRRRRRRRR..",
    "...RRCCCCRRR..",
    "...RRCCCCRRR..",
    "...CCCCCCRRR..",
    "...CCCCCCRRR..",
    "...CCCCCCCC...",
    "...CCCCCCCC...",
    "...CCCCCCCC...",
    "...CCCCCCCC...",
    "...CCCCCCCC...",
    "...PPCCCCPP...",
    "...PPSSSSPP...",
    "..PPPSSSSPPP..",
    "..PTTTTTPPPP..",
    "..PTTTTTPPPP..",
    "..PTTTTTPPPP..",
    "..BTTTTT......",
    "..BTTTTT......",
    ".............."};

const vector<string> CAT_FRONT = {
    "............",
    "............",
    "..KK.....KK.",
    "..KnQKQKQnK.",
    "..KKKKQKKKK.",
    "...KKKKKKK..",
    "...KfKKKfK..",
    "...KKKnKKK..",
    "....KKKKK...",
    "...RRRRRRK..",
    "...KKWyKKK..",
    "..KKKWWKKKK.",
    "...KKWWKKK..",
    "...KKKKKKK..",
    "...WWKKWW...",
    "............"};

const vector<string> CAT_BACK = {
    "............",
    "..KK.....KK.",
    "..KK.....KK.",
    "..KKKKKKKKQ.",
    "...KKKKKKKK.",
    "...KKKKKKKK.",
    "...KKQKQKKK.",
    "...KRRRRRKK.",
    "...KKQKQKKK.",
    "..KKKQKQKKK.",
    "..KKKKKKKKK.",
    "..KKQQQQQKK.",
    "...KKKKKKK..",
    "...KKKKKKK..",
    "...WWWKWWW..",
    "............"};

const vector<string> CAT_SIDE = {
    "........................",
    "........................",
    "..KK..KK...........KKK..",
    "..Kn..Kn.............KK.",
    "..KK.QKK.............QK.",
    "...KKQKK.............KK.",
    "..KfKKfKK.QKQKQ.Q....QK.",
    "..KKKKKKRKQKQKQKQK..KK..",
    "..nKKKKKRKQKQKQKQKKKK...",
    "...KKKKKRKKKKKKKKKKK....",
    ".....KKKRKKKKKKKKKK.....",
    "....KK.KrKKKKKKKKKK.....",
    "....KK.KKrKKKKKKKKK.....",
    "....KK.KK......KKKK.....",
    "....WW.WW......WWWW.....",
    "........................"};

const vector<string> CRATE = {
    "..................",
    ".CCCCCCCCCCCCCCCC.",
    ".CyCCCCCCCCCCCCyC.",
    ".CCCCCCCCCCCCCCCC.",
    ".CCCCCCCCCCCCCoCC.",
    ".oooooooooooooooo.",
    ".CCCCCCCCCCooCCCC.",
    ".CCCCCCCCCoCCCCCC.",
    ".CCCCCCCooCCCCCCC.",
    ".oooooooooooooooo.",
    ".CCCCooCCCCCCCCCC.",
    ".CCooCCCCCCCCCCCC.",
    ".CyCCCCCCCCCCCCyC.",
    ".CCCCCCCCCCCCCCCC.",
    ".................."};

const vector<string> JANITOR_FRONT = {
    "........................",
    "........................",
    ".........HHHHHH.........",
    ".........HHHHHH.....T...",
    "........HHHHHHHH....T...",
    ".......HHHHHHHHH....T...",
    ".........SSSSSS.....T...",
    ".........SSSSSS.....T...",
    ".........SeSSeS.....T...",
    ".........ShhhhS.....T...",
    ".........SSSSSS.....T...",
    ".......RRRSSSSRRR...T...",
    "....RRRRCCRRRRCCRRRRT...",
    "....RRRRCCRRRRCCRRRRT...",
    "....RRRRCCCCCCCCRRRRT...",
    "....RRRRCyCCCCyCRRRRT...",
    "....RRRRCCCCCCCCRRRRT...",
    "....RRRRCCCCCCCCRRRRT...",
    "....RRRRCCooooCCRRRRT...",
    "....RRRRCCCCCCCCRRRRT...",
    "....RRRRCCCCCCCCRRSSS...",
    "....RRRRCCCCCCCCRRSSS...",
    "....RRRRCCCCCCCCRRRRT...",
    "....RRRRCCCCCCCCRRRRT...",
    "....SSSRCCCCCCCCR...T...",
    "....SSSRCCCCCCCCR...T...",
    ".......RCCCCCCCCR...T...",
    ".......CCCC..CCCC...T...",
    ".......CCCC..CCCC...T...",
    ".......CCCC..CCCC...T...",
    ".......CCCC..CCCC..YYY..",
    ".......CCCC..CCCC..YYY..",
    "......BBBBB..BBBBB.YYY..",
    "........................"};

const vector<string> JANITOR_BACK = {
    "........................",
    "........................",
    ".........HHHHHH.........",
    "...T.....HHHHHH.........",
    "...T....HHHHHHHH........",
    "...T....HHHHHHHH........",
    "...T.....HHHHHH.........",
    "...T.....HHHHHH.........",
    "...T.....HHHHHH.........",
    "...T.....HHHHHH.........",
    "...T.....HHHHHH.........",
    "...T...RRRSSSSRRR.......",
    "...TRRRRCCRRRRCCRRRR....",
    "...TRRRRCCRRRRCCRRRR....",
    "...TRRRRCoCCCCoCRRRR....",
    "...TRRRRCCoCCoCCRRRR....",
    "...TRRRRCCCooCCCRRRR....",
    "...TRRRRCCCooCCCRRRR....",
    "...TRRRRCCoCCoCCRRRR....",
    "...TRRRRCoCCCCoCRRRR....",
    "...SSSRRoCCCCCCoRRRR....",
    "...SSSRoCCCCCCCCoRRR....",
    "...TRRRRCCCCCCCCRRRR....",
    "...TRRRRCCCCCCCCRRRR....",
    "...TSSSRCCCCCCCCR.......",
    "...TSSSRCCCCCCCCR.......",
    "...T...RCCCCCCCCR.......",
    "...T...CCCC..CCCC.......",
    "...T...CCCC..CCCC.......",
    "...T...CCCC..CCCC.......",
    "..YYY..CCCC..CCCC.......",
    "..YYY..CCCC..CCCC.......",
    "..YYY.BBBBB..BBBBB......",
    "........................"};

const vector<string> JANITOR_SIDE = {
    "................",
    "................",
    ".....HHHHHH.....",
    "..T..HHHHHH.....",
    "..T.HHHHHHH.....",
    "..THHHHHHHH.....",
    "..T..SSSSSS.....",
    "..T..SSSSSS.....",
    "..T..SeSSSS.....",
    "..T.ShhSSSS.....",
    "..T..SSSSSS.....",
    "..T.RRSSSSRR....",
    "..T.RRRRRRRR....",
    "..T.RRRRRRRR....",
    "..T.CRRRRCCC....",
    "..T.CRRRRCCC....",
    "..T.CRRRRCCC....",
    "..T.CRRRRCCC....",
    "..T.CRRRRCCC....",
    "..T.CRRRRCCC....",
    "..SSSSRRRCCC....",
    "..SSSSRRRCCC....",
    "..T.CRRRRCCC....",
    "..T.CRRRRCCC....",
    "..T.CCCCCCCC....",
    "..T.CCCCCCCC....",
    "..T.CCCCCCCC....",
    "..T..CCCCCC.....",
    "..T..CCCCCC.....",
    "..T..CCCCCC.....",
    ".YYY.CCCCCC.....",
    ".YYY.CCCCCC.....",
    ".YYBBBBBBBBB....",
    "................"};

}  // namespace

// ================================================================== the machinery (no need to edit below this line)
namespace {

SpriteArt finishArt(const vector<string>& rows) {
    SpriteArt a;
    a.h = (int)rows.size(); a.w = (int)rows[0].size();
    a.c.assign((size_t)a.w * a.h, ' '); a.s.assign((size_t)a.w * a.h, 0);
    auto key = [&](int x, int y) -> char {                       // the shaded/flat key at a pixel, upper-case, ' ' = empty
        if (x < 0 || y < 0 || x >= a.w || y >= a.h) return ' ';
        char ch = rows[(size_t)y][(size_t)x];
        return ch == '.' ? ' ' : (char)std::toupper((unsigned char)ch);
    };
    for (int y = 0; y < a.h; y++)
        for (int x = 0; x < a.w; x++) {
            char raw = rows[(size_t)y][(size_t)x], me = key(x, y);
            size_t i = (size_t)y * a.w + x;
            if (me == ' ') continue;
            a.c[i] = me;
            if (std::islower((unsigned char)raw)) continue;      // flat detail: no shading
            bool shadow = key(x + 1, y) != me || key(x, y + 1) != me;      // edge facing away from the light
            bool light = key(x - 1, y) == ' ' || key(x, y - 1) == ' ';     // silhouette edge facing the light
            a.s[i] = shadow ? -1 : (light ? 1 : 0);
        }
    vector<char> src = a.c;                                       // outline: empty pixels touching a filled one
    for (int y = 0; y < a.h; y++)
        for (int x = 0; x < a.w; x++) {
            size_t i = (size_t)y * a.w + x;
            if (src[i] != ' ') continue;
            bool nb = (x > 0 && src[i - 1] != ' ') || (x < a.w - 1 && src[i + 1] != ' ') || (y > 0 && src[i - a.w] != ' ') || (y < a.h - 1 && src[i + a.w] != ' ');
            if (nb) a.c[i] = 'O';
        }
    return a;
}

SpriteSet makeSet(const vector<string>& f, const vector<string>& b, const vector<string>& s) { return {finishArt(f), finishArt(b), finishArt(s)}; }

}  // namespace

const SpriteSet& spriteSet(int art) {
    static vector<SpriteSet> sets;
    if (sets.empty()) {                                   // order must match enum ArtId in Common.h
        sets.push_back(makeSet(MAN_FRONT, MAN_BACK, MAN_SIDE));
        sets.push_back(makeSet(OLD_FRONT, OLD_BACK, OLD_SIDE));
        sets.push_back(makeSet(CAT_FRONT, CAT_BACK, CAT_SIDE));
        { SpriteArt c = finishArt(CRATE); sets.push_back({c, c, c}); }
        sets.push_back(makeSet(JANITOR_FRONT, JANITOR_BACK, JANITOR_SIDE));
    }
    return sets[(size_t)std::clamp(art, 0, ART_COUNT - 1)];
}

bool spriteColor(char ch, RGB main, RGB acc, RGB& o) {
    switch (ch) {
        case 'C': case 'K': o = main; return true;
        case 'D': o = mul(main, 0.55f); return true;
        case 'R': o = acc; return true;
        case 'H': o = {84, 90, 102}; return true;
        case 'O': o = {16, 14, 20}; return true;
        case 'Q': o = mul(main, 0.68f); return true;
        case 'N': o = {196, 120, 124}; return true;
        case 'Z': o = {110, 116, 122}; return true;
        case 'F': o = {226, 230, 96}; return true;
        case 'S': o = {210, 176, 150}; return true;
        case 'E': o = {22, 24, 32}; return true;
        case 'P': o = {66, 78, 100}; return true;
        case 'B': o = {58, 60, 66}; return true;
        case 'W': o = {200, 206, 212}; return true;
        case 'Y': o = {236, 200, 90}; return true;
        case 'G': o = {84, 128, 96}; return true;
        case 'T': o = {130, 96, 70}; return true;
        default: return false;
    }
}

// ================================================================== THE CAST (names, looks, heights)
// {id, name shown in dialogue and journal, role line for the journal, verb for the prompt ("[E] Talk to ..."),
//  design, main colour, accent colour, height as a fraction of the wall (0.62 = a boy, 0.95 = a big man)}
// The id ties a person to their lines in DialogueData.cpp and to their place in LevelData.cpp.
const vector<CastDef>& castList() {
    static const RGB coat{96, 92, 112}, shawl{150, 80, 70};
    static const vector<CastDef> cast = {
        {"volkova", "Mrs. Volkova", "Apt 101. Nosy neighbour.", "Talk to", ART_OLD, coat, shawl, 0.80f},
        {"gromov", "Mr. Gromov", "Apt 102. Headphones, football.", "Talk to", ART_MAN, {120, 96, 70}, {110, 120, 90}, 0.92f},
        {"misha", "Misha", "Apt 104. The boy who does not sleep.", "Talk to", ART_MAN, {90, 110, 140}, {120, 150, 170}, 0.62f},
        {"petrova", "Mrs. Petrova", "Apt 107. Cabbage and silence.", "Talk to", ART_OLD, {110, 100, 90}, {140, 110, 80}, 0.78f},
        {"scarf", "Man in red scarf", "Hallway. Big, wool scarf. Walks the ring.", "Talk to", ART_MAN, {64, 68, 78}, {196, 52, 40}, 0.95f},
        {"cat", "Boris", "Hallway. Tabby with a red collar.", "Crouch by", ART_CAT, {220, 140, 60}, {200, 60, 60}, 0.34f},
        {"pavel", "Pavel", "Second floor. Janitor. Keys and a broom.", "Talk to", ART_JANITOR, {78, 112, 140}, {226, 170, 60}, 0.90f},
        {"tenant", "Old tenant", "Second floor. Complains about the lift.", "Talk to", ART_OLD, coat, shawl, 0.80f}};
    return cast;
}
