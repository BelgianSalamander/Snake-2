//
// Created by Anatol on 23/06/2022.
//

#ifndef SNAKE_DUMMYPLAYER_H
#define SNAKE_DUMMYPLAYER_H

#include <string>
#include "Player.h"

class DummyPlayer : public Player{
public:
    DummyPlayer(std::string name, Color color);
    ~DummyPlayer() override;

    void prepareNextMove(Game &game, Snake& snake) override;
    std::optional<Move> queryNextMove() override {
        return savedMove;
    }

    void onDeath(Game &game, Snake &snake, std::string reason, bool timeout) override;

private:
    static bool isMoveSafe(Move move, Game& game, Snake& snake);

    std::optional<Move> savedMove;
};


#endif //SNAKE_DUMMYPLAYER_H
