//
// Created by Anatol on 23/06/2022.
//

#include "../headers/Snake.h"

Snake::Snake(Player *player, unsigned int id)
    :id(id), player(player)
{

}

void Snake::pushPos(Pos newHead) {
    this->body.push(newHead);
    this->head = newHead;
}

Pos Snake::retractTail() {
    Pos tail = this->body.front();
    this->body.pop();
    return tail;
}
