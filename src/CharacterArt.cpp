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
//     A  auburn hair           X  lipstick red
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

const vector<string> WOMAN_FRONT = {
    "..................",
    ".....AAAAAAAA.....",
    "....AAAAAAAAAA....",
    "...AAAAAAAAAAAA...",
    "....AAssssssAA....",
    "....AAsessesAA....",
    "....AAssssssAA....",
    "....AAssxxssAA....",
    ".....AAssssAA.....",
    ".....AAAssAAA.....",
    "...AACCCRRCCCAA...",
    "...AACCCRRCCCAA...",
    "...AACCCCCCCCAA...",
    "....ACCCCCCCCA....",
    "....CCCRRRRCCC....",
    "....CCCCRRCCCC....",
    "....CCCCRRCCCC....",
    ".....CCCCCCCC.....",
    ".....CRRRRRRC.....",
    ".....CCCCCCCC.....",
    "...SSCCCCCCCCSS...",
    "...SSCCCCCCCCSS...",
    "..CCCCCCCCCCCCCC..",
    "..CCCCCCCCCCCCCC..",
    ".CCCCCCCCCCCCCCCC.",
    ".QQQQQQQQQQQQQQQQ.",
    "..QQQQQQQQQQQQQQ..",
    ".....SSS..SSS.....",
    ".....BBB..BBB.....",
    ".....BBB..BBB.....",
    ".....BBB..BBB.....",
    ".....BBB..BBB.....",
    "....BBBB..BBBB....",
    ".................."};

const vector<string> WOMAN_BACK = {
    "..................",
    ".....AAAAAAAA.....",
    "....AAAAAAAAAA....",
    "...AAAAAAAAAAAA...",
    "....AAAAAAAAAA....",
    "....AAAAAAAAAA....",
    "....AAAAAAAAAA....",
    "....AAAAAAAAAA....",
    ".....AAAAAAAA.....",
    ".....AAAAAAAA.....",
    "...AAAAAAAAAAAA...",
    "...AAAAAAAAAAAA...",
    "...AAAAAAAAAAAA...",
    "....AAAAAAAAAA....",
    "....CCAAAAAACC....",
    "....CCCAAAACCC....",
    "....CCCCAACCCC....",
    ".....CCCCCCCC.....",
    ".....CRRRRRRC.....",
    ".....CCCCCCCC.....",
    "...SSCCCCCCCCSS...",
    "...SSCCCCCCCCSS...",
    "..CCCCCCCCCCCCCC..",
    "..CCCCCCCCCCCCCC..",
    ".CCCCCCCCCCCCCCCC.",
    ".QQQQQQQQQQQQQQQQ.",
    "..QQQQQQQQQQQQQQ..",
    ".....SSS..SSS.....",
    ".....BBB..BBB.....",
    ".....BBB..BBB.....",
    ".....BBB..BBB.....",
    ".....BBB..BBB.....",
    "....BBBB..BBBB....",
    ".................."};

const vector<string> WOMAN_SIDE = {
    "..............",
    "...AAAAAAAA...",
    "..AAAAAAAAAA..",
    "..AAAAAAAAAA..",
    "...ssAAAAAAA..",
    "...seAAAAAAA..",
    "..sssAAAAAAA..",
    "...sxAAAAAAAA.",
    "...ssAAAAAAAA.",
    "....ssAAAAAAA.",
    "...RRRAAAAAAA.",
    "...CCCCAAAAAA.",
    "...CCCCCAAAAA.",
    "...CCCCCAAAAA.",
    "..CCCCCCCAAA..",
    "..CCCCCCCAAA..",
    "...CCCCCCCAA..",
    "....CCCCCC....",
    "....RRRRRR....",
    "....CCCCCC....",
    "...CCSSSSCC...",
    "...CCSSSSCC...",
    "..CCCCCCCCCC..",
    "..CCCCCCCCCC..",
    ".CCCCCCCCCCCC.",
    ".QQQQQQQQQQQQ.",
    "..QQQQQQQQQQ..",
    "....SSSSSS....",
    "....BBBBBB....",
    "....BBBBBB....",
    "....BBBBBB....",
    "....BBBBBB....",
    "..BBBBBBBBB...",
    ".............."};

