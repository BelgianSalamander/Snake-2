//
// Created by Anatol on 25/06/2022.
//

#ifndef SNAKE_EXAMPLE_CLIENT_CLIENT_H
#define SNAKE_EXAMPLE_CLIENT_CLIENT_H

#include <string>

#include <WS2tcpip.h>
#include <winsock.h>
#include <iostream>

#pragma comment (lib, "ws2_32.lib")

enum Move : uint8_t {
    UP = 0,
    RIGHT = 1,
    DOWN = 2,
    LEFT = 3,
};

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

struct Color {
    uint8_t r, g, b;
};

enum class SquareType: char {
    EMPTY, FOOD, SNAKE
};

struct Square {
    SquareType type;
    unsigned int snakeID;

    [[nodiscard]] inline bool canMoveTo() const {
        return type == SquareType::EMPTY || type == SquareType::FOOD;
    }
};

enum InwardBoundPacketType: uint8_t {
    CONNECTION_ESTABLISHED = 0,
    MOVE_REQUEST = 1,
    GAME_CHANGES = 2,
    GAME_START = 3,
};

enum OutwardBoundPacketType: uint8_t {
    NAME_AND_COLOR,
    MOVE_RESPONSE
};

bool initSock = false;
static void initWinsock() {
    if (initSock) {
        return;
    }
    initSock = true;
    WSADATA wsaData;
    WSAStartup(MAKEWORD(1, 1), &wsaData);
}

class Client {
public:
    Client(std::string name, Color color)
        : name(name), color(color) {};

    ~Client() {
        if (board) {
            delete[] board;
        }
    }

    void run(std::string ip, unsigned int port) {
        initWinsock();

        sock = 0;

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            std::cerr << "Socket creation failed" << std::endl;
            return;
        }

        sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address" << std::endl;
            return;
        }

        int client_fd = connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr));

        if (client_fd < 0) {
            std::cerr << "Connection failed" << std::endl;
            return;
        }

        //Send name and color
        char buf[256];
        short packetBodyLength = 3 + name.size();

        //Header
        buf[0] = packetBodyLength & 0xFF;
        buf[1] = packetBodyLength >> 8;
        buf[2] = NAME_AND_COLOR;
        buf[3] = 0;

        //Color
        memcpy(buf + 4, &color.r, 1);
        memcpy(buf + 5, &color.g, 1);
        memcpy(buf + 6, &color.b, 1);

        //Name
        memcpy(buf + 7, name.c_str(), name.size());

        send(sock, buf, packetBodyLength + 4, 0);


        //Await confirmation
        int len = recv(sock, buf, 4, 0);

        if (len != 4) {
            std::cerr << "Failed to receive confirmation header" << std::endl;
            return;
        }

        if (buf[2] != CONNECTION_ESTABLISHED) {
            std::cerr << "Connection failed" << std::endl;
            return;
        }

        std::cout << "Connected!" << std::endl;

        //Now just loop over packets
        while (true) {
            len = recv(sock, buf, 4, 0);

            if (len != 4) {
                std::cerr << "Failed to receive header" << std::endl;
                return;
            }

            auto* header = (unsigned char*)buf;
            unsigned int packetLen = ((unsigned int) header[0]) | (((unsigned int) header[1]) << 8);
            int packetType = header[2];

            //std::cout << "Received packet of type " << packetType << std::endl;

            if (packetLen > 16384) {
                std::cerr << "Packet too large" << std::endl;
                continue;
            }

            char* packetBody = new char[packetLen];

            if (packetLen != 0) {
                len = recv(sock, packetBody, packetLen, 0);

                if (len != packetLen) {
                    std::cerr << "Failed to receive packet" << std::endl;
                    continue;
                }
            }

            handle(packetType, packetBody, packetLen);

            delete[] packetBody;
        }
    }
protected:
    std::string name;
    Color color;

    //Game data
    unsigned int numRows, numCols;
    unsigned int selfID;

    unsigned int headRow, headCol;
    unsigned int currTurn;

    virtual Move getMove() = 0;

    Square getSquare(unsigned int row, unsigned int col) {
        unsigned int idx = row * numCols + col;
        return board[idx];
    }

    Square getSquare(Pos pos) {
        return getSquare(pos.row, pos.col);
    }

    Square setSquare(unsigned int row, unsigned int col, Square value) {
        unsigned int idx = row * numCols + col;
        Square old = board[idx];
        board[idx] = value;
        return old;
    }

    Square setSquare(Pos pos, Square value) {
        return setSquare(pos.row, pos.col, value);
    }

    Pos head() {
        return {headRow, headCol};
    }

    bool isWithinBounds(Pos pos) {
        return pos.row < numRows && pos.col < numCols && pos.row >= 0 && pos.col >= 0;
    }
private:
    SOCKET sock;

    //Game data
    Square* board = nullptr;

    void handle(int type, char* data, int len) {
        switch (type) {
            case GAME_START:
                handleGameStart(data, len);
                break;
            case GAME_CHANGES:
                handleGameChanges(data, len);
                break;
            case MOVE_REQUEST:
                handleMoveRequest(data, len);
                break;
            default:
                std::cerr << "Unknown packet type" << std::endl;
                break;
        }
    }

    void handleGameStart(char *data, int len) {
        numRows = *(unsigned *)data;
        numCols = *(unsigned *)(data + 4);
        selfID = *(unsigned *)(data + 8);

        if (board) {
            delete[] board;
        }

        board = new Square[numRows * numCols];

        for (unsigned int i = 0; i < numRows * numCols; i++) {
            board[i].type = SquareType::EMPTY;
            board[i].snakeID = 0;
        }
    }

    void handleGameChanges(char *data, int len) {
        headRow = *(unsigned *)(data + 0);
        headCol = *(unsigned *)(data + 4);
        currTurn = *(unsigned *)(data + 8);

        int idx = 12;

        while (idx < len) {
            unsigned int row = *(unsigned *)(data + idx);
            unsigned int col = *(unsigned *)(data + idx + 4);
            SquareType type = (SquareType)data[idx + 8];

            if (type == SquareType::SNAKE) {
                unsigned int snakeID = *(unsigned *)(data + idx + 9);
                setSquare(row, col, Square{(SquareType) type, snakeID});
                idx += 13;
            } else {
                setSquare(row, col, Square{(SquareType) type});
                idx += 9;
            }
        }

        if (idx != len) {
            std::cerr << "Packet length mismatch" << std::endl;
        }
    }

    void handleMoveRequest(char *data, int len) {
        if (len != 0) {
            std::cerr << "Packet length mismatch" << std::endl;
            return;
        }

        unsigned char move = getMove();

        char buf[5];

        buf[0] = 1;
        buf[1] = 0;
        buf[2] = MOVE_RESPONSE;
        buf[3] = 0;

        buf[4] = move;

        send(sock, buf, 5, 0);
    }
};

#endif //SNAKE_EXAMPLE_CLIENT_CLIENT_H
