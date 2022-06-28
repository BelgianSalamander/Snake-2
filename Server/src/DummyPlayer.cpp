//
// Created by Anatol on 23/06/2022.
//

#include "../headers/DummyPlayer.h"
#include "../headers/Snake.h"
#include "../headers/Game.h"

#include <iostream>

DummyPlayer::DummyPlayer(std::string name, Color color)
        :Player(color, name)
{}

DummyPlayer::~DummyPlayer() = default;

void DummyPlayer::prepareNextMove(Game &game, Snake& snake) {
    std::vector<Move> moves;

    for (Move move: ALL_MOVES) {
        if (isMoveSafe(move, game, snake)) {
            moves.push_back(move);
        }
    }

    if (moves.size() == 0) {
        this->savedMove = std::optional<Move>(UP);
    } else {
        std::shuffle(moves.begin(), moves.end(), rng);
        this->savedMove = std::optional<Move>(moves[0]);
    }

}

bool DummyPlayer::isMoveSafe(Move move, Game &game, Snake &snake) {
    Pos newHead = snake.getHead() + move;

    if (!game.isWithinBounds(newHead)) {
        return false;
    }

    return game.getSquare(newHead).canMoveTo();
}

void DummyPlayer::onDeath(Game &game, Snake &snake, std::string reason, bool timeout) {
    std::cout << this->getName() << " died. Reason: " << reason << std::endl;
}
