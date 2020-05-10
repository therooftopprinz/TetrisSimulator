#ifndef __TETRISSIMULATOR_HPP__
#define __TETRISSIMULATOR_HPP__

#include <cstring>
#include <stdexcept>
#include <map>
#include <mutex>
#include <memory>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <bfc/ThreadPool.hpp>
#include <bfc/EpollReactor.hpp>
#include <bfc/Timer.hpp>

#include <logless/Logger.hpp>

#include <common/TetrisBoard.hpp>
#include <ConnectionSession.hpp>
#include <Game.hpp>
#include <ITetrisSimulator.hpp>

namespace tetris
{

struct TetrisSimulatorConfig
{
    uint16_t port;
};

class TetrisSimulator : public ITetrisSimulator
{
public:
    TetrisSimulator(const TetrisSimulatorConfig& pCfg)
    {
        mServerFd = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == mServerFd)
        {
            throw std::runtime_error(strerror(errno));
        }

        const int one = 1;
        int res = setsockopt(mServerFd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        if (res)
        {
            throw std::runtime_error(strerror(errno));
        }

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = 0;
        addr.sin_port = ntohs(pCfg.port);

        char loc[24];
        inet_ntop(AF_INET, &addr.sin_addr.s_addr, loc, sizeof(loc));
        Logless("TetisSimulator: binding _:_", loc, pCfg.port);

        res = bind(mServerFd, (sockaddr*)&addr, sizeof(addr));

        if  (-1 == res)
        {
            throw std::runtime_error(strerror(errno));
        }

        res = listen(mServerFd, 100);

        if  (-1 == res)
        {
            throw std::runtime_error(strerror(errno));
        }

        if (!mReactor.addHandler(mServerFd, [this](){
                handleServerRead();
            }))
        {
            throw std::runtime_error("TetisSimulator: Failed to register server to EpollReactor.");
        }
    }

    ~TetrisSimulator()
    {
        close(mServerFd);
    }

    void handleServerRead()
    {
        sockaddr_in addr;
        socklen_t len = sizeof(addr);

        auto res = accept(mServerFd, (sockaddr*)&addr, &len);

        if  (-1 == res)
        {
            throw std::runtime_error(strerror(errno));
        }

        char loc[24];
        inet_ntop(AF_INET, &addr.sin_addr.s_addr, loc, sizeof(loc));
        Logless("TetisSimulator: connected client fd=_ address=_:_", res, loc, ntohs(addr.sin_port));

        std::unique_lock<std::mutex> lg(mConnectionsMutex);
        auto empRes = mConnections.emplace(std::piecewise_construct, std::forward_as_tuple(res), std::forward_as_tuple(res, *this));
        lg.unlock();

        auto& connection = empRes.first->second;

        if (!mReactor.addHandler(res, [&connection](){
                connection.handleRead();
            }))
        {
            Logless("TetisSimulator: Failed to register connection fd=_ to EpollReactor, errno=\"_\"", res, strerror(errno));
            onDisconnect(res);
        }
    }

    void onDisconnect(int pFd)
    {
        Logless("TetisSimulator: Disconnected client fd=_", pFd);

        std::unique_lock<std::mutex> lg(mConnectionsMutex);
        if (!mReactor.removeHandler(pFd))
        {
            Logless("TetisSimulator: Failed to delete client fd=_ from EpollReactor, errno=\"_\"", pFd, strerror(errno));
        }
        mConnections.erase(pFd);
    }

    void run()
    {
        mReactor.run();
    }

    std::shared_ptr<Game> getGame(uint32_t pGameId)
    {
        std::unique_lock<std::mutex> lg(mGamesMutex);
        if (!mGames.count(pGameId))
        {
            return nullptr;
        }

        return mGames.at(pGameId);
    }

    int createGame(std::shared_ptr<Game> pGame)
    {
        std::unique_lock<std::mutex> lg(mGamesMutex);
        auto id = mGameIdCtr++;
        mGames.emplace(id, std::move(pGame));
        Logless("TetrisSimulator : Allocating Game id=_", id);
        return id;
    }

private:
    std::map<int, ConnectionSession> mConnections;
    std::mutex mConnectionsMutex;

    std::map<uint32_t, std::shared_ptr<Game>> mGames;
    int mGameIdCtr = 0;
    std::mutex mGamesMutex;

    bfc::EpollReactor mReactor;

    int mServerFd;
};

} // namespace tetris

#endif // __TETRISSIMULATOR_HPP__