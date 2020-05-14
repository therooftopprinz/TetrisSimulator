#ifndef __GAME_HPP__
#define __GAME_HPP__

#include <cstring>
#include <stdexcept>
#include <variant>

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
    Game(const GameConfig& pConfig, std::weak_ptr<IConnectionSession> pGmSession, bfc::ThreadPool<>& pTp, bfc::Timer<>& pTimer);
    Game (const Game&) = delete;
    void operator=(const Game&) = delete;

    uint8_t join(std::weak_ptr<IConnectionSession> pPlayerSession);

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

    const TetrisBoardConfig& getBoardConfig() const;

private:

    void trigger(bfc::LightFn<void()> pFn);
    void runEvent(GameEvent& pEvent);

    template<typename T>
    void handle(T&)
    {}

    void handle(bfc::LightFn<void()>& pFn);
    void handle(TetrisProtocol& pMsg);
    void handle(GameStartIndication& pMsg);
    void handle(PieceResponse& pMsg);
    void handle(PlayerActionIndication& pMsg);

    Termino onBcbGenerate(PlayerContext& pPlayer);
    void onBcbReplace(PlayerContext& pPlayer, std::vector<Line> pLines);
    void onBcbClear(PlayerContext& pPlayer, std::vector<uint8_t> pLines);
    void onBcbPiecePosition(PlayerContext& pPlayer, CellCoord pCoord);
    void onBcbPlacePiece(PlayerContext& pPlayer, Termino pPiece);
    void onBcbRotate(PlayerContext& pPlayer, uint8_t pRot);
    void onBcbPiecesAdded(PlayerContext& pPlayer, std::vector<Termino> pTerminos);
    void onBcbHold(PlayerContext& pPlayer);
    void onBcbCommit(PlayerContext& pPlayer);

    void startPlayerTimer(PlayerContext& pPlayer);
    void onLockingTimeout(PlayerContext& pPlayer);

    void send(TetrisProtocol& pMessage);

    std::deque<GameEvent> mGameEvents;
    std::mutex mGameEventsMutex;
    std::mutex mRunningWokerMutex;
    std::unique_lock<std::mutex> mRunningWokerLock = std::unique_lock<std::mutex>(mRunningWokerMutex);

    std::vector<Termino> mTerminoCache;

    TetrisBoardConfig mBoardConfig;
    GameConfig mConfig;
    std::weak_ptr<IConnectionSession> mGmSession;

    bool mGameStarted = false;

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