#include <Game.hpp>

namespace tetris
{

Game::Game(CreateGameRequest&& pConfig, std::weak_ptr<IConnectionSession> pGmSession, bfc::ThreadPool<>& pTp, bfc::Timer<>& pTimer)
    : mBoardConfig{pConfig.boardWidth, pConfig.boardHeight}
    , mConfig(std::move(pConfig))
    , mGmSession(pGmSession)
    , mAttacker(std::make_unique<CommonAttacker>(*this, mConfig, mPlayers, pTp, pTimer))
    , mTp(pTp)
    , mTimer(pTimer)
{
    mRunningWokerLock.unlock();
}

void Game::join(JoinRequest& pMsg, std::weak_ptr<IConnectionSession> pPlayerSession)
{
    std::function<void()> doAdd = [this, pMsg, pPlayerSession]() {
            uint8_t id = mPlayersIdCtr++;

            auto res = mPlayers.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(id, mBoardConfig, pPlayerSession));
            auto& player = res.first->second;
            auto& playerCallbacks = player.callbacks;
            playerCallbacks.generate = [&player, this]() -> Termino {return onBcbGenerate(player);};
            playerCallbacks.replace = [&player, this](std::vector<Line> pLines) {return onBcbReplace(player, std::move(pLines));};
            playerCallbacks.clear = [&player, this](std::vector<uint8_t> pLines) {return onBcbClear(player, std::move(pLines));};
            playerCallbacks.piecePosition = [&player, this](CellCoord pCoord) {return onBcbPiecePosition(player, pCoord);};
            playerCallbacks.placePiece = [&player, this](Termino PTermino) {return onBcbPlacePiece(player, PTermino);};
            playerCallbacks.rotate = [&player, this](uint8_t pRot) {return onBcbRotate(player, pRot);};
            playerCallbacks.piecesAdded = [&player, this](std::vector<Termino> pTerminos) {return onBcbPiecesAdded(player, std::move(pTerminos));};
            playerCallbacks.hold = [&player, this]() {return onBcbHold(player);};
            playerCallbacks.commit = [&player, this]() {return onBcbCommit(player);};
            playerCallbacks.gameOver = [&player, this]() {return onBcbGameover(player);};
            playerCallbacks.incomingAttack = [&player, this](uint8_t pLines) {return onBcbIncomingAttack(player, pLines);};

            TetrisProtocol message = JoinAccept{};
            auto& joinAccept = std::get<JoinAccept>(message);

            joinAccept.playerId = id;
            joinAccept.boardHeight = mBoardConfig.height;
            joinAccept.boardWidth = mBoardConfig.width;

            for (auto& i : mPlayers)
            {
                auto& player = i.second;
                joinAccept.playerToAddList.emplace_back(PlayerInfo{id, player.name, player.mode});
            }

            send(player, message);
        };
    trigger(std::move(doAdd));
}

void Game::trigger(bfc::LightFn<void()> pFn)
{
    onMsg(std::move(pFn));
}

void Game::trigger(std::function<void()> pFn)
{
    onMsg(std::move(pFn));
}

void Game::runEvent(GameEvent& pEvent)
{
    std::visit([this](auto& pEvent){handle(pEvent);}, pEvent);
}

void Game::handle(bfc::LightFn<void()>& pFn)
{
    pFn();
}

void Game::handle(std::function<void()>& pFn)
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
    mPlayingCount = 0;

    mTerminoCache.clear();
    TetrisProtocol message = GameStartNotification{};

    for (auto& i : mPlayers)
    {
        auto& player = i.second;
        if (player.connectionSession.lock())
        {
            send(player, message);
            player.restart();
            mPlayingCount++;
            startPlayerTimer(player);
        }
    }

    if (1 == mPlayingCount)
    {
        return;
    }

    mAttacker->start();
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
        i.second.board->onEvent(board::TerminoAvailable{});
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
        auto& board = player.board;
        if (Action::LEFT == pMsg.action)
        {
            board->onEvent(board::Move{int8_t(-pMsg.count)});
        }
        else if (Action::RIGHT == pMsg.action)
        {
            board->onEvent(board::Move{int8_t(pMsg.count)});
        }
        else if (Action::ROT_CLOCK == pMsg.action)
        {
            board->onEvent(board::Rotate{1});
        }
        else if (Action::ROT_CCLOCK == pMsg.action)
        {
            board->onEvent(board::Rotate{-1});
        }
        else if (Action::ROT_180 == pMsg.action)
        {
            board->onEvent(board::Rotate{2});
        }
        else if (Action::SOFT_DROP == pMsg.action)
        {
            board->onEvent(board::SoftDrop{});
        }
        else if (Action::HARD_DROP == pMsg.action)
        {
            board->onEvent(board::Drop{});
        }
        else if (Action::HOLD == pMsg.action)
        {
            board->onEvent(board::Hold{});
        }
    }
}

