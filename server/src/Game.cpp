#include <Game.hpp>

#include <exception>
#include <functional>

namespace tetris
{

Game::Game(CreateGameRequest&& pConfig, std::weak_ptr<IConnectionSession> pGmSession, ITetrisSimulator& pSimulator)
    : mBoardConfig{pConfig.boardWidth, pConfig.boardHeight}
    , mConfig(std::move(pConfig))
    , mGmSession(pGmSession)
    , mAttacker(std::make_unique<CommonAttacker>(*this, mConfig, mPlayers, pSimulator))
    , mSimulator(pSimulator)
{
    mRunningWokerLock.unlock();
}

void Game::join(JoinRequest& pMsg, std::weak_ptr<IConnectionSession> pPlayerSession)
{
    std::function<void()> doAdd = [this, pMsg, pPlayerSession]() {
            uint8_t id = mPlayersIdCtr++;

            auto res = mPlayers.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(id, mBoardConfig, pPlayerSession));
            auto& player = res.first->second;
            if (auto sp = pPlayerSession.lock())
            {
                const std::string& dn = sp->clientDisplayName();
                if (!dn.empty())
                {
                    player.name = dn;
                }
            }
            auto& playerCallbacks = player.callbacks;
            playerCallbacks.generate = [&player, this]() -> Termino {return onBcbGenerate(player);};
            playerCallbacks.insert = [&player, this](std::vector<Line> pLines) {return onBcbInsert(player, std::move(pLines));};
            playerCallbacks.replace = [&player, this](std::vector<Line> pLines) {return onBcbReplace(player, std::move(pLines));};
            playerCallbacks.clear = [&player, this](std::vector<uint8_t> pLines) {return onBcbClear(player, std::move(pLines));};
            playerCallbacks.piecePosition = [&player, this](CellCoord pCoord) {return onBcbPiecePosition(player, pCoord);};
            playerCallbacks.placePiece = [&player, this](Termino pTermino) {return onBcbPlacePiece(player, pTermino);};
            playerCallbacks.rotate = [&player, this](uint8_t pRot) {return onBcbRotate(player, pRot);};
            playerCallbacks.piecesAdded = [&player, this](std::vector<Termino> pTerminos) {return onBcbPiecesAdded(player, std::move(pTerminos));};
            playerCallbacks.hold = [&player, this](Termino pTermino) {return onBcbHold(player, pTermino);};
            playerCallbacks.commit = [&player, this]() {return onBcbCommit(player);};
            playerCallbacks.gameOver = [&player, this]() {return onBcbGameover(player);};
            playerCallbacks.incomingAttack = [&player, this](uint8_t pLines) {return onBcbIncomingAttack(player, pLines);};

            TetrisProtocol message = JoinAccept{};
            auto& joinAccept = std::get<JoinAccept>(message);

            joinAccept.player = id;
            joinAccept.boardHeight = mBoardConfig.height;
            joinAccept.boardWidth = mBoardConfig.width;

            for (auto& i : mPlayers)
            {
                auto& player = i.second;
                joinAccept.playerToAddList.emplace_back(PlayerInfo{player.id, player.name, player.playerMode});
            }

            send(player, message);

            message = PlayerUpdateNotification{};
            auto& playerUpdateNotification = std::get<PlayerUpdateNotification>(message);

            playerUpdateNotification.playeToAddList.emplace_back(PlayerInfo{player.id, player.name, player.playerMode});

            send(message);

            for (auto& i : mPlayers)
            {
                auto& playerX = i.second;
                if (&playerX != &player)
                {
                    send(playerX, message);
                }
            }

            if (mGameStarted)
            {
                beginPlaying(player);
                if (mPlayingCount > 1)
                {
                    mAttacker->start();
                }
            }
        };

    trigger(std::move(doAdd));
}

void Game::trigger(bfc::light_function<void()> pFn)
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

void Game::handle(bfc::light_function<void()>& pFn)
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

    for (auto& i : mPlayers)
    {
        beginPlaying(i.second);
    }

    if (1 == mPlayingCount)
    {
        return;
    }

    mAttacker->start();
}

void Game::beginPlaying(PlayerContext& pPlayer)
{
    if (!pPlayer.connectionSession.lock() || PlayerMode::PLAYER != pPlayer.playerMode)
    {
        return;
    }

    TetrisProtocol message = GameStartNotification{};
    send(pPlayer, message);
    pPlayer.restart();
    mPlayingCount++;
    startPlayerTimer(pPlayer);
}

