#include <ConnectionSession.hpp>

namespace tetris
{

ConnectionSession::ConnectionSession(int pFd, ITetrisSimulator& pTetrisSim)
    : mFd(pFd)
    , mTetrisSim(pTetrisSim)
    , mTp(bfc::Singleton<bfc::ThreadPool<>>::get())
    , mMp(bfc::Singleton<bfc::Log2MemoryPool<>>::get())
    , mTimer(bfc::Singleton<bfc::Timer<>>::get())
{
}

ConnectionSession::~ConnectionSession()
{
    reset();
}

void ConnectionSession::reset()
{
    if (-1 == mFd)
    {
        return;
    }

    Logless("ConnectionSession[fd=_]: reset: closing socket...", mFd);

    close(mFd);
    mFd = -1;
}

void ConnectionSession::handleRead()
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
        // auto raw = mMp.allocate0(mBuffIdx);
        auto raw = new std::byte[mBuffIdx];
        std::memcpy(raw, mBuff, mBuffIdx);
        mTp.execute([this, raw, size = mBuffIdx]() mutable {
                decodeMessage(raw, size);
            });
        mReadState = WAIT_HEADER;
        mBuffIdx = 0;
    }
}

void ConnectionSession::decodeMessage(std::byte* pRaw, size_t pSize)
{
    TetrisProtocol message;
    cum::per_codec_ctx context(pRaw, pSize);
    decode_per(message, context);

    std::string stred;
    str("root", message, stred, true);

    Logless("ConnectionSession[fd=_]: receive: raw=_ decoded=_", mFd, BufferLog(pSize, pRaw), stred.c_str());
    // mMp.free(pRaw, pSize);
    delete[] pRaw;


    std::visit([this, &message](auto& pMessage){
            onMsg(pMessage, message);
        }, message);
}

void ConnectionSession::onMsg(CreateGameRequest& pMsg)
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
        mGame = std::make_shared<Game>(config, shared_from_this(), mTp, mTimer);
        message = CreateGameAccept{};
        auto& createGameAccept = std::get<CreateGameAccept>(message);
        createGameAccept.gameId = mTetrisSim.createGame(mGame);
    }

    send(message);
}

void ConnectionSession::onMsg(JoinRequest& pMsg)
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

    joinAccept.playerId = mGame->join(shared_from_this());
    joinAccept.boardHeight = mGame->getBoardConfig().height;
    joinAccept.boardWidth = mGame->getBoardConfig().width;
    send(message);
}

void ConnectionSession::send(TetrisProtocol& pMessage)
{
    std::byte buffer[512];
    auto& msgSize = *(new (buffer) uint16_t(0));
    cum::per_codec_ctx context(buffer+sizeof(msgSize), sizeof(buffer)-sizeof(msgSize));
    encode_per(pMessage, context);

    msgSize = sizeof(buffer)-context.size()-2;

    std::string stred;
    str("root", pMessage, stred, true);
    Logless("ConnectionSession[fd=_]: send: raw=_ encoded=_", mFd, BufferLog(msgSize, buffer+2), stred.c_str());
    Logger::getInstance().flush();

    auto res = ::send(mFd, buffer, msgSize+2, 0);
    if (-1 == res)
    {
        throw std::runtime_error(strerror(errno));
    }
}


} // namespace tetris