Termino Game::onBcbGenerate(PlayerContext& pPlayer)
{
    auto& index = pPlayer.currentPieceIndex;
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
    auto& boardUpdateNotification = pPlayer.boardUpdates;
    for (auto i : pLines)
    {
        boardUpdateNotification.linesToReplaceList.emplace_back(i);
    }
}

void Game::onBcbClear(PlayerContext& pPlayer, std::vector<uint8_t> pLines)
{
    auto& boardUpdateNotification = pPlayer.boardUpdates;
    for (auto i : pLines)
    {
        boardUpdateNotification.linesToRemoveList.emplace_back(i);
    }

    mAttacker->attack(pPlayer, pLines.size());
}

void Game::onBcbPiecePosition(PlayerContext& pPlayer, CellCoord pCoord)
{
    auto& boardUpdateNotification = pPlayer.boardUpdates;
    boardUpdateNotification.position.emplace(PiecePosition{pCoord.first, pCoord.second});
}

void Game::onBcbPlacePiece(PlayerContext& pPlayer, Termino pPiece)
{
    auto& boardUpdateNotification = pPlayer.boardUpdates;
    boardUpdateNotification.placement.emplace(Piece(pPiece));
}

void Game::onBcbRotate(PlayerContext& pPlayer, uint8_t pRot)
{
    auto& boardUpdateNotification = pPlayer.boardUpdates;
    boardUpdateNotification.rotation.emplace(pRot);
}

void Game::onBcbPiecesAdded(PlayerContext& pPlayer, std::vector<Termino> pTerminos)
{
    if (!pTerminos.size())
    {
        return;
    }

    auto& boardUpdateNotification = pPlayer.boardUpdates;
    for (auto i : pTerminos)
    {
        boardUpdateNotification.pieceToAddList.emplace_back(Piece(i));
    }
}

void Game::onBcbHold(PlayerContext& pPlayer)
{
    // auto& boardUpdateNotification = pPlayer.boardUpdates;
    // boardUpdateNotification.hold.emplace
}

void Game::onBcbCommit(PlayerContext& pPlayer)
{
    auto& boardUpdateNotification = pPlayer.boardUpdates;
    boardUpdateNotification.player = pPlayer.id;
    TetrisProtocol message = std::move(boardUpdateNotification);
    boardUpdateNotification = {};
    send(message);
    for (auto& i : mPlayers)
    {
        send(i.second, message);
    }
}

void Game::onBcbGameover(PlayerContext& pPlayer)
{
    TetrisProtocol message;

    if (PlayerContext::ACTIVE == pPlayer.playState)
    {
        pPlayer.playState = PlayerContext::IDLE;
    }

    if (--mPlayingCount)
    {
        message = GameOverNotification{};
        auto& gameOverNotification = std::get<GameOverNotification>(message);
        gameOverNotification.player = pPlayer.id;

        send(message);
        for (auto& i : mPlayers)
        {
            send(i.second, message);
        }
    }

    if (!mPlayingCount)
    {
        mGameStarted = false;

        message = GameEndNotification{};

        send(message);
        for (auto& i : mPlayers)
        {
            send(i.second, message);
        }
    }
    
}

void Game::onBcbIncomingAttack(PlayerContext& pPlayer, uint8_t pLines)
{
    auto& boardUpdates = pPlayer.boardUpdates;
    boardUpdates.attackIndicator.emplace(pLines);
}

void Game::startPlayerTimer(PlayerContext& pPlayer)
{
    auto& timerId = pPlayer.lockTimerId;
    mTimer.cancel(timerId);
    auto timediff = std::chrono::nanoseconds(mConfig.lockingTimeoutMs)*1000*1000;
    timerId = mTimer.schedule(timediff, [this, &pPlayer] {
            bfc::LightFn<void()> fn = [this, &pPlayer]() -> void {onLockingTimeout(pPlayer);};
            trigger(std::move(fn));
        });
}

void Game::onLockingTimeout(PlayerContext& pPlayer)
{
    auto& board = pPlayer.board;
    board->onEvent(board::Lock{});
    if (!board->isGameOver())
    {
        startPlayerTimer(pPlayer);
        return;
    }

    pPlayer.lockTimerId = -1;
}

void Game::send(TetrisProtocol& pMessage)
{
    auto gmSession = mGmSession.lock();
    if (gmSession)
    {
        gmSession->send(pMessage);
    }
}

void Game::send(PlayerContext& pPlayer, TetrisProtocol& pMessage)
{
    auto session = pPlayer.connectionSession.lock();
    if (session)
    {
        session->send(pMessage);
    }
}

} // namespace tetris
