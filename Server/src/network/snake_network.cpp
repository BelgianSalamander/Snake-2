//
// Created by Anatol on 25/06/2022.
//
#include "network/snake_network.h"

#include <iostream>
#include <winsock.h>

#include "utils.h"
#include "Game.h"
#include "GameCreator.h"

#pragma comment (lib, "ws2_32.lib")

bool initSock = false;

static void initWinsock() {
    if (initSock) {
        return;
    }
    initSock = true;
    WSADATA wsaData;
    WSAStartup(MAKEWORD(1, 1), &wsaData);
}

ConnectionManager::ConnectionManager(const char* ip, GameCreator* gameCreator)
    :creator(gameCreator)
{
    initWinsock();

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    inet_pton(AF_INET, ip, &(serverAddress.sin_addr));

    bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
    listen(serverSocket, SOMAXCONN);

    FD_ZERO(&listener);
    FD_SET(serverSocket, &listener);

    FD_ZERO(&allConnections);

    std::cout << "Listening on ip " << ip << " on port " << PORT << std::endl;
}

void ConnectionManager::tick() {
    //Check if there is a new connection
    fd_set  copy = listener;

    timeval timeout = { 0, 10 };

    int socketCount = select(0, &copy, nullptr, nullptr, &timeout);

    if (socketCount) {
        SOCKET sock = copy.fd_array[0];
        SOCKET client = accept(sock, nullptr, nullptr);

        if (client == INVALID_SOCKET) {
            std::cerr << "Failed to accept connection" << std::endl;
            return;
        }

        std::cout << "New connection" << std::endl;

        Connection* connection =new Connection(client, time(NULL), this);

        connections.push_back(connection);
        connectionMap[client] = connection;

        FD_SET(client, &allConnections);
    }

    for (Connection* conn: this->connections) {
        if(!conn->tick()) {
            handleDeadConnection(conn);
        }
    }

    for (auto* conn: this->connections) {
        if (!conn->player) {
            if (conn->createdAt - time(NULL) > 10) {
                handleDeadConnection(conn);
            }
        }
    }
}

ConnectionManager::~ConnectionManager() {
    closesocket(serverSocket);

    for (Connection* connection : connections) {
        closesocket(connection->socket);
        delete connection;
    }

    if (initSock) {
        WSACleanup();
        initSock = false;
    }
}


void Connection::sendData(const char* data, int len) {
    send(socket, data, len, 0);
}

Connection::Connection(SOCKET i, time_t i1, ConnectionManager* manager) {
    socket = i;
    createdAt = i1;
    this->manager = manager;
}

bool Connection::handle(char packetType, const char* data, int len) {
    Color playerColor;
    char* name;
    char move;
    std::string playerName;

    char* p_data;
    switch (packetType) {
        case NAME_AND_COLOR:
            if (this->player != nullptr) return false;

            unsigned char color[3];
            memcpy(color, data, 3);

            playerColor = {color[0], color[1], color[2]};

            name = new char[len - 3];

            memcpy(name, data + 3, len - 3);

            playerName = std::string(name, len - 3).substr(0, 15);
            playerName = this->manager->creator->getPlayerName(playerName);
            playerColor = this->manager->creator->getPlayerColor(playerColor);

            this->player = new NetworkPlayer(playerName, playerColor, this);

            int length;
            p_data = makeConnectionEstablishedPacket(length);
            sendData(p_data, length);
            delete[] p_data;

            this->manager->creator->addPlayer(this->player);

            std::cout << "Received info from " << playerName << std::endl;

            break;
        case MOVE_RESPONSE:
            if (this->player == nullptr || len != 1) return false;

            move = data[0];

            if (move < 0 || move >= 4) {
                std::cerr << "Invalid move" << std::endl;
                return false;
            }

            this->player->receiveMove((Move) move);
            break;
        default:
            std::cerr << "Unknown packet type" << std::endl;
            return false;
    }

    return true;
}

bool Connection::tick() {
    timeval timeout = { 0, 10 };

    auto hasData = [this]() {
        fd_set test;
        FD_ZERO(&test);
        FD_SET(socket, &test);
        timeval timeout = { 0, 10 };
        select(0, &test, nullptr, nullptr, &timeout);
        return FD_ISSET(socket, &test);
    };

    while (true) {
        if (this->currState == AWAITING_HEADER) {
            if (!hasData()) break;
            int recvd = recv(socket, (char*) (this->header + this->headerReceived), 4 - this->headerReceived, 0);

            if (recvd <= 0) {
                return false;
            }

            this->headerReceived += recvd;

            if (this->headerReceived == 4) {
                this->currState = AWAITING_BODY;
                this->bodyLength = ((unsigned char*) this->header)[0] | (((unsigned char*) this->header)[1] << 8);
                this->bodyReceived = 0;

                if (this->bodyLength == 0) {
                    this->currState = COMPLETE;
                }
            }
        } else if (this->currState == AWAITING_BODY) {
            if (!hasData()) break;
            int rcvd = recv(socket, this->body + this->bodyReceived, this->bodyLength - this->bodyReceived, 0);
            if (rcvd <= 0) {
                return false;
            }

            this->bodyReceived += rcvd;

            if (this->bodyReceived == this->bodyLength) {
                this->currState = COMPLETE;
            }
        } else if (this->currState == COMPLETE) {
            char packetType = this->header[2];
            if (!this->handle(packetType, this->body, this->bodyLength)) {
                return false;
            }

            this->currState = AWAITING_HEADER;
            this->headerReceived = 0;
        }
    }

    return true;
}

