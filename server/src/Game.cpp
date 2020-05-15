#include <Game.hpp>

namespace tetris
{

Game::Game(const GameConfig& pConfig, std::weak_ptr<IConnectionSession> pGmSession, bfc::ThreadPool<>& pTp, bfc::Timer<>& pTimer)
    : mBoardConfig{pConfig.width, pConfig.height}
    , mConfig(pConfig)
    , mGmSession(pGmSession)
    , mTp(pTp)
    , mTimer(pTimer)
{
    mRunningWokerLock.unlock();
}

uint8_t Game::join(std::weak_ptr<IConnectionSession> pPlayerSession)
{
    std::unique_lock<std::mutex> lg(mPlayersMutex);

    uint8_t id = mPlayersIdCtr++;

    auto res = mPlayers.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(id, mBoardConfig, pPlayerSession));
    auto& player = res.first->second;
    auto& playerCallbacks = player.getCallbacks();
    playerCallbacks.generate = [&player, this]() -> Termino {return onBcbGenerate(player);};
    playerCallbacks.replace = [&player, this](std::vector<Line> pLines) {return onBcbReplace(player, std::move(pLines));};
    playerCallbacks.clear = [&player, this](std::vector<uint8_t> pLines) {return onBcbClear(player, std::move(pLines));};
    playerCallbacks.piecePosition = [&player, this](CellCoord pCoord) {return onBcbPiecePosition(player, pCoord);};
    playerCallbacks.placePiece = [&player, this](Termino PTermino) {return onBcbPlacePiece(player, PTermino);};
    playerCallbacks.rotate = [&player, this](uint8_t pRot) {return onBcbRotate(player, pRot);};
    playerCallbacks.piecesAdded = [&player, this](std::vector<Termino> pTerminos) {return onBcbPiecesAdded(player, std::move(pTerminos));};
    playerCallbacks.hold = [&player, this]() {return onBcbHold(player);};
    playerCallbacks.commit = [&player, this]() {return onBcbCommit(player);};

    return id;
}

const TetrisBoardConfig& Game::getBoardConfig() const
{
    return mBoardConfig;
}

void Game::trigger(bfc::LightFn<void()> pFn)
{
    onMsg(std::move(pFn));
}

void Game::runEvent(GameEvent& pEvent)
{
    std::unique_lock<std::mutex> lg(mPlayersMutex);
    std::visit([this](auto& pEvent){handle(pEvent);}, pEvent);
}

void Game::handle(bfc::LightFn<void()>& pFn)
{
    pFn();
}

void Game::handle(TetrisProtocol& pMsg)
{
    std::visit([this](auto& pMsg){handle(pMsg);}, pMsg);
}

void Game::handle(GameStartIndication& pMsg)
{
    if (mGameStarted)
    {
        return;
    }

    mGameStarted = true;

    mTerminoCache.clear();
    TetrisProtocol message = GameStartNotification{};

    for (auto& i : mPlayers)
    {
        auto session = i.second.getConnectionSession();
        if (session)
        {
            session->send(message);
        }

        i.second.reset();
        startPlayerTimer(i.second);
    }
}

void Game::handle(PieceResponse& pMsg)
{
    if (!mGameStarted)
    {
        return;
    }

    for (auto i : pMsg.pieceToAddList)
    {
        mTerminoCache.emplace_back((Termino)i);
    }
    for (auto& i : mPlayers)
    {
        i.second.getBoard().onEvent(board::TerminoAvailable{});
    }
}

void Game::handle(PlayerActionIndication& pMsg)
{
    if (!mGameStarted)
    {
        return;
    }

    auto foundIt = mPlayers.find(pMsg.player);
    if (mPlayers.end() != foundIt)
    {
        auto& player =foundIt->second; 
        auto& board = player.getBoard();
        if (Action::LEFT == pMsg.action)
        {
            board.onEvent(board::Move{int8_t(-pMsg.count)});
        }
        else if (Action::RIGHT == pMsg.action)
        {
            board.onEvent(board::Move{int8_t(pMsg.count)});
        }
        else if (Action::ROT_CLOCK == pMsg.action)
        {
            board.onEvent(board::Rotate{1});
        }
        else if (Action::ROT_CCLOCK == pMsg.action)
        {
            board.onEvent(board::Rotate{3});
        }
        else if (Action::ROT_180 == pMsg.action)
        {
            board.onEvent(board::Rotate{2});
        }
        else if (Action::SOFT_DROP == pMsg.action)
        {
            board.onEvent(board::SoftDrop{});
        }
        else if (Action::HARD_DROP == pMsg.action)
        {
            board.onEvent(board::Drop{});
        }
    }
}

