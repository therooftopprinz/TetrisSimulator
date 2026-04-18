#ifndef __CONNECTIONSESSION_HPP__
#define __CONNECTIONSESSION_HPP__

#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <sys/socket.h>
#include <netinet/in.h>

#include <interface/protocol_export.hpp>
#include <tetris_log.hpp>

#include <Game.hpp>
#include <ITetrisSimulator.hpp>
#include <IConnectionSession.hpp>

namespace tetris
{

class ConnectionSession : public IConnectionSession, public std::enable_shared_from_this<ConnectionSession>
{
public:
    ConnectionSession(int pFd, ITetrisSimulator& pTetrisSim);

    ~ConnectionSession();

    ConnectionSession(const ConnectionSession&) = delete;

    ConnectionSession& operator=(const ConnectionSession&) = delete;

    std::shared_ptr<Game> attachedGame() const { return mGame; }

    void reset();
    void handleRead();
    void decodeMessage(const std::byte* pRaw, size_t pSize);

    const std::string& clientDisplayName() const override;

    int fd() const noexcept { return mFd; }

    void send(TetrisProtocol& pMessage) override;

    void onMsg(ClientChat& pMsg, TetrisProtocol& pParent);

    void onMsg(LeaveIndication& pMsg, TetrisProtocol& pParent);

    void handleLogin(LoginRequest&& pMsg);

    template <typename T>
    void onMsg(T&& pMsg, TetrisProtocol& pParent)
    {
        // Session-level PDUs must not be forwarded to Game::handle (which does not dispatch them).
        if constexpr (std::is_same_v<std::decay_t<T>, ClientChat>)
        {
            onMsg(pMsg, pParent);
            return;
        }
        if constexpr (std::is_same_v<std::decay_t<T>, LeaveIndication>)
        {
            onMsg(pMsg, pParent);
            return;
        }
        if (mGame)
        {
            mGame->onMsg(std::move(pParent));
        }
        else
        {
            onMsg(std::move(pMsg));
        }
    }

    template <typename T>
    void onMsg(T&& pMsg)
    {
        std::string stred;
        str("msg", pMsg, stred, true);
        logless::log(tetris_logger(), logless::WARNING, logless::LOGALL,
            "ConnectionSession[fd=%d;]: unhandled message (no game): %s;", mFd, stred.c_str());
    }

    void disassociateGame() override;

private:

    void onMsg(CreateGameRequest&& pMsg);
    void onMsg(JoinRequest&& pMsg);

    bool mLoggedIn = false;
    std::string mClientDisplayName;

    std::byte mBuff[512];
    uint16_t mBuffIdx = 0;
    enum ReadState {WAIT_HEADER, WAIT_REMAINING};
    ReadState mReadState = WAIT_HEADER;
    int mExpectedReadSize = 0;
    int mFd;
    enum SessionMode {IDLE, GM, PLAYER};
    SessionMode mSessionMode = IDLE;
    ITetrisSimulator& mTetrisSim;
    std::shared_ptr<Game> mGame;
};

} // namespace tetris

#endif // __CONNECTIONSESSION_HPP__