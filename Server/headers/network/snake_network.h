//
// Created by Anatol on 25/06/2022.
//

#ifndef SNAKE_SNAKE_NETWORK_H
#define SNAKE_SNAKE_NETWORK_H

#include <WS2tcpip.h>
#include <vector>
#include <memory>
#include <map>
#include "Player.h"

#define PORT 42069

class GameCreator;

/*
 * All packet headers are 4 bytes long.
 * The first two bytes are the packet length
 * The third byte is the packet type.
 * The fourth byte is padding
 */

enum OutwardBoundPacketType: uint8_t {
    CONNECTION_ESTABLISHED = 0,
    MOVE_REQUEST = 1,
    GAME_CHANGES = 2,
    GAME_START = 3,
#ifdef _DEBUG
    WHOLE_GRID = 4,
#endif
    SNAKE_DEAD = 5,
    GAME_RESULTS = 6,
};

enum InwardBoundPacketType: uint8_t {
    NAME_AND_COLOR,
    MOVE_RESPONSE
};

class NetworkPlayer;
class ConnectionManager;

class Connection {
public:
    Connection(SOCKET i, time_t i1, ConnectionManager* manager);

    SOCKET socket;
    long long createdAt;

    NetworkPlayer* player = 0;
    ConnectionManager* manager;

    bool removed = false;

    //Allow for receiving partial packets
    enum CurrState {
        AWAITING_HEADER, AWAITING_BODY, COMPLETE
    };

    CurrState currState = AWAITING_HEADER;
    char header[4];
    int headerReceived = 0;
    char body[1024];
    int bodyLength = 0;
    int bodyReceived = 0;

    //Returns false if the connection should be destroyed
    bool tick();

    void sendData(const char* data, int len);
    bool handle(char packetType, const char* data, int len);
};

class NetworkPlayer: public Player {
public:
    Connection* connection;

    NetworkPlayer(std::string name, Color color, Connection* c);

    void receiveMove(Move move);

private:
    std::optional<Move> receivedMove;
public:
protected:
    void onDeath(Game &game, Snake &snake, std::string reason, bool timeout) override;

public:
    void
    endGame(Game &game, Snake &snake, bool died, unsigned int length, int score, unsigned int diedOn, unsigned int rank,
            unsigned int numTies, int newElo) override;

public:
    void beginGame(Game &game, Snake &snake) override;

    void receiveChanges(Game &game, Snake &snake, Changes &changes) override;

    void onRemoved() override;

protected:
    void prepareNextMove(Game &game, Snake &snake) override;

    std::optional<Move> queryNextMove() override;
};

class ConnectionManager {
public:
    ConnectionManager(const char* ip, GameCreator* creator);
    ~ConnectionManager();

    void tick();

    std::vector<Connection*> connections;
    fd_set allConnections;
    std::map<SOCKET, Connection*> connectionMap;
    GameCreator* creator;
private:
    SOCKET serverSocket;
    fd_set listener;

    void handleDeadConnection(Connection *conn);
};

char* makeConnectionEstablishedPacket(int& len);
char* makeMoveRequestPacket(int& len);
char* makeGameChangesPacket(int& len, Game& game, Snake& snake, Changes& changes);
char* makeGameStartPacket(int& len, Game& game, Snake& snake);
char* makeWholeGridPacket(int& len, Game& game);
char* makeSnakeDeadPacket(int& len, std::string& reason);
char* makeGameResultsPacket(int& len, bool died, unsigned int length, int score, unsigned int diedOn, unsigned int rank, unsigned int numTies, int newElo);

#endif //SNAKE_SNAKE_NETWORK_H
