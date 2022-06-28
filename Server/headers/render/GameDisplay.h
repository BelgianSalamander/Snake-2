//
// Created by Anatol on 24/06/2022.
//

#ifndef SNAKE_GAMEDISPLAY_H
#define SNAKE_GAMEDISPLAY_H

#include "Game.h"
#include "imgui.h"
#include <string>

static unsigned int counter = 1;

class GameDisplay {
public:
    GameDisplay() = default;
    GameDisplay(std::string);

    Game* game = nullptr;

    void renderWindow();
private:
    unsigned int id = counter++;
    std::string windowName = "Game " + std::to_string(id);

    void drawPlayers(ImDrawList *pList, float x, float y, float width, float height, bool horizontal);
};


#endif //SNAKE_GAMEDISPLAY_H
