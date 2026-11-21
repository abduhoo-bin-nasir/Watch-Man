// Block 7 - a pixel-art first-person detective game (C++17 + raylib 5.x). 1990s-style software renderer.
//
// Controls: W/S move, A/D strafe, Left/Right turn, PgUp/PgDn look up/down (Home = recentre), TAB = mouse look on/off,
//           E = knock / talk / examine / lift, J = detective's journal, ESC = back out / quit
//
// Where things live:
//   Game.cpp          the game's state machine and main loop        LevelData.cpp     floor plans, lights, who stands where
//   Level.cpp         turns ASCII plans into the stacked building   CharacterArt.cpp   the cast and their pixel designs
//   Entities.cpp      people, props and the player                  DialogueData.cpp  everything people say
//   ClueBook.cpp      the clues and their dependency graph          Renderer.cpp      the software renderer
//   Dialogue.cpp      conversations                                 Ui.cpp            HUD, dialogue box, journal, ending
#include "Game.h"

int main() {
    Game game;
    return game.run();
}
