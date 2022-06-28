#include <iostream>

#include "client.h"

#include <vector>
#include <random>

class MyClient : public Client{
public:
    MyClient(std::string name, Color color)
        : Client(name, color) {};

protected:
    bool isMoveSafe(Move move) {
        Pos newHead = head() + move;

        if (!isWithinBounds(newHead)) {
            return false;
        }

        return getSquare(newHead).canMoveTo();
    }

    Move getMove() override {
        std::vector<Move> moves;

        for (Move move: ALL_MOVES) {
            if (isMoveSafe(move)) {
                moves.push_back(move);
            }
        }

        if (moves.size() == 0) {
            return UP;
        } else {
            return moves[rand() % moves.size()];
        }
    }
};

int main() {
    MyClient client("Anatol", Color{255, 0, 255});

    client.run("127.0.0.1", 42069);
}
