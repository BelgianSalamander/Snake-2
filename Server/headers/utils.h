//
// Created by Anatol on 23/06/2022.
//

#ifndef SNAKE_UTILS_H
#define SNAKE_UTILS_H

#include <cstdint>
#include <random>
#include <cassert>

#define MIN_T(a, b) ((a) < (b) ? (a) : (b))

static std::mt19937 rng(std::random_device{}());

struct Color {
    uint8_t r, g, b;

    //Color as uint32
    explicit operator uint32_t() const {
        return (0xff << 24) | (b << 16) | (g << 8) | r;
    }
};

static Color COLORS[] = {
        {0xff, 0x00, 0x00}, //RED
        {0x00, 0xff, 0x00}, //GREEN
        {0x00, 0x00, 0xff}, //BLUE
        {0xff, 0x00, 0xff}, //PURPLE
        {0x00, 0xff, 0xff}, //CYAN
        {252, 3, 240}, //PINK
        {252, 136, 3}, //ORANGE
        {3, 252, 240}, //TEAL
        {16, 81, 194}, //BLUE_DARK
        {0xff, 0xff, 0xff}, //WHITE
};

enum Move : uint8_t {
    UP = 0,
    RIGHT = 1,
    DOWN = 2,
    LEFT = 3,
};

static std::pair<Move, Move> perp(Move m) {
    switch (m) {
        case UP:
            return {LEFT, RIGHT};
        case RIGHT:
            return {DOWN, UP};
        case DOWN:
            return {RIGHT, LEFT};
        case LEFT:
            return {UP, DOWN};
    }

    assert(false);
}

const Move ALL_MOVES[4] = {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

const std::pair<int, int> MOVES[4] = {
        {-1, 0},
        {0, 1},
        {1, 0},
        {0, -1}
};

/*
 * Board:
 *                 col
 *         0 1 2 3 4 5 6 7 8 9
 *        0
 *        1
 *        2
 *        3
 *      r 4
 *      o 5
 *      w 6
 *        7
 *        8
 *        9
 */
struct Pos {
    union {
        unsigned int row;
        unsigned int y;
    };

    union {
        unsigned int col;
        unsigned int column;
        unsigned int x;
    };

    Pos(const unsigned int row, const unsigned int col) {
        this->row = row;
        this->col = col;
    }

    bool operator<(const Pos& other) const {
        if (this->row != other.row) {
            return this->row < other.row;
        } else {
            return this->col < other.col;
        }
    }

    [[nodiscard]] Pos moved(Move move) const {
        return {row + MOVES[move].first, col + MOVES[move].second};
    }

    [[nodiscard]] Pos moved(Move move, int amount) const {
        return {row + MOVES[move].first * amount, col + MOVES[move].second * amount};
    }

    Pos operator+(const Move move) const {
        return this->moved(move);
    }
};

static Move getMove(Pos from, Pos to) {
    if (from.row == to.row) {
        if (from.col < to.col) {
            return RIGHT;
        } else {
            return LEFT;
        }
    } else {
        if (from.row < to.row) {
            return DOWN;
        } else {
            return UP;
        }
    }
}

#endif //SNAKE_UTILS_H