const vector<string> TANK_FRONT = {
    "..................",
    "..ZZZZZZZZZZZZZZ..",
    "..ZCCCCCCCCCCCCZ..",
    "..ZCCCCCCCCCCCCZ..",
    "..ZCCCCCCCCCCCCZ..",
    "..ZCCCCCCCCCCCCZ..",
    "..ZooooooooooooZ..",
    "..ZCCCCCCCCCCCCZ..",
    "..ZCCCCCCCCCCCCZ..",
    "..ZCCCCCCCCCCCCZ..",
    "..ZCCCCCCCCCCCCZ..",
    "..ZooooooooooooZ..",
    "..ZCCCCCCCCCCCCZ..",
    "..ZCCCCCCCCCCCCZ..",
    "..ZCCCCCCCCCCCCZ..",
    "..ZCCCCCCCCCCCCZ..",
    "..ZooooooooooooZ..",
    "..ZCCCCCCCCCCCCZ..",
    "..ZZ..........ZZ..",
    "..ZZ..........ZZ..",
    "..ZZ..........ZZ..",
    "..ZZ..........ZZ..",
    "..ZZ..........ZZ..",
    ".................."};

const vector<string> ANTENNA_FRONT = {
    ".....ZZZZ.....",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "..ZZZZZZZZZZ..",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    ".ZZZZZZZZZZZZ.",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "...ZZZZZZZZ...",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "..ZZZZZZZZZZ..",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "....ZZZZZZ....",
    "....ZZZZZZ....",
    "..ZZZZZZZZZZ..",
    "..ZZZZZZZZZZ.."};

const vector<string> ANTENNA_SIDE = {
    ".....ZZZZ.....",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "....ZZZZZZ....",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "....ZZZZZZ....",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "....ZZZZZZ....",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "....ZZZZZZ....",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "......ZZ......",
    "....ZZZZZZ....",
    "....ZZZZZZ....",
    "..ZZZZZZZZZZ..",
    "..ZZZZZZZZZZ.."};

const vector<string> TABLE_FRONT = {
    "....................",
    "TTTTTTTTTTTTTTTTTTTT",
    "TTTTTTTTTTTTTTTTTTTT",
    ".DDDDDDDDDDDDDDDDDD.",
    ".TT..............TT.",
    ".TT..............TT.",
    ".TT..............TT.",
    ".TT..............TT.",
    ".TT..............TT.",
    ".TT..............TT.",
    ".TT..............TT.",
    ".TT..............TT.",
    ".TT..............TT.",
    ".TT..............TT."};

const vector<string> TABLE_SIDE = {
    "............",
    "TTTTTTTTTTTT",
    "TTTTTTTTTTTT",
    ".DDDDDDDDDD.",
    ".TT......TT.",
    ".TT......TT.",
    ".TT......TT.",
    ".TT......TT.",
    ".TT......TT.",
    ".TT......TT.",
    ".TT......TT.",
    ".TT......TT.",
    ".TT......TT.",
    ".TT......TT."};

const vector<string> CHAIR_FRONT = {
    "........",
    "TTTTTTTT",
    "TDDDDDDT",
    "TDDDDDDT",
    "TDDDDDDT",
    "TDDDDDDT",
    "TDDDDDDT",
    "TTTTTTTT",
    "TTTTTTTT",
    "TTTTTTTT",
    "TT....TT",
    "TT....TT",
    "TT....TT",
    "TT....TT",
    "TT....TT",
    "TT....TT"};

const vector<string> CHAIR_SIDE = {
    "........",
    ".....TTT",
    ".....TTT",
    ".....TTT",
    ".....TTT",
    ".....TTT",
    ".....TTT",
    ".....TTT",
    "TTTTTTTT",
    "TTTTTTTT",
    "TT....TT",
    "TT....TT",
    "TT....TT",
    "TT....TT",
    "TT....TT",
    "TT....TT"};

