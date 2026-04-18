#include <ConnectionSession.hpp>
#include <tetris_log.hpp>

#include <cerrno>

namespace tetris
{

ConnectionSession::ConnectionSession(int pFd, ITetrisSimulator& pTetrisSim)
    : mFd(pFd)
    , mTetrisSim(pTetrisSim)
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

    logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
        "ConnectionSession[fd=%d;]: reset: closing socket...", mFd);

    close(mFd);
    mFd = -1;
}

void ConnectionSession::disassociateGame()
{
    mGame.reset();
    mSessionMode = IDLE;
}

const std::string& ConnectionSession::clientDisplayName() const
{
    return mClientDisplayName;
}

void ConnectionSession::onMsg(ClientChat& pMsg, TetrisProtocol&)
{
    if (!pMsg.message.empty())
    {
        mTetrisSim.broadcastClientChat(pMsg, mFd);
    }
}

void ConnectionSession::onMsg(LeaveIndication& pMsg, TetrisProtocol&)
{
    if (!mGame)
    {
        return;
    }
    if (static_cast<uint32_t>(pMsg.gameId) != mGame->gameId())
    {
        return;
    }
    mGame->onVoluntaryLeave(std::static_pointer_cast<IConnectionSession>(shared_from_this()));
}

void ConnectionSession::handleRead()
{
    int readSize = 0;
    if (WAIT_HEADER == mReadState)
    {
        readSize = 2 - static_cast<int>(mBuffIdx);
    }
    else
    {
        readSize = mExpectedReadSize - static_cast<int>(mBuffIdx);
    }

    auto res = read(mFd, mBuff+mBuffIdx, readSize);

    if (-1 == res)
    {
        const int err = errno;
        if (err == EINTR)
        {
            return;
        }
        logless::log(tetris_logger(), logless::WARNING, logless::LOGALL,
            "ConnectionSession[fd=%d;]: read failed: errno=%d; %s;", mFd, err, strerror(err));
        mTetrisSim.onDisconnect(mFd);
        return;
    }
    if (0 == res)
    {
        mTetrisSim.onDisconnect(mFd);
        return;
    }

    mBuffIdx += static_cast<uint16_t>(res);

    if (WAIT_HEADER == mReadState)
    {
        if (mBuffIdx < 2)
        {
            return;
        }
        std::memcpy(&mExpectedReadSize, mBuff, 2);
        mBuffIdx = 0;
        if (mExpectedReadSize <= 0 || mExpectedReadSize > static_cast<int>(sizeof(mBuff)))
        {
            mTetrisSim.onDisconnect(mFd);
            return;
        }
        mReadState = WAIT_REMAINING;
        return;
    }

    if (mExpectedReadSize == static_cast<int>(mBuffIdx))
    {
        decodeMessage(mBuff, mBuffIdx);
        mReadState = WAIT_HEADER;
        mBuffIdx = 0;
    }
}

void ConnectionSession::handleLogin(LoginRequest&& pMsg)
{
    if (pMsg.username.empty())
    {
        mTetrisSim.onDisconnect(mFd);
        return;
    }
    mClientDisplayName = pMsg.username;
    if (!mTetrisSim.claimUsername(shared_from_this(), pMsg.username))
    {
        mClientDisplayName.clear();
        TetrisProtocol response = LoginResponse{};
        std::get<LoginResponse>(response).result = LoginResult::ALREADY_EXIST;
        send(response);
        mTetrisSim.onDisconnect(mFd);
        return;
    }
    mLoggedIn = true;
    TetrisProtocol response = LoginResponse{};
    std::get<LoginResponse>(response).result = LoginResult::OK;
    send(response);
}

void ConnectionSession::decodeMessage(const std::byte* pRaw, size_t pSize)
{
    TetrisProtocol message;
    cum::per_codec_ctx context(const_cast<std::byte*>(pRaw), pSize);
    decode_per(message, context);

    std::string stred;
    str("root", message, stred, true);

    logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
        "ConnectionSession[fd=%d;]: receive: raw=%%; decoded=%s;", mFd, BufferLog(pSize, pRaw), stred.c_str());

    if (!mLoggedIn)
    {
        if (!std::holds_alternative<LoginRequest>(message))
        {
            mTetrisSim.onDisconnect(mFd);
            return;
        }
        handleLogin(std::get<LoginRequest>(std::move(message)));
        return;
    }

    if (std::holds_alternative<LoginRequest>(message))
    {
        mTetrisSim.onDisconnect(mFd);
        return;
    }

    std::visit([this, &message](auto& pMessage){
            onMsg(pMessage, message);
        }, message);
}

void ConnectionSession::onMsg(CreateGameRequest&& pMsg)
{
    TetrisProtocol message;
    if (IDLE != mSessionMode)
    {
        message = CreateGameReject{};
    }
    else
    {
        mSessionMode = GM;
        mGame = std::make_shared<Game>(std::move(pMsg), shared_from_this(), mTetrisSim);
        message = CreateGameAccept{};
        auto& createGameAccept = std::get<CreateGameAccept>(message);
        createGameAccept.gameId = mTetrisSim.createGame(mGame);
    }

    send(message);
}

void ConnectionSession::onMsg(JoinRequest&& pMsg)
{
    TetrisProtocol message;

    auto game = mTetrisSim.getGame(pMsg.gameId);

    if (IDLE != mSessionMode || !game)
    {
        message = JoinReject{};
        send(message);
        return;
    }

    game->join(pMsg, shared_from_this());
    mGame = game;
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
    logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
        "ConnectionSession[fd=%d;]: send: raw=%%; encoded=%s;", mFd, BufferLog(msgSize, buffer+2), stred.c_str());
    tetris_logger().flush();

    auto res = ::send(mFd, buffer, msgSize+2, MSG_NOSIGNAL);
    if (-1 == res)
    {
        if (errno == EPIPE || errno == ECONNRESET || errno == ENOTCONN)
        {
            mTetrisSim.onDisconnect(mFd);
            return;
        }
        throw std::runtime_error(strerror(errno));
    }
}


} // namespace tetris
