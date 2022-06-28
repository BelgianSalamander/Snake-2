#include <iostream>

#include "Game.h"
#include "DummyPlayer.h"
#include "render/GameDisplay.h"
#include "GameCreator.h"
#include "render/ImGuiRenderer.h"
#include "network/snake_network.h"

class MyRenderer: public ImGuiRenderer {
public:
    GameCreator* gameCreator;
    ConnectionManager* connectionManager;

    MyRenderer(GameCreator* gameCreator, ConnectionManager* connectionManager) {
        this->gameCreator = gameCreator;
        this->connectionManager = connectionManager;
    }
protected:
    void render() override {
        this->gameCreator->tick();
        this->connectionManager->tick();
        this->gameCreator->render();
    }
};

int main() {
    std::string ip;

    std::cout << "Enter ip: ";
    std::cin >> ip;

    GameCreator gameCreator(2);
    ConnectionManager connectionManager(ip.c_str(), &gameCreator);
    MyRenderer renderer(&gameCreator, &connectionManager);

    renderer.init();
    renderer.mainloop();
    renderer.cleanup();
}
