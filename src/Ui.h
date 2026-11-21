// Ui.h - draws everything on top of the 3D view. It reads the Game's state and never changes game rules.
#pragma once
#include "Common.h"

class Game;

class Ui {
public:
    void init();                              // creates the GPU textures (needs an open window)
    void shutdown();
    void pointerInput(Game& g, Input& in);    // mouse over/click on the dialogue menu and the journal tabs
    void draw(Game& g, int ww, int wh);       // the whole screen for this frame

private:
    void drawHud(const Game& g, Rectangle dst, int ww);
    void drawDialogue(const Game& g, int ww, int wh);
    void drawJournal(Game& g, int ww, int wh);
    void drawEnding(const Game& g, int ww, int wh);

    Texture2D sceneTex{}, paperTex{}, corkTex{};
    vector<Rectangle> menuRects;              // clickable dialogue lines (filled while drawing)
    Rectangle tabRects[4] = {};               // clickable journal tabs
};
