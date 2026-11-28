// LevelData.cpp - THE LEVEL DESIGN. Edit this file to change the building.
//
// Every floor is an ASCII plan, one character per tile (a tile = 2x2 cells = about one doorway wide). Both floors have
// the same size and sit exactly on top of each other. Legend:
//     #  wall               W  window (outer wall)       .  walkable floor (hallway, landings)
//     k  stair handrail     m  metal (lift cabin walls)  1-8  apartment doors (101..108 / 201..208)
//     E  lift doors         L  lift cabin                u / d  staircase tiles (only for reading: see "flights")
//     P  roof parapet       ,  open roof (outdoor floors only)
//
// Reading the plan (19 x 15 tiles): a hallway ring runs round the outside (windows in the outer wall), the apartments
// are the solid block inside it, and the stairwell sits in the CENTRE: two flights side by side, one going up (on the
// ground floor it is on the left, entered from the north hallway) and one going down (on the right, entered from the
// south hallway). The lift is in the north-west corner of the block.
#include "Level.h"

BuildingDef designBuilding() {
    BuildingDef b;

    // ------------------------------------------------------------------ ground floor
    //            0         1
    //            0123456789012345678
    LevelDef g;
    g.name = "Ground floor";
    g.rows = {
        "###W###W###W###W###",   //  0  outer wall, windows
        "#.................#",   //  1  north hallway
        "#.###1##.####2###.#",   //  2  doors 101, 102 | (8,2) = mouth of the UP flight
        "#.#m###kuk#######.#",   //  3  lift cabin wall | up flight (x=8) | rail (x=9)
        "W.ELm##kuk#######.W",   //  4  lift doors + cabin (x=2,3)
        "#.#m###kuk######4.#",   //  5  door 104
        "#.#####kuk#######.#",   //  6  (10,6) = locked boiler-room door (made from the flight definition)
        "#.3####kukdk#####.#",   //  7  door 103 | down flight (x=10) starts here
        "#.#######kdk#####.#",   //  8  (8,8) = top of the up flight
        "#.#######kdk####6.#",   //  9  door 106 (the crime scene)
        "W.5######kdk#####.W",   // 10  door 105
        "#.#######kdk#####.#",   // 11
        "#.###7####.##8###.#",   // 12  doors 107, 108 | (10,12) = mouth of the DOWN flight
        "#.................#",   // 13  south hallway
        "###W###W###W###W###"};  // 14
    // ceiling bulbs, tile units; mode 0 steady, 1 buzzing, 2 dying. Spaced irregularly on purpose.
    g.bulbs = {{2.6f, 1.5f, 0}, {6.2f, 1.5f, 1}, {9.6f, 1.5f, 0}, {13.0f, 1.5f, 0}, {16.4f, 1.5f, 0},
               {17.5f, 3.4f, 0}, {17.5f, 7.6f, 0}, {17.5f, 11.8f, 0},
               {15.0f, 13.5f, 0}, {11.2f, 13.5f, 0}, {7.0f, 13.5f, 0}, {3.3f, 13.5f, 0},
               {1.5f, 10.4f, 2}, {1.5f, 6.0f, 0},
               {8.5f, 5.5f, 0}, {10.5f, 9.5f, 0},          // over the two flights
               {3.5f, 4.5f, 0}};                            // inside the lift cabin

    // ------------------------------------------------------------------ upper floor
    LevelDef u;
    u.name = "Second floor";
    u.rows = {
        "###W###W###W###W###",   //  0
        "#.................#",   //  1
        "#.###1####.##2###.#",   //  2  doors 201, 202 | (10,2) = mouth of the UP flight (to the roof)
        "#.#m###kdkuk#####.#",   //  3  down flight (x=8) | up flight (x=10)
        "W.ELm##kdkuk#####.W",   //  4  lift
        "#.#m###kdkuk####4.#",   //  5  door 204
        "#.#####kdkuk#####.#",   //  6
        "#.3####kdkuk#####.#",   //  7  door 203
        "#.######.########.#",   //  8  (8,8) = foot of the stairs you came up; (10,8) = locked roof door
        "#.######.#######6.#",   //  9  door 206
        "W.5#####.########.W",   // 10  door 205
        "#.######.########.#",   // 11
        "#.###7##.####8###.#",   // 12  doors 207, 208 | (8,12) = corridor from the south hallway to the stairs
        "#.................#",   // 13
        "###W###W###W###W###"};  // 14
    u.bulbs = {{2.6f, 1.5f, 0}, {6.2f, 1.5f, 0}, {9.6f, 1.5f, 0}, {13.0f, 1.5f, 1}, {16.4f, 1.5f, 0},
               {17.5f, 3.4f, 0}, {17.5f, 7.6f, 0}, {17.5f, 11.8f, 0},
               {15.0f, 13.5f, 0}, {11.2f, 13.5f, 0}, {7.0f, 13.5f, 0}, {3.3f, 13.5f, 0},
               {1.5f, 10.4f, 0}, {1.5f, 6.0f, 2},
               {8.5f, 5.5f, 0}, {10.5f, 5.5f, 0}, {8.5f, 10.5f, 0},
               {3.5f, 4.5f, 0}};
    // ------------------------------------------------------------------ roof (outdoors)
    // ',' = open roof under the sky (no ceiling, rain falls), P = the low parapet round the edge, # = walls of the little
    // stair hut in the middle. The flight you climbed from the second floor comes up inside the hut.
    LevelDef r;
    r.name = "Roof";
    r.outdoor = true;
    r.rows = {
        "PPPPPPPPPPPPPPPPPPP",   //  0
        "P,,,,,,,,,,,,,,,,,P",   //  1
        "P,,,,,,,#####,,,,,P",   //  2  the hut's north wall
        "P,,,,,,,#kuk#,,,,,P",   //  3  the flight from the second floor (x=10)
        "P,,,,,,,#kuk#,,,,,P",   //  4
        "P,,,,,,,#kuk#,,,,,P",   //  5
        "P,,,,,,,#kuk#,,,,,P",   //  6
        "P,,,,,,,#kuk#,,,,,P",   //  7  (10,7) = top step
        "P,,,,,,,#...#,,,,,P",   //  8  landing inside the hut
        "P,,,,,,,##.##,,,,,P",   //  9  the hut's door, open
        "P,,,,,,,,,,,,,,,,,P",   // 10
        "P,,,,,,,,,,,,,,,,,P",   // 11
        "P,,,,,,,,,,,,,,,,,P",   // 12
        "P,,,,,,,,,,,,,,,,,P",   // 13
        "PPPPPPPPPPPPPPPPPPP"};  // 14
    // bulbs: two inside the hut, then invisible lamps: mode 3 = lit by lightning, mode 4 = faint glow of the town
    r.bulbs = {{10.5f, 8.5f, 0}, {10.5f, 4.5f, 2},
               {4.0f, 4.0f, 3}, {15.0f, 4.0f, 3}, {4.0f, 11.0f, 3}, {15.0f, 11.0f, 3}, {9.5f, 12.5f, 3},
               {4.0f, 7.5f, 4}, {15.0f, 7.5f, 4}, {9.5f, 11.0f, 4}};
    b.floors = {g, u, r};

    // ------------------------------------------------------------------ flights (staircases)
    // One entry per physical staircase. It is laid into BOTH floors it connects, so the floors always agree.
    // {first step tile x, y, direction of the climb, lower floor (-1 = basement)}
    b.flights = {
        {8, 3, DIR_S, 0},     // ground <-> second floor: climbs south along x=8 (entered from the north hallway downstairs)
        {10, 7, DIR_S, -1},   // basement -> ground floor: the DOWN flight on the ground floor (ends at a locked boiler door)
        {10, 3, DIR_S, 1}};   // second floor -> roof: the UP flight on the second floor (comes out in the roof hut)

    // ------------------------------------------------------------------ you
    b.startFloor = 0; b.start = {2.5f, 1.5f}; b.startYaw = 0;

    // ------------------------------------------------------------------ people in the hallways. id = see the cast list in CharacterArt.cpp
    // {id, floor, tile position, patrol route (empty = stands still), tiles per second, facing in radians}
    b.characters = {
        {"scarf", 0, {17.5f, 1.5f}, {{17.5f, 1.5f}, {17.5f, 13.5f}, {1.5f, 13.5f}, {1.5f, 1.5f}}, 0.8f, 0},
        {"cat", 0, {4.5f, 13.5f}, {{4.5f, 13.5f}, {11.5f, 13.5f}}, 0.5f, 0},
        {"pavel", 1, {17.5f, 9.5f}, {}, 0, 3.14159265f},
        {"tenant", 1, {3.0f, 13.5f}, {}, 0, -1.5707963f},
        {"dasha", 0, {1.25f, 4.6f}, {}, 0, 0},                 // waits by the lift doors, talking to nobody
        {"lev", 1, {11.5f, 1.35f}, {}, 0, -1.5707963f}};       // stares at the rain through the north window

    // ------------------------------------------------------------------ who lives behind which door: they answer when you knock
    // {id, floor, door number (1 = apartment x01)}
    b.homes = {{"volkova", 0, 1}, {"gromov", 0, 2}, {"misha", 0, 4}, {"petrova", 0, 7}, {"fedorov", 0, 8},
               {"tamara", 1, 1}, {"kolya", 1, 2}, {"beketov", 1, 4}, {"zoya", 1, 6}};
    // doors that stay empty on purpose (peephole rooms later): 103, 105 downstairs; 203, 205, 207, 208 upstairs

    // ------------------------------------------------------------------ props (no characters): {art, floor, tile position, height, colour, facing}
    const RGB wood{150, 112, 76};
    b.props = {{ART_CRATE, 0, {15.5f, 13.5f}, 0.3f, wood, 0.35f}, {ART_CRATE, 0, {1.5f, 8.5f}, 0.3f, wood, 1.7f},
               {ART_CRATE, 1, {17.5f, 3.5f}, 0.3f, wood, 3.0f},
               // the roof: crates, two water tanks, two aerials
               {ART_CRATE, 2, {3.5f, 3.5f}, 0.3f, wood, 0.4f}, {ART_CRATE, 2, {4.4f, 3.2f}, 0.22f, wood, 1.2f}, {ART_CRATE, 2, {15.5f, 12.0f}, 0.3f, wood, 2.0f},
               {ART_TANK, 2, {4.5f, 11.5f}, 0.75f, {88, 108, 104}, 0.0f}, {ART_TANK, 2, {14.5f, 6.5f}, 0.9f, {104, 94, 84}, 0.0f},
               {ART_ANTENNA, 2, {3.0f, 7.5f}, 1.35f, {110, 112, 110}, 0.0f}, {ART_ANTENNA, 2, {16.0f, 3.0f}, 1.2f, {110, 112, 110}, 0.0f}};

    // ------------------------------------------------------------------ what you see through a peephole
    // Every flat is the same small room (6 x 5 tiles: wall, 4 x 3 tiles of floor, wall) seen from just inside its door, which is
    // the west wall; the windows are on the east wall, the rain beyond them. The furniture is what makes each one different.
    // Props: {art, (floor, ignored), tile position, height, colour, facing in radians: 0 = faces east (away from the door), pi = faces the door}.
    const float PI_ = 3.14159265f, H_ = PI_ / 2;
    auto room = [&](int floor, int door, vector<Bulb> bulbs, vector<PropDef> props) {
        FlatDef f;
        f.floor = floor; f.door = door;
        f.plan.name = "Flat"; f.plan.flat = true;
        f.plan.rows = {"######", "#....W", "#....#", "#....W", "######"};
        f.plan.bulbs = bulbs;
        f.props = props;
        return f;
    };
    const RGB dusty{86, 78, 70}, blue{70, 90, 112}, dark{58, 50, 48}, steel{88, 108, 104}, sheet{190, 188, 176}, plum{104, 62, 78};
    b.flats = {
        // 103: night worker. Neat. The bed is made, the lamp is on, he is not in.
        room(0, 3, {{3.0f, 2.5f, 2}, {4.6f, 3.4f, 4}},
             {{ART_BED, 0, {4.5f, 1.9f}, 0.40f, blue, H_}, {ART_WARDROBE, 0, {3.0f, 1.25f}, 0.90f, wood, H_}, {ART_TABLE, 0, {3.0f, 3.4f}, 0.375f, wood, 0},
              {ART_CHAIR, 0, {3.7f, 3.4f}, 0.45f, wood, PI_}, {ART_LAMP, 0, {4.7f, 3.5f}, 0.75f, steel, 0}}),
        // 105: nobody has lived here for a long time. A sofa under dust, a table, a chair turned to the window.
        room(0, 5, {{3.0f, 2.5f, 2}},
             {{ART_SOFA, 0, {4.7f, 2.5f}, 0.425f, dusty, PI_}, {ART_TABLE, 0, {2.6f, 3.5f}, 0.375f, dusty, 0}, {ART_CHAIR, 0, {4.0f, 1.4f}, 0.45f, dusty, 0},
              {ART_CRATE, 0, {2.1f, 1.5f}, 0.22f, wood, 0.6f}}),
        // 203: Igor's flat. Sparse. A bed that has been slept in, a toolbox on the floor, one bulb that barely works.
        room(1, 3, {{3.0f, 2.5f, 2}},
             {{ART_BED, 1, {4.5f, 2.0f}, 0.40f, dark, H_}, {ART_WARDROBE, 1, {3.0f, 1.25f}, 0.90f, dark, H_}, {ART_TABLE, 1, {2.8f, 3.3f}, 0.375f, wood, 0},
              {ART_CRATE, 1, {4.6f, 3.5f}, 0.22f, steel, 0.4f}}),
        // 205: being renovated. Crates, a paint barrel, a work light that is far too bright.
        room(1, 5, {{3.0f, 2.5f, 0}},
             {{ART_CRATE, 1, {3.0f, 1.5f}, 0.30f, wood, 0.2f}, {ART_CRATE, 1, {3.7f, 1.5f}, 0.30f, wood, 0.9f}, {ART_CRATE, 1, {4.5f, 2.6f}, 0.30f, wood, 1.8f},
              {ART_TANK, 1, {4.6f, 3.5f}, 0.50f, steel, 0}, {ART_TABLE, 1, {2.5f, 3.5f}, 0.375f, sheet, 0}, {ART_CHAIR, 1, {3.3f, 3.4f}, 0.45f, sheet, H_}}),
        // 207: the television is on, with the sound off. Nobody on the sofa.
        room(1, 7, {{3.0f, 2.5f, 1}, {4.4f, 2.0f, 4}},
             {{ART_SOFA, 1, {3.2f, 3.2f}, 0.425f, plum, -0.4f}, {ART_TV, 1, {4.7f, 1.9f}, 0.42f, steel, PI_}}),
        // 208: a piano and a stool. The lamp is on. The music you can hear from the hall is a recording, or is it.
        room(1, 8, {{3.0f, 2.5f, 2}, {4.7f, 1.5f, 4}},
             {{ART_PIANO, 1, {4.6f, 2.5f}, 0.62f, wood, PI_}, {ART_CHAIR, 1, {3.7f, 2.5f}, 0.45f, wood, 0}, {ART_LAMP, 1, {4.7f, 1.4f}, 0.75f, steel, 0}}),
    };
    return b;
}
