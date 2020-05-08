#ifndef __ITETRISSIMULATOR_HPP__
#define __ITETRISSIMULATOR_HPP__

#include <memory>

class Game;

namespace tetris
{

struct ITetrisSimulator
{
    virtual void onDisconnect(int pFd) = 0;
    virtual std::shared_ptr<Game> getGame(uint32_t pGameId) = 0;
    virtual int createGame(std::shared_ptr<Game> pGame) = 0;
};

} // namespace tetris

#endif // __ITETRISSIMULATOR_HPP__