const vector<string> SOFA_FRONT = {
    "............................",
    "..CCCCCCCCCCCCCCCCCCCCCCCC..",
    "..CCCCCCCCCCCCCCCCCCCCCCCC..",
    "..CCCCDDDCCCCCCCCCCDDDCCCC..",
    "..CCCCCCCCCCCCCCCCCCCCCCCC..",
    "QQQQCCCCCCCCCCCCCCCCCCCCQQQQ",
    "QQQQCCCCCCCCCCCCCCCCCCCCQQQQ",
    "QQQQCCCCCCCCCCCCCCCCCCCCQQQQ",
    "QQQQCCCCCCCCCCCCCCCCCCCCQQQQ",
    "QQQQCCCCCCCCCCCCCCCCCCCCQQQQ",
    "QQQQCCCCCCCCCCCCCCCCCCCCQQQQ",
    "QQQQCCCCCCCCCCCCCCCCCCCCQQQQ",
    "QQQQCCCCCCCCCCCCCCCCCCCCQQQQ",
    "DDDDDDDDDDDDDDDDDDDDDDDDDDDD",
    "DDDDDDDDDDDDDDDDDDDDDDDDDDDD",
    ".BB......................BB."};

const vector<string> SOFA_SIDE = {
    "..............",
    ".........CCCCC",
    ".........CCCCC",
    ".........CCCCC",
    ".........CCCCC",
    "QQQQQQQQQQQQQQ",
    "QQQQQQQQQQQQQQ",
    "QQQQQQQQQQQQQQ",
    "QQQQQQQQQQQQQQ",
    "QQQQQQQQQQQQQQ",
    "QQQQQQQQQQQQQQ",
    "QQQQQQQQQQQQQQ",
    "QQQQQQQQQQQQQQ",
    "DDDDDDDDDDDDDD",
    "DDDDDDDDDDDDDD",
    ".BB........BB."};

const vector<string> WARDROBE_FRONT = {
    "..................",
    "DDDDDDDDDDDDDDDDDD",
    "DDDDDDDDDDDDDDDDDD",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "TTTTTTTTooTTTTTTTT",
    "DDDDDDDDDDDDDDDDDD",
    "DDDDDDDDDDDDDDDDDD"};

const vector<string> WARDROBE_SIDE = {
    "..........",
    "DDDDDDDDDD",
    "DDDDDDDDDD",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "TTTTTTTTTT",
    "DDDDDDDDDD",
    "DDDDDDDDDD"};

const vector<string> BED_FRONT = {
    "......................",
    "TTTTTTTTTTTTTTTTTTTTTT",
    "TTTTTTTTTTTTTTTTTTTTTT",
    "TTTTTTTTTTTTTTTTTTTTTT",
    ".CCCCCCCCCCCCCCCCCCCC.",
    ".CCWWWWWWWWWWWWWWWWCC.",
    ".CCWWWWWWWWWWWWWWWWCC.",
    ".CCCCCCCCCCCCCCCCCCCC.",
    ".CCCCCCCCCCCCCCCCCCCC.",
    ".CCCCCCCCCCCCCCCCCCCC.",
    ".CCCCCCCCCCCCCCCCCCCC.",
    "TT..................TT"};

const vector<string> BED_SIDE = {
    "..............................",
    "...........................TTT",
    "...........................TTT",
    "...........................TTT",
    "...........................TTT",
    "TT....................WWWWWWTT",
    "TT....................WWWWWWTT",
    "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
    "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
    "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
    "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
    "TT..........................TT"};

const vector<string> LAMP_FRONT = {
    "..........",
    "...FFFF...",
    "...FFFF...",
    "..FFFFFF..",
    "..FFFFFF..",
    ".FFFFFFFF.",
    ".FFFFFFFF.",
    ".FFFFFFFF.",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "..BBBBBB..",
    "..BBBBBB..",
    ".........."};

const vector<string> LAMP_SIDE = {
    "..........",
    "...FFFF...",
    "...FFFF...",
    "..FFFFFF..",
    "..FFFFFF..",
    ".FFFFFFFF.",
    ".FFFFFFFF.",
    ".FFFFFFFF.",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "....ZZ....",
    "..BBBBBB..",
    "..BBBBBB..",
    ".........."};

