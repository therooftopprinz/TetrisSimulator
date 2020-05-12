#ifndef __GAME_HPP__
#define __GAME_HPP__

#include <cstring>
#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h> 

#include <bfc/Timer.hpp>
#include <bfc/ThreadPool.hpp>

#include <interface/protocol.hpp>

#include <common/TetrisBoard.hpp>

#include <ITetrisSimulator.hpp>
#include <IConnectionSession.hpp>

namespace tetris
{

struct GameConfig
{
    uint8_t width;
    uint8_t height;
    uint16_t lockingTimeout;
};

class PlayerContext
{
public:
    PlayerContext(uint8_t pPlayerId, TetrisBoardConfig& pConfig, std::weak_ptr<IConnectionSession> pConnectionSession)
        : mPlayerId(pPlayerId)
        , mBoard(pConfig, mCallbacks)
        , mConnectionSession(pConnectionSession)
    {}

    TetrisBoard& getBoard()
    {
        return mBoard;
    }

    uint32_t& getCurrentPiece()
    {
        return mCurrentPieceIndex;
    }

    std::shared_ptr<IConnectionSession> getConnectionSession()
    {
        return mConnectionSession.lock();
    }

    uint8_t getId()
    {
        return mPlayerId;
    }

    TetrisBoardCallbacks& getCallbacks()
    {
        return mCallbacks;
    }

    BoardUpdateNotification& getBoardUpates()
    {
        return mBoardUpdates;
    }

    int& getLockTimerId()
    {
        return mLockTimerId;
    }

    void reset()
    {
        mBoardUpdates = {};
        mLockTimerId = -1;
        mCurrentPieceIndex = 0;
        mBoard.reset();
    }

private:
    uint8_t mPlayerId;
    int mLockTimerId = -1;
    TetrisBoardCallbacks mCallbacks;
    uint32_t mCurrentPieceIndex = 0;
    TetrisBoard mBoard;
    std::weak_ptr<IConnectionSession> mConnectionSession;
    BoardUpdateNotification mBoardUpdates;
};


using GameEvent = std::variant<TetrisProtocol, bfc::LightFn<void()>>;

class Game
{
public:
    Game(const GameConfig& pConfig, std::weak_ptr<IConnectionSession> pGmSession, bfc::ThreadPool<>& pTp, bfc::Timer<>& pTimer)
        : mBoardConfig{pConfig.width, pConfig.height}
        , mConfig(pConfig)
        , mGmSession(pGmSession)
        , mTp(pTp)
        , mTimer(pTimer)
    {
        mRunningWokerLock.unlock();
    }

    Game (const Game&) = delete;
    void operator=(const Game&) = delete;

    uint8_t join(std::weak_ptr<IConnectionSession> pPlayerSession)
    {
        std::unique_lock<std::mutex> lg(mPlayersMutex);

        uint8_t id = mPlayersIdCtr++;

        auto res = mPlayers.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(id, mBoardConfig, pPlayerSession));
        auto& player = res.first->second;
        auto& playerCallbacks = player.getCallbacks();
        playerCallbacks.generate = [&player, this]() -> Termino {return onBcbGenerate(player);};
        playerCallbacks.clear = [&player, this](std::vector<uint8_t> pLines) {return onBcbClear(player, std::move(pLines));};
        playerCallbacks.piecePosition = [&player, this](CellCoord pCoord) {return onBcbPiecePosition(player, pCoord);};
        playerCallbacks.placePiece = [&player, this](Termino PTermino) {return onBcbPlacePiece(player, PTermino);};
        playerCallbacks.piecesAdded = [&player, this](std::vector<Termino> pTerminos) {return onBcbPiecesAdded(player, std::move(pTerminos));};
        playerCallbacks.hold = [&player, this]() {return onBcbHold(player);};
        playerCallbacks.commit = [&player, this]() {return onBcbCommit(player);};

        return id;
    }

