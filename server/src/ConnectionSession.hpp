#ifndef __CONNECTIONSESSION_HPP__
#define __CONNECTIONSESSION_HPP__

#include <cstring>
#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h> 

#include <interface/protocol.hpp>

#include <Game.hpp>
#include <ITetrisSimulator.hpp>

namespace tetris
{

class ConnectionSession
{
public:
    ConnectionSession(int pFd, ITetrisSimulator& pTetrisSim)
        : mFd(pFd)
        , mTetrisSim(pTetrisSim)
    {
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
            decodeMessage();
            mReadState = WAIT_HEADER;
            mBuffIdx = 0;
        }
    }

    void decodeMessage()
    {
        

        TetrisProtocol message;
        cum::per_codec_ctx context(mBuff, mBuffIdx);
        decode_per(message, context);

        std::string stred;
        str("root", message, stred, true);

        Logless("ConnectionSession[fd=_] : Receive raw=_ decoded=_", mFd, BufferLog(mBuffIdx, mBuff), stred.c_str());

        std::visit([this](auto& message)
            {
                handleMessage(message);
            }, message);
    }

    template <typename T>
    void handleMessage(T& pMsg)
    {
    }

    void handleMessage(CreateGameRequest& pMsg)
    {
        TetrisProtocol message;
        if (IDLE != mSessionMode)
        {
            message = CreateGameRequestReject{};
        }
        else
        {
            mSessionMode = GM;
            GameConfig config = {};
            config.height = pMsg.boardHeight;
            config.width = pMsg.boardWidth;
            config.lockingTimeout = pMsg.lockingTimeoutMs;
            auto mGame = std::make_shared<Game>(config);
            message = CreateGameRequestAccept{};
            auto& createGameRequestAccept = std::get<CreateGameRequestAccept>(message);
            createGameRequestAccept.gameId = mTetrisSim.createGame(mGame);
        }

        send(message);
    }

    void handleMessage(JoinRequest& pMsg)
    {
        auto game = mTetrisSim.getGame(pMsg.gameId);
    }

    void send(TetrisProtocol& pMessage)
    {
        std::byte buffer[512];
        auto& msgSize = *(new (buffer) uint16_t(0)); 
        cum::per_codec_ctx context(buffer + 2, sizeof(buffer) -2);
        encode_per(pMessage, context);

        msgSize = sizeof(buffer) - context.size();

        std::string stred;
        str("root", pMessage, stred, true);
        Logless("ConnectionSession[fd=_] : send : raw=_ encoded=_", mFd, BufferLog(msgSize, buffer), stred.c_str());

        auto res = ::send(mFd, buffer, msgSize+2, 0);
        if (-1 == res)
        {
            throw std::runtime_error(strerror(errno));
        }
    }

private:
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