const vector<string> TV_FRONT = {
    "......................",
    "BBBBBBBBBBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBBBBBBBBBB",
    "BBVVVVVVVVVVVVVVVVVVBB",
    "BBVVVVVVVVVVVVVVVVVVBB",
    "BBVVVVVVVVVVVVVVVVVVBB",
    "BBVVVVVVVVVVVVVVVVVVBB",
    "BBVVVVVVVVVVVVVVVVVVBB",
    "BBVVVVVVVVVVVVVVVVVVBB",
    "BBVVVVVVVVVVVVVVVVVVBB",
    "BBVVVVVVVVVVVVVVVVVVBB",
    "BBVVVVVVVVVVVVVVVVVVBB",
    "BBBBBBBBBBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBBBBBBYYBB",
    "....ZZZZZZZZZZZZZZ....",
    "......................"};

const vector<string> TV_SIDE = {
    "..............",
    "BBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBB",
    "BBBBBBBBBBBBBB",
    "...ZZZZZZZZ...",
    ".............."};

const vector<string> PIANO_FRONT = {
    "..................................",
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
    ".TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT.",
    ".TTTTTTTTTTTWWWWWWWWWWTTTTTTTTTTT.",
    ".TTTTTTTTTTTWWWWWWWWWWTTTTTTTTTTT.",
    ".TTTTTTTTTTTWWWWWWWWWWTTTTTTTTTTT.",
    ".TTTTTTTTTTTWWWWWWWWWWTTTTTTTTTTT.",
    ".TTTTTTTTTTTWWWWWWWWWWTTTTTTTTTTT.",
    ".TTTTTTTTTTTWWWWWWWWWWTTTTTTTTTTT.",
    ".TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT.",
    ".TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT.",
    "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD",
    "..WoWoWoWoWoWoWoWoWoWoWoWoWoWoWo..",
    "..WoWoWoWoWoWoWoWoWoWoWoWoWoWoWo..",
    "..WoWoWoWoWoWoWoWoWoWoWoWoWoWoWo..",
    ".TTTTTTTTTTTTTDDDDDDTTTTTTTTTTTTT.",
    ".TTTTTTTTTTTTTDDDDDDTTTTTTTTTTTTT.",
    ".TTTTTTTTTTTTTDDDDDDTTTTTTTTTTTTT.",
    ".TTTTTTTTTTTTTDDDDDDTTTTTTTTTTTTT.",
    ".TTTTTTTTTTTTTDDDDDDTTTTTTTTTTTTT.",
    ".TTTTTTTTTTTTTDDDDDDTTTTTTTTTTTTT.",
    ".DDD...........YYYY...........DDD.",
    ".DDD..........................DDD."};

const vector<string> PIANO_SIDE = {
    "..............",
    "TTTTTTTTTTTTTT",
    "TTTTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "DDDDDDDDDDDDDD",
    "DDDDDDDDDDDDDD",
    "DDDDDDDDDDDDDD",
    "DDDDDDDDDDDDDD",
    "..TTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "..TTTTTTTTTTTT",
    "..DDD.....DDD.",
    "..DDD.....DDD."};

}  // namespace

