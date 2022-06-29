//
// Created by Anatol on 23/06/2022.
//

#ifndef SNAKE_SNAKE_H
#define SNAKE_SNAKE_H

#include <queue>
#include "utils.h"
#include <iostream>

class Player;

class Snake {
public:
    unsigned int startSize;
    unsigned int diedOnTurn;
    unsigned int sizeOnDeath = 0;

    Snake(Player* player, unsigned int id);

    void pushPos(Pos newHead);
    [[nodiscard]] Pos retractTail();

    [[nodiscard]] inline Pos getHead() const {
        return head;
    }

    [[nodiscard]] inline size_t size() const {
        return this->body.size();
    }

    [[nodiscard]] inline Player* getPlayer() const {
        return player;
    }

    inline const std::queue<Pos>& getBody() const {
        return body;
    }

    [[nodiscard]] inline unsigned int getID() const {
        return id;
    }

    inline void kill() {
        if (sizeOnDeath == 0) {
            std::cerr << "Forgot to set sizeOnDeath" << std::endl;
        }
        this-> alive = false;
    }

    [[nodiscard]] inline bool isAlive() const {
        return alive;
    }

    [[nodiscard]] inline unsigned int getSize() const {
        if (alive) {
            return body.size();
        } else {
            return sizeOnDeath;
        }
    }
private:
    const unsigned int id;

    Pos head = {1 << 30, 1 << 30};
    std::queue<Pos> body;
    Player* player;
    bool alive = true;
};


#endif //SNAKE_SNAKE_H