void Game::handle(PieceResponse& pMsg)
{
    if (!mGameStarted)
    {
        return;
    }

    mTerminoRequested = false;

    for (auto i : pMsg.pieceToAddList)
    {
        mTerminoCache.emplace_back((Termino)i);
    }
    for (auto& i : mPlayers)
    {
        auto& player = i.second;
        if (PlayerMode::PLAYER == player.playerMode)
        {
            player.board->onEvent(board::TerminoAvailable{});
        }
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

        if (PlayerContext::PLAYING != player.internalMode)
        {
            return;
        }

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
        if (mTerminoRequested)
        {
            return NONE;
        }
        mTerminoRequested = true;

        TetrisProtocol message;
        message = PieceRequest{};
        auto& pieceRequest = std::get<PieceRequest>(message);
        pieceRequest.count = 20;

        send(message);
        return NONE;
    }
    return mTerminoCache[index++];
}

void Game::onBcbInsert(PlayerContext& pPlayer, std::vector<Line> pLines)
{
    auto& boardUpdateNotification = pPlayer.boardUpdates;
    pPlayer.boardUpdatesCount += pLines.size() ? 1 : 0;
    for (auto i : pLines)
    {
        boardUpdateNotification.linesToInsertList.emplace_back(i);
    }
}

void Game::onBcbReplace(PlayerContext& pPlayer, std::vector<Line> pLines)
{
    auto& boardUpdateNotification = pPlayer.boardUpdates;
    pPlayer.boardUpdatesCount += pLines.size() ? 1 : 0;
    for (auto i : pLines)
    {
        boardUpdateNotification.linesToReplaceList.emplace_back(i);
    }
}

void Game::onBcbClear(PlayerContext& pPlayer, std::vector<uint8_t> pLines)
{
    auto& boardUpdateNotification = pPlayer.boardUpdates;
    pPlayer.boardUpdatesCount += pLines.size() ? 1 : 0;
    for (auto i : pLines)
    {
        boardUpdateNotification.linesToRemoveList.emplace_back(i);
    }

    mAttacker->attack(pPlayer, pLines.size());
}

void Game::onBcbPiecePosition(PlayerContext& pPlayer, CellCoord pCoord)
{
    auto& boardUpdateNotification = pPlayer.boardUpdates;
    pPlayer.boardUpdatesCount++;
    boardUpdateNotification.position.emplace(PiecePosition{pCoord.first, pCoord.second});
}

void Game::onBcbPlacePiece(PlayerContext& pPlayer, Termino pPiece)
{
    auto& boardUpdateNotification = pPlayer.boardUpdates;
    pPlayer.boardUpdatesCount++;
    boardUpdateNotification.placement.emplace(Piece(pPiece));
}

void Game::onBcbRotate(PlayerContext& pPlayer, uint8_t pRot)
{
    auto& boardUpdateNotification = pPlayer.boardUpdates;
    pPlayer.boardUpdatesCount++;
    boardUpdateNotification.rotation.emplace(pRot);
}

void Game::onBcbPiecesAdded(PlayerContext& pPlayer, std::vector<Termino> pTerminos)
{
    if (!pTerminos.size())
    {
        return;
    }

    auto& boardUpdateNotification = pPlayer.boardUpdates;
    pPlayer.boardUpdatesCount++;
    for (auto i : pTerminos)
    {
        boardUpdateNotification.pieceToAddList.emplace_back(Piece(i));
    }
}

void Game::onBcbHold(PlayerContext& pPlayer, Termino pTermino)
{
    auto& boardUpdateNotification = pPlayer.boardUpdates;
    pPlayer.boardUpdatesCount++;
    boardUpdateNotification.hold.emplace(Piece(pTermino));
}

void Game::onBcbCommit(PlayerContext& pPlayer)
{
    if (!pPlayer.boardUpdatesCount)
    {
        return;
    }
    pPlayer.boardUpdatesCount = 0;
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

    if (PlayerContext::GAMEOVER == pPlayer.internalMode)
    {
        return;
    }

    pPlayer.internalMode = PlayerContext::GAMEOVER;

    mSimulator.cancelGameTimer(pPlayer.lockTimerId);

    mPlayingCount--;

    message = GameOverNotification{};
    auto& gameOverNotification = std::get<GameOverNotification>(message);
    gameOverNotification.player = pPlayer.id;

    send(message);
    for (auto& i : mPlayers)
    {
        send(i.second, message);
    }

    concludeMatchIfOneOrNoPlayersLeft();
}

void Game::concludeMatchIfOneOrNoPlayersLeft()
{
    if (mPlayingCount == 1)
    {
        TetrisProtocol message = GameOverNotification{};
        auto& gameOverNotification = std::get<GameOverNotification>(message);
        for (auto& i : mPlayers)
        {
            auto& player = i.second;
            if (PlayerContext::PLAYING == player.internalMode)
            {
                mSimulator.cancelGameTimer(player.lockTimerId);
                gameOverNotification.player = player.id;
                send(message);
                for (auto& j : mPlayers)
                {
                    send(j.second, message);
                }
                break;
            }
        }
    }

    if (mPlayingCount <= 1)
    {
        mGameStarted = false;
        TetrisProtocol message = GameEndNotification{};
        send(message);
        for (auto& i : mPlayers)
        {
            send(i.second, message);
        }
        mAttacker->stop();
    }
}