// ================================================================== the machinery (no need to edit below this line)
namespace {

SpriteArt shadeArt(const vector<string>& rows) {          // colour keys + edge shading, no outline yet
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
    return a;
}

void addOutline(SpriteArt& a) {                                   // outline: empty pixels touching a filled one
    vector<char> src = a.c;
    for (int y = 0; y < a.h; y++)
        for (int x = 0; x < a.w; x++) {
            size_t i = (size_t)y * a.w + x;
            if (src[i] != ' ') continue;
            bool nb = (x > 0 && src[i - 1] != ' ') || (x < a.w - 1 && src[i + 1] != ' ') || (y > 0 && src[i - a.w] != ' ') || (y < a.h - 1 && src[i + a.w] != ' ');
            if (nb) a.c[i] = 'O';
        }
}

SpriteArt finishArt(const vector<string>& rows) { SpriteArt a = shadeArt(rows); addOutline(a); return a; }

SpriteSet makeSet(const vector<string>& f, const vector<string>& b, const vector<string>& s) { return {finishArt(f), finishArt(b), finishArt(s)}; }


// ---------------------------------------------------------------- 3D: every design is carved into a small voxel model
// The three flat views are enough to make a solid: a voxel exists where the FRONT silhouette (column ix, row iy) and the
// SIDE silhouette (column iz, row iy) both have a pixel. (This is "visual hull" carving: the model is the largest solid
// that looks right from the front and from the side.) iz = 0 is the front of the character, the side design faces left,
// so side column 0 is the front. The model is then rendered once, at load time, from SPRITE_VIEWS directions around it
// into ordinary SpriteArt frames, so drawing a character in the game stays one cheap camera-facing sprite.
struct VoxModel {
    int W = 0, H = 0, D = 0;
    SpriteArt front, back, side;                  // side already resampled to H rows
    bool solid(int ix, int iy, int iz) const {
        if (ix < 0 || iy < 0 || iz < 0 || ix >= W || iy >= H || iz >= D) return false;
        return front.c[(size_t)iy * W + ix] != ' ' && side.c[(size_t)iy * D + iz] != ' ';
    }
};

SpriteArt resampleRows(const SpriteArt& a, int h) {              // nearest-neighbour, in case a view has a different height
    if (a.h == h) return a;
    SpriteArt r; r.w = a.w; r.h = h; r.c.assign((size_t)r.w * h, ' '); r.s.assign((size_t)r.w * h, 0);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < a.w; x++) {
            size_t src = (size_t)std::clamp(y * a.h / h, 0, a.h - 1) * a.w + x, dst = (size_t)y * a.w + x;
            r.c[dst] = a.c[src]; r.s[dst] = a.s[src];
        }
    return r;
}

SpriteArt renderVoxelView(const VoxModel& m, int k) {
    const float PI2 = 6.2831853f;
    float phi = PI2 * (float)k / (float)SPRITE_VIEWS;
    float rx = std::cos(phi), rz = std::sin(phi);       // screen-right in (ix, iz) space
    float lx = -std::sin(phi), lz = std::cos(phi);      // looking direction
    int FW = (int)std::ceil(std::sqrt((float)(m.W * m.W + m.D * m.D))) + 2;
    if (FW % 2) FW++;                                   // even: the character stands exactly in the middle
    SpriteArt out; out.w = FW; out.h = m.H; out.c.assign((size_t)FW * m.H, ' '); out.s.assign((size_t)FW * m.H, 0);
    float cx = m.W * 0.5f, cz = m.D * 0.5f, far = (float)(m.W + m.D);
    for (int j = 0; j < FW; j++) {
        float off = (j + 0.5f) - FW * 0.5f;
        float ox = cx + rx * off - lx * far, oz = cz + rz * off - lz * far;     // start well outside the model
        int ix = (int)std::floor(ox), iz = (int)std::floor(oz);
        int sx = lx > 0 ? 1 : -1, sz = lz > 0 ? 1 : -1;
        float tdx = std::fabs(lx) < 1e-6f ? 1e30f : std::fabs(1.0f / lx), tdz = std::fabs(lz) < 1e-6f ? 1e30f : std::fabs(1.0f / lz);
        float tx = std::fabs(lx) < 1e-6f ? 1e30f : (lx > 0 ? (ix + 1 - ox) : (ox - ix)) * tdx;
        float tz = std::fabs(lz) < 1e-6f ? 1e30f : (lz > 0 ? (iz + 1 - oz) : (oz - iz)) * tdz;
        int nx = 0, nz = 0;                                 // normal of the face we entered through
        vector<char> done((size_t)m.H, 0);
        for (int step = 0; step < 4 * (m.W + m.D) + 8; step++) {
            for (int iy = 0; iy < m.H; iy++) {
                if (done[(size_t)iy] || !m.solid(ix, iy, iz)) continue;
                done[(size_t)iy] = 1;
                char key; int8_t sh;
                size_t fi = (size_t)iy * m.W + ix;
                if (nz < 0 || (nx == 0 && nz == 0)) { key = m.front.c[fi]; sh = m.front.s[fi]; }           // looking at the front of the character
                else if (nz > 0) {                                                                         // looking at the back
                    size_t bi = (size_t)iy * m.W + (m.W - 1 - ix);
                    if (m.back.c[bi] != ' ') { key = m.back.c[bi]; sh = m.back.s[bi]; } else { key = m.front.c[fi]; sh = -1; }
                } else { size_t si = (size_t)iy * m.D + iz; key = m.side.c[si]; sh = m.side.s[si]; }      // a flank
                float facing = nx * rx + nz * rz;                                                          // face turned toward the light (screen-left) or away
                int s2 = sh + (facing < -0.3f ? 1 : facing > 0.3f ? -1 : 0);
                size_t oi = (size_t)iy * FW + j;
                out.c[oi] = key; out.s[oi] = (int8_t)std::clamp(s2, -1, 1);
            }
            if (tx < tz) { ix += sx; tx += tdx; nx = -sx; nz = 0; } else { iz += sz; tz += tdz; nz = -sz; nx = 0; }
        }
    }
    addOutline(out);
    return out;
}

}  // namespace

