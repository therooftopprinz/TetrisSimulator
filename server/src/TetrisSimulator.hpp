#ifndef __TETRISSIMULATOR_HPP__
#define __TETRISSIMULATOR_HPP__

#include <chrono>
#include <cstring>
#include <stdexcept>
#include <map>
#include <memory>
#include <unordered_map>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
    TetrisSimulator(bfc::epoll_reactor<>& pReactor, const TetrisSimulatorConfig& pCfg)
        : mReactor(pReactor)
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

        auto empRes = mConnections.emplace(res, std::make_shared<ConnectionSession>(res, *this));
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
        ClientChat out = pMsg;
        const auto it = mFdToConnection.find(senderFd);
        if (it != mFdToConnection.end())
        {
            out.username = it->second->clientDisplayName();
        }
        message = out;
        for (auto& c : mConnections)
        {
            if (c.first != senderFd)
            {
                c.second->send(message);
            }
        }
    }

    bool claimUsername(std::shared_ptr<ConnectionSession> session, const std::string& username) override
    {
        if (username.empty())
        {
            return false;
        }
        if (mUsernameToConnection.count(username))
        {
            return false;
        }
        mUsernameToConnection[username] = session;
        mFdToConnection[session->fd()] = session;
        return true;
    }

    void onDisconnect(int pFd) override
    {
        logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
            "TetrisSimulator: Disconnected client fd=%d;", pFd);

        {
            const auto itc = mFdToConnection.find(pFd);
            if (itc != mFdToConnection.end())
            {
                mUsernameToConnection.erase(itc->second->clientDisplayName());
                mFdToConnection.erase(itc);
            }
        }

        std::shared_ptr<ConnectionSession> session;
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

        if (auto game = session->attachedGame())
        {
            game->onConnectionLost(std::static_pointer_cast<IConnectionSession>(session));
        }
    }

    std::shared_ptr<Game> getGame(uint32_t pGameId) override
    {
        if (!mGames.count(pGameId))
        {
            return nullptr;
        }

        return mGames.at(pGameId);
    }

    int createGame(std::shared_ptr<Game> pGame) override
    {
        auto id = static_cast<uint32_t>(mGameIdCtr++);
        pGame->setGameId(id);
        mGames.emplace(id, std::move(pGame));
        logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
            "TetrisSimulator: Allocating Game id=%d;", id);
        return static_cast<int>(id);
    }

    void destroyGame(uint32_t pGameId) override
    {
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
    bfc::epoll_reactor<>& mReactor;

    std::unordered_map<std::string, std::shared_ptr<ConnectionSession>> mUsernameToConnection;
    std::unordered_map<int, std::shared_ptr<ConnectionSession>> mFdToConnection;

    std::map<int, std::shared_ptr<ConnectionSession>> mConnections;

    std::map<uint32_t, std::shared_ptr<Game>> mGames;
    int mGameIdCtr = 0;

    int mServerFd;
};

} // namespace tetris

#endif // __TETRISSIMULATOR_HPP__