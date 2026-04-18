#ifndef __CONNECTIONSESSION_HPP__
#define __CONNECTIONSESSION_HPP__

#include <cstring>
#include <stdexcept>
#include <memory>

#include <sys/socket.h>
#include <netinet/in.h>

#include <bfc/memory_pool.hpp>
#include <singleton.hpp>
#include <bfc/thread_pool.hpp>

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

    std::shared_ptr<Game> attachedGame() const { return mGame; }

    void reset();
    void handleRead();
    void decodeMessage(std::byte* pRaw, size_t pSize);

    const std::string& clientDisplayName() const override;

    void send(TetrisProtocol& pMessage) override;

    void onMsg(ClientChat& pMsg, TetrisProtocol& pParent);

    void onMsg(LeaveIndication&& pMsg, TetrisProtocol& pParent);

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

    void disassociateGame() override;

private:

    void onMsg(CreateGameRequest&& pMsg);
    void onMsg(JoinRequest&& pMsg);

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
    bfc::thread_pool<>& mTp;
    bfc::log2_memory_pool<>& mMp;
};

} // namespace tetris

#endif // __CONNECTIONSESSION_HPP__