const SpriteSet& spriteSet(int art) {
    static vector<SpriteSet> sets;
    if (sets.empty()) {                                   // order must match enum ArtId in Common.h
        sets.push_back(makeSet(MAN_FRONT, MAN_BACK, MAN_SIDE));
        sets.push_back(makeSet(OLD_FRONT, OLD_BACK, OLD_SIDE));
        sets.push_back(makeSet(CAT_FRONT, CAT_BACK, CAT_SIDE));
        { SpriteArt c = finishArt(CRATE); sets.push_back({c, c, c}); }
        sets.push_back(makeSet(JANITOR_FRONT, JANITOR_BACK, JANITOR_SIDE));
        sets.push_back(makeSet(WOMAN_FRONT, WOMAN_BACK, WOMAN_SIDE));
        sets.push_back(makeSet(TANK_FRONT, TANK_FRONT, TANK_FRONT));
        sets.push_back(makeSet(ANTENNA_FRONT, ANTENNA_FRONT, ANTENNA_SIDE));
        sets.push_back(makeSet(TABLE_FRONT, TABLE_FRONT, TABLE_SIDE));
        sets.push_back(makeSet(SOFA_FRONT, SOFA_FRONT, SOFA_SIDE));
        sets.push_back(makeSet(WARDROBE_FRONT, WARDROBE_FRONT, WARDROBE_SIDE));
        sets.push_back(makeSet(BED_FRONT, BED_FRONT, BED_SIDE));
        sets.push_back(makeSet(LAMP_FRONT, LAMP_FRONT, LAMP_SIDE));
        sets.push_back(makeSet(TV_FRONT, TV_FRONT, TV_SIDE));
        sets.push_back(makeSet(PIANO_FRONT, PIANO_FRONT, PIANO_SIDE));
        sets.push_back(makeSet(CHAIR_FRONT, CHAIR_FRONT, CHAIR_SIDE));
    }
    return sets[(size_t)std::clamp(art, 0, ART_COUNT - 1)];
}

