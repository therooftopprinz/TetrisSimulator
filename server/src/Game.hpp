#ifndef __GAME_HPP__
#define __GAME_HPP__

#include <cstring>
#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h> 

#include <interface/protocol.hpp>

#include <common/TetrisBoard.hpp>

namespace tetris
{

struct GameConfig
{
    uint8_t width;
    uint8_t height;
    uint16_t lockingTimeout;
};

class Player
{
public:
private:
    TetrisBoard mBoard;
};

class Game
{
public:
    Game(const GameConfig& pConfig)
        : mConfig(pConfig)
    {}

private:
    GameConfig mConfig;
    std::map<uint32_t, Player> mPlayers;
};

} // namespace tetris

#endif // __GAME_HPP__