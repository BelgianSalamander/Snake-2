//
// Created by Anatol on 23/06/2022.
//

#ifndef SNAKE_GAME_H
#define SNAKE_GAME_H

#include <set>
#include <map>
#include "Snake.h"
#include "Player.h"

#define TIMEOUT_MS 2000

#define ELO_D 400
#define ELO_K 50

enum class SquareType: char {
    EMPTY, FOOD, SNAKE
};

struct Square {
    SquareType type;
    unsigned int snakeID;

    inline bool canMoveTo() const {
        return type == SquareType::EMPTY || type == SquareType::FOOD;
    }

    static Square empty() {
        return {SquareType::EMPTY, 0};
    }

    static Square food() {
        return {SquareType::FOOD, 0};
    }

    static Square snake(unsigned int id) {
        return {SquareType::SNAKE, id};
    }

    bool operator<(const Square& other) const {
        return std::tie(type, snakeID) < std::tie(other.type, other.snakeID);
    }
};

struct GameConfig {
    unsigned int numRows, numCols;
    unsigned int numFood;

    struct SnakeConfig {
        Pos back;
        std::vector<Move> body;

        SnakeConfig(Pos back, std::vector<Move> body)
            :back(back), body(body)
        {}

        SnakeConfig(std::initializer_list<Pos> body);

        SnakeConfig()
            :back(0, 0), body({})
        {}
    };

    std::vector<SnakeConfig> snakes;

    static GameConfig fromFile(const std::string& filename);
};

struct Changes {
    std::set<std::pair<Pos, Square>> changes;
    unsigned int newTurn;
};

class Game {
public:
    Game(GameConfig& config, std::vector<Player*>& players);
    ~Game();

    [[nodiscard]] Square getSquare(Pos pos) const;
    Square setSquare(Pos pos, Square value);

    inline bool isWithinBounds(Pos pos) {
        return pos.row < this->numRows && pos.col < this->numCols && pos.row >= 0 && pos.col >= 0;
    }

    void tryTick();

    unsigned int getNumRows() const;

    unsigned int getNumCols() const;

    bool hasGameEnded() const;
private:
    unsigned int numRows, numCols, numFood;
    unsigned int currTurn = 0;
    Square* grid;
    std::set<Pos> changes;
    std::vector<Snake> snakes;
    std::vector<Snake*> snakesDeadThisTurn;

    friend class GameDisplay;
    friend class GameCreator;

    long long lastMoveAsk;

    std::map<Snake*, float> expectedScores;

    void pushChanges();
    void requestMoves();

    void updateFood();

    [[nodiscard]] inline unsigned int idx(Pos pos) const {
        return pos.row * numCols + pos.col;
    }

    bool isTimeout() const;

    void killSnake(Snake* snake, std::string reason, bool timeout);

    void tick();

    bool canTick() const;

    void finish();
};


#endif //SNAKE_GAME_H