const SpriteArt& spriteFrame(int art, int k) {
    static vector<vector<SpriteArt>> frames;
    if (frames.empty()) {
        frames.resize(ART_COUNT);
        const vector<string>* v[ART_COUNT][3] = {{&MAN_FRONT, &MAN_BACK, &MAN_SIDE}, {&OLD_FRONT, &OLD_BACK, &OLD_SIDE}, {&CAT_FRONT, &CAT_BACK, &CAT_SIDE},
                                                  {&CRATE, &CRATE, &CRATE}, {&JANITOR_FRONT, &JANITOR_BACK, &JANITOR_SIDE}, {&WOMAN_FRONT, &WOMAN_BACK, &WOMAN_SIDE},
                                                  {&TANK_FRONT, &TANK_FRONT, &TANK_FRONT}, {&ANTENNA_FRONT, &ANTENNA_FRONT, &ANTENNA_SIDE}, {&TABLE_FRONT, &TABLE_FRONT, &TABLE_SIDE}, {&SOFA_FRONT, &SOFA_FRONT, &SOFA_SIDE}, {&WARDROBE_FRONT, &WARDROBE_FRONT, &WARDROBE_SIDE}, {&BED_FRONT, &BED_FRONT, &BED_SIDE}, {&LAMP_FRONT, &LAMP_FRONT, &LAMP_SIDE}, {&TV_FRONT, &TV_FRONT, &TV_SIDE}, {&PIANO_FRONT, &PIANO_FRONT, &PIANO_SIDE}, {&CHAIR_FRONT, &CHAIR_FRONT, &CHAIR_SIDE}};   // order = enum ArtId
        for (int a = 0; a < ART_COUNT; a++) {
            VoxModel m;
            m.front = shadeArt(*v[a][0]);
            m.W = m.front.w; m.H = m.front.h;
            m.back = resampleRows(shadeArt(*v[a][1]), m.H);
            m.side = resampleRows(shadeArt(*v[a][2]), m.H);
            m.D = m.side.w;
            if (m.back.w != m.W) m.back = m.front;               // a back view of another width cannot be mapped: fall back to the front
            for (int i = 0; i < SPRITE_VIEWS; i++) frames[(size_t)a].push_back(renderVoxelView(m, i));
        }
    }
    return frames[(size_t)std::clamp(art, 0, ART_COUNT - 1)][(size_t)((k % SPRITE_VIEWS + SPRITE_VIEWS) % SPRITE_VIEWS)];
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
        case 'V': o = {160, 196, 232}; return true;     // television light
        case 'S': o = {210, 176, 150}; return true;
        case 'E': o = {22, 24, 32}; return true;
        case 'P': o = {66, 78, 100}; return true;
        case 'B': o = {58, 60, 66}; return true;
        case 'W': o = {200, 206, 212}; return true;
        case 'Y': o = {236, 200, 90}; return true;
        case 'G': o = {84, 128, 96}; return true;
        case 'T': o = {130, 96, 70}; return true;
        case 'A': o = {122, 58, 34}; return true;       // auburn hair
        case 'X': o = {178, 40, 60}; return true;       // lipstick
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
        {"tenant", "Old tenant", "Second floor. Complains about the lift.", "Talk to", ART_OLD, coat, shawl, 0.80f},
        // ---- the neighbours who matter less and are more fun
        {"fedorov", "Colonel Fedorov", "Apt 108. Retired. Plays chess against the radio.", "Talk to", ART_MAN, {96, 104, 84}, {196, 168, 80}, 0.94f},
        {"dasha", "Dasha", "Ground floor, by the lift. Earbuds. Fake phone calls.", "Talk to", ART_WOMAN, {196, 92, 140}, {240, 220, 90}, 0.80f},
        {"lev", "Lev", "Second floor. Counts raindrops at the north window.", "Talk to", ART_MAN, {110, 100, 84}, {150, 140, 110}, 0.88f},
        {"tamara", "Tamara Ilyinichna", "Apt 201. Retired opera singer. Everything is a stage.", "Talk to", ART_WOMAN, {120, 50, 90}, {220, 190, 90}, 0.86f},
        {"kolya", "Kolya", "Apt 202. Streams to six viewers and a bot.", "Talk to", ART_MAN, {60, 140, 150}, {230, 90, 200}, 0.86f},
        {"beketov", "Doctor Beketov", "Apt 204. Retired vet. Distrusts your watch.", "Talk to", ART_MAN, {88, 92, 84}, {200, 170, 80}, 0.90f},
        {"zoya", "Zoya", "Apt 206. Night nurse. Dead on her feet.", "Talk to", ART_WOMAN, {70, 130, 140}, {226, 230, 232}, 0.84f}};
    return cast;
}
