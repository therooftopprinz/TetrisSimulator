#ifndef __ITETRISSIMULATOR_HPP__
#define __ITETRISSIMULATOR_HPP__

#include <chrono>
#include <memory>
#include <optional>

#include <bfc/function.hpp>

#include <GameTimerId.hpp>

struct ClientChat;

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
    virtual void destroyGame(uint32_t pGameId) = 0;
};

struct ITetrisSimulator : IGameManager, IConnectionCallback
{
    virtual GameTimerId scheduleGameTimer(std::chrono::nanoseconds delay, bfc::light_function<void()> cb) = 0;
    virtual bool cancelGameTimer(GameTimerId id) = 0;
    virtual void cancelGameTimer(std::optional<GameTimerId>& slot) = 0;
    virtual void broadcastClientChat(const ClientChat& pMsg, int senderFd) = 0;
};

} // namespace tetris

#endif // __ITETRISSIMULATOR_HPP__