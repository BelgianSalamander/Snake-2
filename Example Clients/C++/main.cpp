#include <iostream>

#include "client.h"

class MyClient : public Client{
public:
    MyClient(std::string name, Color color)
        : Client(name, color) {};

protected:
    Move getMove() override {
        return Move::UP;
    }
};

int main() {
    MyClient client("Anatol", Color{255, 0, 255});

    client.run("127.0.0.1", 42069);
}
