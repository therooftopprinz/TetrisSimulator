#ifndef __GAME_HPP__
#define __GAME_HPP__

#include <cstring>
#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h> 

#include <interface/protocol.hpp>

#include <common/TetrisBoard.hpp>
#include <ITetrisSimulator.hpp>

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
    Game(const GameConfig& pConfig, IExecutor& pExecutor)
        : mConfig(pConfig)
        , mExecutor(pExecutor)
    {}

private:
    GameConfig mConfig;
    std::map<uint32_t, Player> mPlayers;
    IExecutor& mExecutor;
};

} // namespace tetris

#endif // __GAME_HPP__