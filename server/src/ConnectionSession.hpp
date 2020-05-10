#ifndef __CONNECTIONSESSION_HPP__
#define __CONNECTIONSESSION_HPP__

#include <cstring>
#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h>

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

class ConnectionSession : public IConnectionSession
{
public:
    ConnectionSession(int pFd, ITetrisSimulator& pTetrisSim)
        : mFd(pFd)
        , mTetrisSim(pTetrisSim)
        , mTp(bfc::Singleton<bfc::ThreadPool<>>::get())
        , mMp(bfc::Singleton<bfc::Log2MemoryPool<>>::get())
        , mTimer(bfc::Singleton<bfc::Timer<>>::get())
    {
    }

    ~ConnectionSession()
    {
        reset();
    }

    ConnectionSession(const ConnectionSession&) = delete;

    ConnectionSession& operator=(const ConnectionSession&) = delete;

    void reset()
    {
        if (-1 == mFd)
        {
            return;
        }

        Logless("ConnectionSession[fd=_]: reset: closing socket...", mFd);

        close(mFd);
        mFd = -1;
    }

    void handleRead()
    {
        int readSize = 0;
        if (WAIT_HEADER == mReadState)
        {
            readSize = 2;
        }
        else
        {
            readSize = mExpectedReadSize - mBuffIdx;
        }

        auto res = read(mFd, mBuff+mBuffIdx, readSize);

        if (-1 == res)
        {
            throw std::runtime_error(strerror(errno));
        }
        if (0 == res)
        {
            mTetrisSim.onDisconnect(mFd);
            return;
        }

        mBuffIdx += res;

        if (WAIT_HEADER == mReadState)
        {
            std::memcpy(&mExpectedReadSize, mBuff, 2);
            mBuffIdx = 0;
            mReadState = WAIT_REMAINING;
            return;
        }

        if (mExpectedReadSize == mBuffIdx)
        {
            auto raw = mMp.allocate0(mBuffIdx);
            std::memcpy(raw, mBuff, mBuffIdx);
            mTp.execute([this, raw, size = mBuffIdx]() mutable {
                    decodeMessage(raw, size);
                });
            mReadState = WAIT_HEADER;
            mBuffIdx = 0;
        }
    }

    void decodeMessage(std::byte* pRaw, size_t pSize)
    {
        TetrisProtocol message;
        cum::per_codec_ctx context(pRaw, pSize);
        decode_per(message, context);

        mMp.free(pRaw, pSize);

        std::string stred;
        str("root", message, stred, true);

        Logless("ConnectionSession[fd=_]: Receive raw=_ decoded=_", mFd, BufferLog(mBuffIdx, mBuff), stred.c_str());

        std::visit([this](auto& message){
                onMsg(message);
            }, message);
    }

    template <typename T>
    void onMsg(T& pMsg)
    {
        if (mGame)
        {
            mGame->onMsg(pMsg);
        }
    }

    void onMsg(CreateGameRequest& pMsg)
    {
        TetrisProtocol message;
        if (IDLE != mSessionMode)
        {
            message = CreateGameReject{};
        }
        else
        {
            mSessionMode = GM;
            GameConfig config = {};
            config.height = pMsg.boardHeight;
            config.width = pMsg.boardWidth;
            config.lockingTimeout = pMsg.lockingTimeoutMs;
            mGame = std::make_shared<Game>(config, *this, mTp, mTimer);
            message = CreateGameAccept{};
            auto& createGameAccept = std::get<CreateGameAccept>(message);
            createGameAccept.gameId = mTetrisSim.createGame(mGame);
        }

        send(message);
    }

    void onMsg(JoinRequest& pMsg)
    {
        TetrisProtocol message;

        auto game = mTetrisSim.getGame(pMsg.gameId);

        if (IDLE != mSessionMode || !game)
        {
            message = JoinReject{};
            send(message);
            return;
        }

        mGame = game;

        message = JoinAccept{};
        auto& joinAccept = std::get<JoinAccept>(message);

        joinAccept.playerId = mGame->join(*this);

        send(message);
    }

private:

    void send(TetrisProtocol& pMessage)
    {
        std::byte buffer[512];
        auto& msgSize = *(new (buffer) uint16_t(0));
        cum::per_codec_ctx context(buffer+2, sizeof(buffer)-2);
        encode_per(pMessage, context);

        msgSize = sizeof(buffer) - context.size();

        std::string stred;
        str("root", pMessage, stred, true);
        Logless("ConnectionSession[fd=_]: send: raw=_ encoded=_", mFd, BufferLog(msgSize, buffer), stred.c_str());

        auto res = ::send(mFd, buffer, msgSize+2, 0);
        if (-1 == res)
        {
            throw std::runtime_error(strerror(errno));
        }
    }

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