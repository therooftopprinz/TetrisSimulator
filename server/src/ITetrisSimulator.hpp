#ifndef __ITETRISSIMULATOR_HPP__
#define __ITETRISSIMULATOR_HPP__

#include <memory>

#include <bfc/FixedFunctionObject.hpp>

namespace tetris
{

class Game;

struct IConnectionCallback
{
    virtual void onDisconnect(int pFd) = 0;
};

struct IGameManager
{
    virtual std::shared_ptr<Game> getGame(uint32_t pGameId) = 0;
    virtual int createGame(std::shared_ptr<Game> pGame) = 0;
};

struct ITetrisSimulator : IGameManager, IConnectionCallback
{};

} // namespace tetris

#endif // __ITETRISSIMULATOR_HPP__