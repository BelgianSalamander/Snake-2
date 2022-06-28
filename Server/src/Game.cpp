//
// Created by Anatol on 23/06/2022.
//

#include "../headers/Game.h"
#include <iostream>
#include <chrono>
#include <cassert>
#include <map>

#include <json/json.hpp>
#include <fstream>

// Multiplayer ELO is based on https://towardsdatascience.com/developing-a-generalized-elo-rating-system-for-multiplayer-games-b9b495e87802

static Move getMove(std::string basicString);

Game::Game(GameConfig& config, std::vector<Player*>& players)
        :numRows(config.numRows),
        numCols(config.numCols),
        numFood(config.numFood)
{
    this->grid = new Square[numRows * numCols];

    for (int i = 0; i < numRows * numCols; i++) {
        this->grid[i].type = SquareType::EMPTY;
    }

    for (Player* player: players) {
        assert(!player->inGame);
        player->inGame = true;
    }

    //Shuffle players
    std::shuffle(players.begin(), players.end(), rng);

    if (players.size() != config.snakes.size()) {
        std::cerr << "Number of players does not match number of snakes" << std::endl;
    }

    for (unsigned int i = 0; i < MIN_T(players.size(), config.snakes.size()); i++) {
        this->snakes.emplace_back(Snake{players[i], i});
        Snake& snake = this->snakes.back();

        Pos p = config.snakes[i].back;
        this->setSquare(p, Square::snake(i));
        snake.pushPos(p);

        for (Move move : config.snakes[i].body) {
            p = p + move;
            this->setSquare(p, Square::snake(i));

            snake.pushPos(p);
        }

        snake.startSize = snake.getBody().size();
    }

    updateFood();

    for (Snake& snake : this->snakes) {
        snake.getPlayer()->beginGame(*this, snake);
    }

    //Calculate expected scores
    for (Snake& snake: this->snakes) {
        int currScore = snake.getPlayer()->getElo();
        float total = 0;

        for (Snake& otherSnake: this->snakes) {
            if (snake.getID() == otherSnake.getID()) {
                continue;
            }

            total += 1.0f / (1.f + std::pow(10.f, (otherSnake.getPlayer()->getElo() - currScore) / ELO_D));
        }

        total /= this->snakes.size() * (this->snakes.size() - 1) / 2;

        this->expectedScores[&snake] = total;
    }

    pushChanges();
    requestMoves();
}

Game::~Game() {
    delete[] this->grid;
}

Square Game::getSquare(Pos pos) const {
    return this->grid[this->idx(pos)];
}

Square Game::setSquare(Pos pos, Square value) {
    Square old = this->grid[this->idx(pos)];
    this->grid[this->idx(pos)] = value;
    this->changes.insert(pos);

    return old;
}

void Game::updateFood() {
    int currentFoodN = 0;
    std::vector<Pos> empty;

    for (unsigned int row = 0; row < this->numRows; row++) {
        for (unsigned int col = 0; col < this->numCols; col++) {
            Pos pos = {row, col};
            if (this->getSquare(pos).type == SquareType::FOOD) {
                currentFoodN++;
            } else if (this->getSquare(pos).type == SquareType::EMPTY) {
                empty.push_back(pos);
            }
        }
    }

    if (currentFoodN < this->numFood) {
        if (currentFoodN == this->numFood - 1) {
            this->setSquare(empty[rng() % empty.size()], Square::food());
        } else {
            std::shuffle(empty.begin(), empty.end(), rng);

            for (int i = 0; i < this->numFood - currentFoodN; i++) {
                this->setSquare(empty[i], Square::food());
            }
        }
    }
}

