// LevelData.cpp - THE LEVEL DESIGN. Edit this file to change the building.
//
// Every floor is an ASCII plan, one character per tile (a tile = 2x2 cells = about one doorway wide). Both floors have
// the same size and sit exactly on top of each other. Legend:
//     #  wall               W  window (outer wall)       .  walkable floor (hallway, landings)
//     k  stair handrail     m  metal (lift cabin walls)  1-8  apartment doors (101..108 / 201..208)
//     E  lift doors         L  lift cabin                u / d  staircase tiles (only for reading: see "flights")
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
    b.floors = {g, u};

    // ------------------------------------------------------------------ flights (staircases)
    // One entry per physical staircase. It is laid into BOTH floors it connects, so the floors always agree.
    // {first step tile x, y, direction of the climb, lower floor (-1 = basement)}
    b.flights = {
        {8, 3, DIR_S, 0},     // ground <-> second floor: climbs south along x=8 (entered from the north hallway downstairs)
        {10, 7, DIR_S, -1},   // basement -> ground floor: the DOWN flight on the ground floor (ends at a locked boiler door)
        {10, 3, DIR_S, 1}};   // second floor -> roof: the UP flight on the second floor (ends at a locked roof door)

    // ------------------------------------------------------------------ you
    b.startFloor = 0; b.start = {2.5f, 1.5f}; b.startYaw = 0;

    // ------------------------------------------------------------------ people in the hallways. id = see the cast list in CharacterArt.cpp
    // {id, floor, tile position, patrol route (empty = stands still), tiles per second, facing in radians}
    b.characters = {
        {"scarf", 0, {17.5f, 1.5f}, {{17.5f, 1.5f}, {17.5f, 13.5f}, {1.5f, 13.5f}, {1.5f, 1.5f}}, 0.8f, 0},
        {"cat", 0, {4.5f, 13.5f}, {{4.5f, 13.5f}, {11.5f, 13.5f}}, 0.5f, 0},
        {"pavel", 1, {17.5f, 9.5f}, {}, 0, 3.14159265f},
        {"tenant", 1, {3.0f, 13.5f}, {}, 0, -1.5707963f}};

    // ------------------------------------------------------------------ who lives behind which door: they answer when you knock
    // {id, floor, door number (1 = apartment x01)}
    b.homes = {{"volkova", 0, 1}, {"gromov", 0, 2}, {"misha", 0, 4}, {"petrova", 0, 7}};

    // ------------------------------------------------------------------ props (no characters): {art, floor, tile position, height, colour, facing}
    const RGB wood{150, 112, 76};
    b.props = {{ART_CRATE, 0, {15.5f, 13.5f}, 0.3f, wood, 0.35f}, {ART_CRATE, 0, {1.5f, 8.5f}, 0.3f, wood, 1.7f},
               {ART_CRATE, 1, {17.5f, 3.5f}, 0.3f, wood, 3.0f}};
    return b;
}