void Game::onConnectionLost(std::shared_ptr<IConnectionSession> session)
{
    if (!session)
    {
        return;
    }
    trigger(std::function<void()>([this, session]() { handleConnectionLostNow(session); }));
}

void Game::handleConnectionLostNow(const std::shared_ptr<IConnectionSession>& session)
{
    if (auto gm = mGmSession.lock())
    {
        if (gm.get() == session.get())
        {
            shutdownDueToGmDisconnect();
            return;
        }
    }

    for (auto it = mPlayers.begin(); it != mPlayers.end(); ++it)
    {
        auto ps = it->second.connectionSession.lock();
        if (ps && ps.get() == session.get())
        {
            removePlayerAt(it, DeleteReason::DISCONNECTED);
            return;
        }
    }
}

void Game::onVoluntaryLeave(std::shared_ptr<IConnectionSession> session)
{
    if (!session)
    {
        return;
    }
    trigger(std::function<void()>([this, session]() { handleVoluntaryLeaveNow(session); }));
}

void Game::handleVoluntaryLeaveNow(const std::shared_ptr<IConnectionSession>& session)
{
    if (auto gm = mGmSession.lock())
    {
        if (gm.get() == session.get())
        {
            shutdownDueToGmDisconnect();
            return;
        }
    }

    for (auto it = mPlayers.begin(); it != mPlayers.end(); ++it)
    {
        auto ps = it->second.connectionSession.lock();
        if (ps && ps.get() == session.get())
        {
            removePlayerAt(it, DeleteReason::LEFT);
            return;
        }
    }
}

void Game::shutdownDueToGmDisconnect()
{
    mAttacker->stop();

    TetrisProtocol msg = GameEndNotification{};
    for (auto& kv : mPlayers)
    {
        mSimulator.cancelGameTimer(kv.second.lockTimerId);
    }

    for (auto& kv : mPlayers)
    {
        if (auto s = kv.second.connectionSession.lock())
        {
            try
            {
                s->send(msg);
            }
            catch (const std::exception&)
            {
            }
            s->disassociateGame();
        }
    }

    if (auto gm = mGmSession.lock())
    {
        try
        {
            gm->send(msg);
        }
        catch (const std::exception&)
        {
        }
        gm->disassociateGame();
    }

    mPlayers.clear();
    mGameStarted = false;
    mPlayingCount = 0;
    mSimulator.destroyGame(mGameId);
}

void Game::removePlayerAt(std::unordered_map<uint8_t, PlayerContext>::iterator it, DeleteReason reason)
{
    PlayerContext& removed = it->second;
    const uint8_t removedId = removed.id;
    std::shared_ptr<IConnectionSession> disconnected = removed.connectionSession.lock();

    mAttacker->stop();
    mSimulator.cancelGameTimer(removed.lockTimerId);

    const bool wasPlaying = (PlayerContext::PLAYING == removed.internalMode);
    if (wasPlaying)
    {
        mPlayingCount--;
    }

    TetrisProtocol msg = PlayerUpdateNotification{};
    auto& pun = std::get<PlayerUpdateNotification>(msg);
    pun.playerToDelete.emplace_back(PlayerToRemove{removedId, reason});

    send(msg);
    for (auto& kv : mPlayers)
    {
        send(kv.second, msg);
    }

    mPlayers.erase(it);

    if (disconnected)
    {
        disconnected->disassociateGame();
    }

    if (mGameStarted && mPlayingCount > 1)
    {
        mAttacker->start();
    }
    else
    {
        mAttacker->stop();
    }

    if (wasPlaying)
    {
        concludeMatchIfOneOrNoPlayersLeft();
    }
}

void Game::onBcbIncomingAttack(PlayerContext& pPlayer, uint8_t pLines)
{
    auto& boardUpdates = pPlayer.boardUpdates;
    pPlayer.boardUpdatesCount++;
    boardUpdates.attackIndicator.emplace(pLines);
}

void Game::startPlayerTimer(PlayerContext& pPlayer)
{
    auto& timerId = pPlayer.lockTimerId;
    mSimulator.cancelGameTimer(timerId);
    auto timediff = std::chrono::nanoseconds(mConfig.lockingTimeoutMs)*1000*1000;
    timerId = mSimulator.scheduleGameTimer(timediff, [this, &pPlayer] {
            bfc::light_function<void()> fn = [this, &pPlayer]() -> void {onLockingTimeout(pPlayer);};
            trigger(std::move(fn));
        });
}

void Game::onLockingTimeout(PlayerContext& pPlayer)
{
    if (!pPlayer.lockTimerId || PlayerContext::GAMEOVER == pPlayer.internalMode)
    {
        return;
    }

    auto& board = pPlayer.board;
    board->onEvent(board::Lock{});

    if (pPlayer.lockTimerId && PlayerContext::GAMEOVER != pPlayer.internalMode)
    {
        startPlayerTimer(pPlayer);
    }
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
