//
// Created by Anatol on 24/06/2022.
//

#ifndef SNAKE_GAMECREATOR_H
#define SNAKE_GAMECREATOR_H

#include "Game.h"
#include "render/GameDisplay.h"

class GameCreator {
public:
    GameCreator(unsigned int targetGameAmount);

    void addPlayer(Player* player);

    void tick();

    void render();

    std::string getPlayerName(std::string basicString);

    Color getPlayerColor(Color color);

private:
    GameConfig config;
    std::vector<Player*> freePlayers;

    std::vector<GameDisplay> displays;
    std::vector<Game*> games;

    unsigned int targetGameAmount;
    unsigned int currentGameAmount = 0;

    //Config state
    char fileBuf[64];

    void tryShrink();

    void tryMakeNewGame();
};


#endif //SNAKE_GAMECREATOR_H
