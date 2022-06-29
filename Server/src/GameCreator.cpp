//
// Created by Anatol on 24/06/2022.
//

#include "GameCreator.h"
#include <algorithm>
#include <random>
#include <iostream>

static const char* DEFAULT_CONFIG = "smallfour";

static std::string fullPath(const std::string& base) {
    return "./res/layouts/" + base + ".json";
}

GameCreator::GameCreator(unsigned int targetGameAmount)
    : config(GameConfig::fromFile(fullPath(DEFAULT_CONFIG))), targetGameAmount(targetGameAmount)
{
    this->games.resize(targetGameAmount);

    for (unsigned int i = 0; i < targetGameAmount; i++) {
        this->displays.emplace_back(GameDisplay());
    }

    memset(this->fileBuf, 0, 64);
    strcpy_s(this->fileBuf, DEFAULT_CONFIG);
}

void GameCreator::tryShrink() {
    int i = 0;
    while (i < this->games.size() && this->games.size() > this->targetGameAmount) {
        if (this->games[i] == nullptr) {
            this->games.erase(this->games.begin() + i);
        } else {
            i++;
        }
    }
}

void GameCreator::addPlayer(Player *player) {
    this->freePlayers.push_back(player);
}

void GameCreator::render() {
    for (GameDisplay& display : this->displays) {
        display.renderWindow();
    }

    //Leaderboard
    ImGui::Begin("Leaderboard");
    ImGui::Text("Leaderboard");
    ImGui::BeginChild("Leaderboard", ImVec2(0, 0), true);

    //Get all players
    std::vector<Player*> players;

    for (Game* game : this->games) {
        if (game != nullptr) {
            for (Snake& snake : game->snakes) {
                players.push_back(snake.getPlayer());
            }
        }
    }

    for (Player* player : freePlayers) {
        players.push_back(player);
    }

    //Sort players by elo
    std::sort(players.begin(), players.end(), [](Player* a, Player* b) {
        return a->getElo() > b->getElo();
    });

    //Render players
    int totalScore = 0;
    for (Player* player : players) {
        ImGui::PushStyleColor(ImGuiCol_Text, (uint32_t) player->getColor());
        ImGui::Text("%s: %d", player->getName().c_str(), player->getElo());
        ImGui::PopStyleColor();

        totalScore += player->getElo();
    }

    ImGui::EndChild();
    ImGui::End();

    //Options menu
    ImGui::Begin("Options");

    //Text entry for layout path
    ImGui::InputText("Layout path", this->fileBuf, 64);

    if (ImGui::Button("Update")) {
        try {
            this->config = GameConfig::fromFile(fullPath(this->fileBuf));
        } catch (std::runtime_error& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }

    ImGui::End();

    //std::cout << "Total score: " << totalScore << std::endl;
}

void GameCreator::tick() {
    for (int i = 0; i < this->freePlayers.size(); i++) {
        if (this->freePlayers[i]->kicked) {
            this->freePlayers[i]->onRemoved();
            this->freePlayers.erase(this->freePlayers.begin() + i);
            i--;
        }
    }

    for (auto & game : this->games) {
        if (game != nullptr) {
            if (game->hasGameEnded()) {
                game->finish();

                for (Snake& snake : game->snakes) {
                    if (snake.getPlayer()->kicked) {
                        snake.getPlayer()->onRemoved();
                    } else {
                        snake.getPlayer()->inGame = false;
                        this->freePlayers.push_back(snake.getPlayer());
                    }
                }

                for (GameDisplay& display : this->displays) {
                    if (display.game == game) {
                        display.game = nullptr;
                    }
                }

                delete game;
                game = nullptr;
                currentGameAmount--;
            } else {
                game->tryTick();
            }
        }
    }

    tryShrink();

    tryMakeNewGame();
}

void GameCreator::tryMakeNewGame() {
    while (this->currentGameAmount < this->targetGameAmount) {
        if (this->freePlayers.size() >= this->config.snakes.size()) {
            std::vector<int> indices;

            for (int i = 0; i < this->freePlayers.size(); i++) {
                indices.push_back(i);
            }

            std::shuffle(indices.begin(), indices.end(), rng);

            std::vector<Player*> players;

            for (int i = 0; i < this->config.snakes.size(); i++) {
                players.push_back(this->freePlayers[indices[i]]);
            }

            std::sort(indices.begin(), indices.begin() + this->config.snakes.size(), std::greater<int>());
            for (int i = 0; i < this->config.snakes.size(); i++) {
                this->freePlayers.erase(this->freePlayers.begin() + indices[i]);
            }

            unsigned int freeGameIndex = 0;

            while (freeGameIndex < this->games.size() && this->games[freeGameIndex] != nullptr) {
                freeGameIndex++;
            }

            assert(freeGameIndex < this->games.size());

            this->games[freeGameIndex] = new Game(this->config, players);

            for (GameDisplay& display: displays) {
                if (display.game == nullptr) {
                    display.game = this->games[freeGameIndex];
                    break;
                }
            }

            currentGameAmount++;
        } else {
            break;
        }
    }
}

std::string GameCreator::getPlayerName(std::string name) {
    std::set<std::string> allNames;

    for (Game* game : this->games) {
        if (game != nullptr) {
            for (Snake& snake : game->snakes) {
                allNames.insert(snake.getPlayer()->getName());
            }
        }
    }

    for (Player* player : this->freePlayers) {
        allNames.insert(player->getName());
    }

    if (allNames.find(name) == allNames.end()) {
        return name;
    }

    int i = 1;

    while (true) {
        std::string newName = name + " " + std::to_string(i);
        if (allNames.find(newName) == allNames.end()) {
            return newName;
        }
        i++;
    }
}

uint8_t addCapped(uint8_t n, int m) {
    if (n + m > 255) return 255;
    if (n + m < 0) return 0;
    return n + m;
}

int mod(int n, int m) {
    int r = n % m;
    if (r < 0) r += m;
    return r;
}

Color GameCreator::getPlayerColor(Color color) {
    while (abs(color.r - 26) + abs(color.g - 26) + abs(color.b - 26) < 50) {
        color = {static_cast<uint8_t>(rng() % 255), static_cast<uint8_t>(rng() % 255), static_cast<uint8_t>(rng() % 255)};
    }

    return color;
}
