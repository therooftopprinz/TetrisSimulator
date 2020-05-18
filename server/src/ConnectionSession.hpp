#ifndef __CONNECTIONSESSION_HPP__
#define __CONNECTIONSESSION_HPP__

#include <cstring>
#include <stdexcept>
#include <memory>

#include <sys/socket.h>
#include <netinet/in.h>

#include <logless/Logger.hpp>

#include <bfc/MemoryPool.hpp>
#include <bfc/Singleton.hpp>
#include <bfc/Timer.hpp>
#include <bfc/ThreadPool.hpp>

#include <interface/protocol.hpp>

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

    void reset();
    void handleRead();
    void decodeMessage(std::byte* pRaw, size_t pSize);

    template <typename T>
    void onMsg(T&& pMsg, TetrisProtocol& pParent)
    {
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
    void onMsg(T&&)
    {
    }

private:

    void disassociateGame();

    void onMsg(CreateGameRequest&& pMsg);
    void onMsg(JoinRequest&& pMsg);

    void send(TetrisProtocol& pMessage);

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
    bfc::ThreadPool<>& mTp;
    bfc::Log2MemoryPool<>& mMp;
    bfc::Timer<>& mTimer;
};

} // namespace tetris

#endif // __CONNECTIONSESSION_HPP__