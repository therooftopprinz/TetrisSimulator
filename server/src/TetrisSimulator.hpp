#ifndef __TETRISSIMULATOR_HPP__
#define __TETRISSIMULATOR_HPP__

#include <chrono>
#include <cstring>
#include <stdexcept>
#include <map>
#include <mutex>
#include <memory>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <bfc/thread_pool.hpp>
#include <bfc/epoll_reactor.hpp>

#include <tetris_log.hpp>

#include <common/StandardTetrisBoard.hpp>
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
        logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
            "TetrisSimulator: binding %s; port=%hu;", loc, pCfg.port);

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

        if (!mReactor.add_read_rdy(mServerFd, [this](){
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
        logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
            "TetrisSimulator: connected client fd=%d; address=%s; port=%hu;", res, loc, ntohs(addr.sin_port));

        std::unique_lock<std::mutex> lg(mConnectionsMutex);
        auto empRes = mConnections.emplace(res, std::make_shared<ConnectionSession>(res, *this));
        lg.unlock();

        auto connection = empRes.first->second;

        if (!mReactor.add_read_rdy(res, [connection](){
                connection->handleRead();
            }))
        {
            logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
                "TetrisSimulator: Failed to register connection fd=%d; to EpollReactor, errno=%s;", res, strerror(errno));
            onDisconnect(res);
        }
    }

    void broadcastClientChat(const ClientChat& pMsg, int senderFd) override
    {
        TetrisProtocol message;
        message = pMsg;
        std::unique_lock<std::mutex> lg(mConnectionsMutex);
        for (auto& c : mConnections)
        {
            if (c.first != senderFd)
            {
                c.second->send(message);
            }
        }
    }

    void onDisconnect(int pFd) override
    {
        logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
            "TetrisSimulator: Disconnected client fd=%d;", pFd);

        std::shared_ptr<ConnectionSession> session;
        {
            std::unique_lock<std::mutex> lg(mConnectionsMutex);
            auto it = mConnections.find(pFd);
            if (it == mConnections.end())
            {
                return;
            }
            session = it->second;
            if (!mReactor.rem_read_rdy(pFd))
            {
                logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
                    "TetrisSimulator: Failed to delete client fd=%d; from EpollReactor, errno=%s;", pFd, strerror(errno));
            }
            mConnections.erase(it);
        }

        if (auto game = session->attachedGame())
        {
            game->onConnectionLost(std::static_pointer_cast<IConnectionSession>(session));
        }
    }

    void run()
    {
        mReactor.run();
    }

    std::shared_ptr<Game> getGame(uint32_t pGameId) override
    {
        std::unique_lock<std::mutex> lg(mGamesMutex);
        if (!mGames.count(pGameId))
        {
            return nullptr;
        }

        return mGames.at(pGameId);
    }

    int createGame(std::shared_ptr<Game> pGame) override
    {
        std::unique_lock<std::mutex> lg(mGamesMutex);
        auto id = static_cast<uint32_t>(mGameIdCtr++);
        pGame->setGameId(id);
        mGames.emplace(id, std::move(pGame));
        logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
            "TetrisSimulator: Allocating Game id=%d;", id);
        return static_cast<int>(id);
    }

    void destroyGame(uint32_t pGameId) override
    {
        std::unique_lock<std::mutex> lg(mGamesMutex);
        mGames.erase(pGameId);
    }

    GameTimerId scheduleGameTimer(std::chrono::nanoseconds delay, bfc::light_function<void()> cb) override
    {
        const int64_t us = std::chrono::duration_cast<std::chrono::microseconds>(delay).count();
        auto id = mReactor.get_timer().wait_us(us, std::move(cb));
        mReactor.wake_up(nullptr);
        return id;
    }

    bool cancelGameTimer(GameTimerId id) override
    {
        const bool ok = mReactor.get_timer().cancel(id);
        mReactor.wake_up(nullptr);
        return ok;
    }

    void cancelGameTimer(std::optional<GameTimerId>& slot) override
    {
        if (!slot)
        {
            return;
        }
        mReactor.get_timer().cancel(*slot);
        slot.reset();
        mReactor.wake_up(nullptr);
    }

private:
    std::map<int, std::shared_ptr<ConnectionSession>> mConnections;
    std::mutex mConnectionsMutex;

    std::map<uint32_t, std::shared_ptr<Game>> mGames;
    int mGameIdCtr = 0;
    std::mutex mGamesMutex;

    bfc::epoll_reactor<> mReactor;

    int mServerFd;
};

} // namespace tetris

#endif // __TETRISSIMULATOR_HPP__