NetworkPlayer::NetworkPlayer(std::string name, Color color, Connection* c)
    :Player(color, name),
    connection(c)
{

}

void NetworkPlayer::receiveMove(Move move) {
    this->receivedMove = std::optional<Move>(move);
}

void NetworkPlayer::beginGame(Game &game, Snake &snake) {
    int length;
    char* data = makeGameStartPacket(length, game, snake);

    connection->sendData(data, length);

    delete[] data;
}

void NetworkPlayer::receiveChanges(Game &game, Snake &snake, Changes &changes) {
    int length;
    char* data = makeGameChangesPacket(length, game, snake, changes);

    connection->sendData(data, length);

    delete[] data;

    data = makeWholeGridPacket(length, game);
    connection->sendData(data, length);
    delete[] data;
}

void NetworkPlayer::onRemoved() {
    //Remove connection from manager

    if (!connection->removed) {
        this->connection->manager->connections.erase(std::remove(this->connection->manager->connections.begin(),
                                                                 this->connection->manager->connections.end(),
                                                                 this->connection),
                                                     this->connection->manager->connections.end());
        this->connection->manager->connectionMap.erase(this->connection->socket);

        //Remove connection from allConnections
        FD_CLR(this->connection->socket, &this->connection->manager->allConnections);

        //Close socket
        closesocket(this->connection->socket);
    }

    //Delete connection
    delete this->connection;

    //Delete player
    delete this;
}

void ConnectionManager::handleDeadConnection(Connection* conn) {
    std::cout << "Connection closed" << std::endl;
    FD_CLR(conn->socket, &allConnections);
    connections.erase(std::remove(connections.begin(), connections.end(), conn), connections.end());
    connectionMap.erase(conn->socket);
    closesocket(conn->socket);

    if (!conn->player) {
        delete conn;
    } else {
        conn->player->kicked = true;
    }

    conn->removed = true;
}

void NetworkPlayer::prepareNextMove(Game &game, Snake &snake) {
    int length;
    char* packet = makeMoveRequestPacket(length);

    this->receivedMove = std::nullopt;
    this->connection->sendData(packet, length);

    delete[] packet;
}

std::optional<Move> NetworkPlayer::queryNextMove() {
    return this->receivedMove;
}

template<typename T>
int write(void* buf, T data) {
    memcpy(buf, &data, sizeof(T));
    return sizeof(T);
}

char* makeConnectionEstablishedPacket(int& len) {
    char* packet = new char[4];

    packet[0] = 0; //Length
    packet[1] = 0; //Length

    packet[2] = CONNECTION_ESTABLISHED; //Type
    packet[3] = 0; //Padding

    len = 4;

    return packet;
}

char* makeMoveRequestPacket(int& len) {
    char* packet = new char[4];

    packet[0] = 0; //Length
    packet[1] = 0; //Length

    packet[2] = MOVE_REQUEST; //Type
    packet[3] = 0; //Padding

    len = 4;

    return packet;
}

char* makeGameChangesPacket(int& len, Game& game, Snake& snake, Changes& changes) {
    unsigned short bodyLength = 8 + 4;

    for (auto& change: changes.changes) {
        if (change.second.type == SquareType::SNAKE) {
            bodyLength += 13;
        } else {
            bodyLength += 9;
        }
    }

    char* packet = new char[4 + bodyLength];

    packet[0] = bodyLength & 0xFF; //Length
    packet[1] = bodyLength >> 8; //Length

    packet[2] = GAME_CHANGES; //Type
    packet[3] = 0; //Padding

    char* buf = packet + 4;

    buf += write(buf, snake.getHead().row);
    buf += write(buf, snake.getHead().col);

    buf += write(buf, changes.newTurn);

    for (auto& change : changes.changes) {
        buf += write(buf, change.first.row);
        buf += write(buf, change.first.col);
        buf += write(buf, change.second.type);

        if (change.second.type == SquareType::SNAKE) {
            buf += write(buf, change.second.snakeID);
        }
    }

    len = 4 + bodyLength;

    return packet;
}

char* makeGameStartPacket(int& len, Game& game, Snake& snake) {
    short bodyLength = 8 /*dimensions*/ + 4 /*id*/;

    char* packet = new char[4 + bodyLength];

    packet[0] = bodyLength & 0xFF; //Length
    packet[1] = bodyLength >> 8; //Length

    packet[2] = GAME_START; //Type
    packet[3] = 0; //Padding

    char* buf = packet + 4;

    buf += write(buf, game.getNumRows());
    buf += write(buf, game.getNumCols());
    buf += write(buf, snake.getID());

    len = 4 + bodyLength;

    return packet;
}

char* makeWholeGridPacket(int& len, Game& game) {
    short bodyLength = game.getNumRows() * game.getNumCols() * 5;

    char* packet = new char[4 + bodyLength];

    packet[0] = bodyLength & 0xFF; //Length
    packet[1] = bodyLength >> 8; //Length

    packet[2] = WHOLE_GRID; //Type
    packet[3] = 0; //Padding

    char* buf = packet + 4;

    for (unsigned int i = 0; i < game.getNumRows(); i++) {
        for (unsigned int j = 0; j < game.getNumCols(); j++) {
            buf += write(buf, game.getSquare({i, j}).type);
            buf += write(buf, game.getSquare({i, j}).snakeID);
        }
    }

    len = 4 + bodyLength;

    return packet;
}