    template <typename T>
    void onMsg(T&& pMsg)
    {
        std::unique_lock<std::mutex> lg(mGameEventsMutex);
        mGameEvents.emplace_back(std::move(pMsg));
        lg.unlock();

        if (!mRunningWokerLock.owns_lock() && mRunningWokerLock.try_lock())
        {
            while (true)
            {
                lg.lock();
                if (!mGameEvents.size())
                {
                    lg.unlock();
                    break;
                }
                auto event = std::move(mGameEvents.back());
                mGameEvents.pop_back();
                lg.unlock();
                runEvent(event);
            }
            mRunningWokerLock.unlock();
        }
    }

    const TetrisBoardConfig& getBoardConfig() const
    {
        return mBoardConfig;
    }


private:

    void trigger(bfc::LightFn<void()> pFn)
    {
        onMsg(std::move(pFn));
    }

    void runEvent(GameEvent& pEvent)
    {
        std::unique_lock<std::mutex> lg(mPlayersMutex);
        std::visit([this](auto& pEvent){handle(pEvent);}, pEvent);
    }

    void handle(bfc::LightFn<void()>& pFn)
    {
        pFn();
    }

    template<typename T>
    void handle(T&)
    {}

    void handle(TetrisProtocol& pMsg)
    {
        std::visit([this](auto& pMsg){handle(pMsg);}, pMsg);
    }

    void handle(GameStartIndication& pMsg)
    {
        mTerminoCache.clear();
        for (auto& i : mPlayers)
        {
            i.second.reset();
            startPlayerTimer(i.second);
        }
    }

    void handle(PieceResponse& pMsg)
    {
        for (auto i : pMsg.pieceToAddList)
        {
            mTerminoCache.emplace_back((Termino)i);
        }
        for (auto& i : mPlayers)
        {
            i.second.getBoard().onEvent(board::TerminoAvailable{});
        }
    }

    Termino onBcbGenerate(PlayerContext& pPlayer)
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

    void onBcbClear(PlayerContext& pPlayer, std::vector<uint8_t> pLines)
    {
        auto& boardUpdateNotification = pPlayer.getBoardUpates();
        for (auto i : pLines)
        {
            boardUpdateNotification.linesToRemoveList.emplace_back(i);
        }
    }

    void onBcbPiecePosition(PlayerContext& pPlayer, CellCoord pCoord)
    {
        auto& boardUpdateNotification = pPlayer.getBoardUpates();
        boardUpdateNotification.position.emplace(PiecePosition{pCoord.first, pCoord.second});
    }

    void onBcbPlacePiece(PlayerContext& pPlayer, Termino pPiece)
    {
        auto& boardUpdateNotification = pPlayer.getBoardUpates();
        boardUpdateNotification.placement.emplace(Piece(pPiece));
    }

    void onBcbPiecesAdded(PlayerContext& pPlayer, std::vector<Termino> pTerminos)
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

    void onBcbHold(PlayerContext& pPlayer)
    {
        auto& boardUpdateNotification = pPlayer.getBoardUpates();
        boardUpdateNotification.action.emplace(PlayerAction{1, Action::HOLD});
    }

    void onBcbCommit(PlayerContext& pPlayer)
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

    void startPlayerTimer(PlayerContext& pPlayer)
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

    void onLockingTimeout(PlayerContext& pPlayer)
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

    void send(TetrisProtocol& pMessage)
    {
        auto gmSession = mGmSession.lock();
        if (gmSession)
        {
            gmSession->send(pMessage);
        }
    }

    std::deque<GameEvent> mGameEvents;
    std::mutex mGameEventsMutex;
    std::mutex mRunningWokerMutex;
    std::unique_lock<std::mutex> mRunningWokerLock = std::unique_lock<std::mutex>(mRunningWokerMutex);

    std::vector<Termino> mTerminoCache;

    TetrisBoardConfig mBoardConfig;
    GameConfig mConfig;
    std::weak_ptr<IConnectionSession> mGmSession;

    std::unordered_map<uint8_t, PlayerContext> mPlayers;
    std::vector<uint8_t> mFreePlayersId;
    uint8_t mPlayersIdCtr = 0;
    std::mutex mPlayersMutex;

    std::unordered_map<int, bfc::LightFn<void()>> mTriggers;
    int mTriggersId = 0;

    bfc::ThreadPool<>& mTp;
    bfc::Timer<>& mTimer;
};

} // namespace tetris

#endif // __GAME_HPP__