void Game::requestMoves() {
    for (Snake& snake : this->snakes) {
        if (snake.isAlive()) {
            snake.getPlayer()->askForNextMove(*this, snake);
        }
    }

    using namespace std::chrono;
    this->lastMoveAsk = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void Game::pushChanges() {
    Changes changesToBroadcast;

    for (Pos pos : this->changes) {
        changesToBroadcast.changes.insert({pos, this->getSquare(pos)});
    }

    changesToBroadcast.newTurn = this->currTurn;

    for (Snake& snake : this->snakes) {
        if (snake.isAlive()) {
            snake.getPlayer()->receiveChanges(*this, snake, changesToBroadcast);
        }
    }
}

void Game::tryTick() {
    if (!canTick()) {
        return;
    }

    bool receivedAllMoves = true;

    std::set<Snake*> deadSnakes;

    for (Snake& snake : this->snakes) {
        if (snake.isAlive() && !snake.getPlayer()->nextMove().has_value()) {
            receivedAllMoves = false;
            deadSnakes.insert(&snake);
        }
    }

    if (receivedAllMoves || isTimeout()) {
        this->snakesDeadThisTurn.clear();

        for (Snake* snake: deadSnakes) {
            killSnake(snake, std::string("Didn't receive move after ") + std::to_string(TIMEOUT_MS) + "ms", true);
        }

        tick();
    }
}

void Game::tick() {
    std::map<Pos, std::vector<Snake*>> targets;

    for (Snake& snake : this->snakes) {
        if (snake.isAlive()) {
            assert(snake.getPlayer()->nextMove().has_value());

            Move move = snake.getPlayer()->nextMove().value();
            targets[snake.getHead() + move].push_back(&snake);
        }
    }

    for (auto entry: targets) {
        if (entry.second.size() > 1) {
            for (Snake* snake : entry.second) {
                killSnake(snake, "Collision with other snake's head", false);
            }
        } else {
            Snake* snake = entry.second[0];
            if (!isWithinBounds(entry.first)) {
                this->setSquare(snake->retractTail(), Square::empty());
            } else if (this->getSquare(entry.first).type != SquareType::FOOD) {
                this->setSquare(snake->retractTail(), Square::empty());
            }
        }
    }

    for (auto entry: targets) {
        if (entry.second.size() == 1) {
            Snake* snake = entry.second[0];
            Pos target = entry.first;

            if (!isWithinBounds(target)) {
                killSnake(snake, "Out of bounds", false);
                continue;
            }

            Square square = this->getSquare(target);

            if (!square.canMoveTo()) {
                if (square.snakeID == snake->getID()) {
                    killSnake(snake, "Tried to move to own body", false);
                } else {
                    killSnake(snake, "Tried to move to other snake's body", false);
                }
                continue;
            }

            snake->pushPos(target);
            this->setSquare(target, Square::snake(snake->getID()));
        }
    }

    updateFood();

    this->currTurn++;

    pushChanges();
    requestMoves();
}

bool Game::isTimeout() const {
    using namespace std::chrono;
    auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    return now.count() - this->lastMoveAsk > TIMEOUT_MS;
}

bool Game::canTick() const {
    using namespace std::chrono;
    auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    return now.count() - this->lastMoveAsk > 80;
}

void Game::killSnake(Snake *snake, std::string reason, bool timeout) {
    if (!snake->isAlive()) {
        std::cerr << "Tried to kill a dead snake" << std::endl;
        return;
    }
    std::cout << "Killing snake " << snake->getPlayer()->getName() << ": " << reason << std::endl;

    snake->kill();
    snake->getPlayer()->died(*this, *snake, reason, timeout);
    snake->diedOnTurn = this->currTurn;

    std::queue<Pos> snakeBody = snake->getBody();

    while (!snakeBody.empty()) {
        Pos pos = snakeBody.front();
        snakeBody.pop();
        this->setSquare(pos, Square::empty());
    }

    this->snakesDeadThisTurn.emplace_back(snake);
}

//Linear
float getScore(int rank, int numSnakes) {
    return (numSnakes - rank) / (float) (numSnakes * (numSnakes - 1) / 2);
}

unsigned int Game::getNumRows() const {
    return numRows;
}

unsigned int Game::getNumCols() const {
    return numCols;
}

bool Game::hasGameEnded() const {
    int snakesAlive = 0;

    for (const Snake& s : this->snakes) {
        if (s.isAlive()) {
            snakesAlive++;
        }
    }

    return snakesAlive <= 1;
}

int snakeSizeScore(const Snake& snake) {
    return snake.getBody().size() - snake.startSize + (snake.isAlive() ? 10 : 0);
}

void Game::finish() {
    std::vector<Snake*> snakePtrs;

    for (Snake& snake : this->snakes) {
        snakePtrs.push_back(&snake);
        if (snake.isAlive()) {
            snake.diedOnTurn = 1 << 30;
        }
    }

    std::sort(snakePtrs.begin(), snakePtrs.end(), [](Snake* a, Snake* b) {
        int aScore = snakeSizeScore(*a);
        int bScore = snakeSizeScore(*b);

        if (aScore == bScore) {
            return a->diedOnTurn > b->diedOnTurn;
        } else {
            return aScore > bScore;
        }
    });

    std::vector<float> scores;

    for (int i = 0; i < snakePtrs.size(); i++) {
        scores.push_back(getScore(i + 1, snakePtrs.size()));
    }

    //Average score of snakes with same size score + diedOnTurn

    int backPtr = 0;
    int frontPtr = 0;
    float totalScore = 0;
    int numSnakes = 0;
    while (frontPtr < snakePtrs.size()) {
        if (snakeSizeScore(*snakePtrs[frontPtr]) == snakeSizeScore(*snakePtrs[backPtr]) && snakePtrs[frontPtr]->diedOnTurn == snakePtrs[backPtr]->diedOnTurn) {
            totalScore += scores[frontPtr];
            numSnakes++;
            frontPtr++;
        } else {
            float score = totalScore / numSnakes;

            for (int i = backPtr; i < frontPtr; i++) {
                scores[i] = score;
            }

            backPtr = frontPtr;
            totalScore = 0;
            numSnakes = 0;
        }
    }

    float score = totalScore / numSnakes;

    for (int i = backPtr; i < frontPtr; i++) {
        scores[i] = score;
    }

    int totalChange = 0;
    std::vector<int> changes;
    for (int i = 0; i < snakePtrs.size(); i++) {
        float score = scores[i];
        float expectedScore = expectedScores[snakePtrs[i]];
        float scoreDiff = score - expectedScore;

        int change = ((int) (scoreDiff * ELO_K * (snakePtrs.size() - 1)));
        totalChange += change;

        changes.push_back(change);
    }

    //Sometimes because of rounding, the total change is not exactly zero. In that case we just punish the bottom or reward the top
    if (totalChange < 0) {
        changes[0] -= totalChange;
    } else if (totalChange > 0) {
        changes[changes.size() - 1] -= totalChange;
    }

    for (int i = 0; i < snakePtrs.size(); i++) {
        Snake* snake = snakePtrs[i];
        snake->getPlayer()->setElo(snake->getPlayer()->getElo() + changes[i]);
    }
}
/*
void Game::updateScores() {
    int numAliveSnakes = 0;

    for (Snake& snake : this->snakes) {
        if (snake.isAlive()) {
            numAliveSnakes++;
        }
    }

    float actualScore = 0;

    //Take average of scores for ties
    for (Snake* snake: this->snakesDeadThisTurn) {
        actualScore += getScore(++numAliveSnakes, this->snakes.size());
    }

    actualScore /= this->snakesDeadThisTurn.size();

    for (Snake* snake: this->snakesDeadThisTurn) {
        float expectedScore = this->expectedScores[snake];
        float excess = actualScore - expectedScore;
        float change = ELO_K * (this->snakes.size() - 1) * excess;

        snake->getPlayer()->setElo(snake->getPlayer()->getElo() + change);
    }
}
*/

GameConfig::SnakeConfig::SnakeConfig(std::initializer_list<Pos> body)
    :back({0, 0})
{
    assert(body.size() > 0);

    this->back = *body.begin();

    auto itFrom = body.begin(), itTo = body.begin() + 1;

    for (int i = 0; i < body.size() - 1; i++) {
        this->body.push_back(getMove(*itFrom, *itTo));
        itFrom++;
        itTo++;
    }
}

GameConfig GameConfig::fromFile(const std::string& filename) {
    using namespace nlohmann;

    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + filename);
    }

    json root;
    file >> root;

    unsigned int numRows = root["num_rows"];
    unsigned int numCols = root["num_cols"];
    unsigned int numFood = root["num_food"];

    std::vector<GameConfig::SnakeConfig> snakes;

    for (json snakeJson : root["snakes"]) {
        SnakeConfig snakeConfig;

        snakeConfig.back = {snakeJson["back"][0], snakeJson["back"][1]};

        for (std::string moveName: snakeJson["body"]) {
            snakeConfig.body.push_back(getMove(moveName));
        }

        snakes.push_back(snakeConfig);
    }

    return {numRows, numCols, numFood, snakes};
}

static Move getMove(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (name == "up" || name == "u" || name == "north" || name == "n") {
        return Move::UP;
    } else if (name == "down" || name == "d" || name == "south" || name == "s") {
        return Move::DOWN;
    } else if (name == "left" || name == "l" || name == "west" || name == "w") {
        return Move::LEFT;
    } else if (name == "right" || name == "r" || name == "east" || name == "e") {
        return Move::RIGHT;
    } else {
        throw std::runtime_error("Invalid move name: " + name);
    }
}