Termino Game::onBcbGenerate(PlayerContext& pPlayer)
{
    auto& index = pPlayer.getCurrentPiece();
    if (index >= mTerminoCache.size())
    {
        TetrisProtocol message;
        message = PieceRequest{};
        auto& pieceRequest = std::get<PieceRequest>(message);
        pieceRequest.count = 100;

        send(message);
        return NONE;
    }
    return mTerminoCache[index++];
}

void Game::onBcbReplace(PlayerContext& pPlayer, std::vector<Line> pLines)
{
    auto& boardUpdateNotification = pPlayer.getBoardUpates();
    for (auto i : pLines)
    {
        boardUpdateNotification.linesToReplaceList.emplace_back(i);
    }
}

void Game::onBcbClear(PlayerContext& pPlayer, std::vector<uint8_t> pLines)
{
    auto& boardUpdateNotification = pPlayer.getBoardUpates();
    for (auto i : pLines)
    {
        boardUpdateNotification.linesToRemoveList.emplace_back(i);
    }
}

void Game::onBcbPiecePosition(PlayerContext& pPlayer, CellCoord pCoord)
{
    auto& boardUpdateNotification = pPlayer.getBoardUpates();
    boardUpdateNotification.position.emplace(PiecePosition{pCoord.first, pCoord.second});
}

void Game::onBcbPlacePiece(PlayerContext& pPlayer, Termino pPiece)
{
    auto& boardUpdateNotification = pPlayer.getBoardUpates();
    boardUpdateNotification.placement.emplace(Piece(pPiece));
}

void Game::onBcbRotate(PlayerContext& pPlayer, uint8_t pRot)
{
    auto& boardUpdateNotification = pPlayer.getBoardUpates();
    boardUpdateNotification.rotation.emplace(pRot);
}

void Game::onBcbPiecesAdded(PlayerContext& pPlayer, std::vector<Termino> pTerminos)
{
    if (!pTerminos.size())
    {
        return;
    }

    auto& boardUpdateNotification = pPlayer.getBoardUpates();
    for (auto i : pTerminos)
    {
        boardUpdateNotification.pieceToAddList.emplace_back(Piece(i));
    }
}

void Game::onBcbHold(PlayerContext& pPlayer)
{
    // auto& boardUpdateNotification = pPlayer.getBoardUpates();
    // boardUpdateNotification.hold.emplace
}

void Game::onBcbCommit(PlayerContext& pPlayer)
{
    auto& boardUpdateNotification = pPlayer.getBoardUpates();
    boardUpdateNotification.player = pPlayer.getId();
    TetrisProtocol message = std::move(boardUpdateNotification);
    boardUpdateNotification = {};
    send(message);
    for (auto& i : mPlayers)
    {
        auto playerSession = i.second.getConnectionSession();
        if (playerSession)
        {
            playerSession->send(message);
        }
    }
}

void Game::startPlayerTimer(PlayerContext& pPlayer)
{
    auto& timerId = pPlayer.getLockTimerId();
    mTimer.cancel(timerId);
    auto timediff = std::chrono::nanoseconds(mConfig.lockingTimeout)*1000*1000;
    timerId = mTimer.schedule(timediff, [this, &pPlayer]{
        trigger([this, &pPlayer](){
                onLockingTimeout(pPlayer);
            });
        });
}

void Game::onLockingTimeout(PlayerContext& pPlayer)
{
    auto& board = pPlayer.getBoard();
    board.onEvent(board::Lock{});
    if (!board.isGameOver())
    {
        startPlayerTimer(pPlayer);
        return;
    }

    pPlayer.getLockTimerId() = -1;
}

void Game::send(TetrisProtocol& pMessage)
{
    auto gmSession = mGmSession.lock();
    if (gmSession)
    {
        gmSession->send(pMessage);
    }
}

} // namespace tetris