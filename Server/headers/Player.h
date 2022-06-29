//
// Created by Anatol on 23/06/2022.
//

#ifndef SNAKE_PLAYER_H
#define SNAKE_PLAYER_H

#include <cstdint>
#include <utility>
#include <optional>
#include "utils.h"

class Game;
class Snake;
class Changes;

class Player {
public:
    bool inGame = false;
    bool kicked = false;

    explicit Player(Color color, std::string name)
        :color(color),
        name(name)
    {}

    virtual ~Player() = default;

    // Special actions
    virtual void beginGame(Game& game, Snake& snake) {

    }

    void died(Game& game, Snake& snake, std::string reason, bool timeout) {
        this->kicked = timeout;
        onDeath(game, snake, reason, timeout);
    }

    //Per turn
    virtual void receiveChanges(Game& game, Snake& snake, Changes& changes) {}

    //Means players can't change their moves once they've given one
    std::optional<Move> nextMove() {
        if (savedMove.has_value()) {
           return savedMove;
        } else {
            return savedMove = queryNextMove();
        }
    }

    void askForNextMove(Game& game, Snake& snake) {
        savedMove = std::nullopt;
        prepareNextMove(game, snake);
    }

    virtual void endGame(Game& game, Snake &snake, bool died, unsigned int length, int score, unsigned int diedOn, unsigned int rank, unsigned int numTies, int newElo){}
    virtual void onRemoved() {}

    inline Color getColor() {
        return color;
    }

    inline int getElo() const {
        return elo;
    }

    virtual void setElo(int elo) {
        this->elo = elo;
    }

    inline std::string getName() const {
        return name;
    }
protected:
    virtual void prepareNextMove(Game& game, Snake& snake) = 0;
    virtual std::optional<Move> queryNextMove() = 0;
    virtual void onDeath(Game& game, Snake& snake, std::string reason, bool timeout){}
private:
    Color color;
    std::optional<Move> savedMove;
    std::string name;

    int elo = 1000;
};


#endif //SNAKE_PLAYER_H
