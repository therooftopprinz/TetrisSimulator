#ifndef __GAME_HPP__
#define __GAME_HPP__

#include <cstring>
#include <stdexcept>
#include <variant>
#include <functional>

#include <sys/socket.h>
#include <netinet/in.h> 

#include <bfc/Timer.hpp>
#include <bfc/ThreadPool.hpp>
#include <bfc/FixedFunctionObject.hpp>

#include <interface/protocol.hpp>

#include <common/StandardTetrisBoard.hpp>

#include <ITetrisSimulator.hpp>
#include <IConnectionSession.hpp>
#include <CommonAttacker.hpp>
#include <PlayerContext.hpp>

namespace tetris
{

using GameEvent = std::variant<TetrisProtocol, bfc::LightFn<void()>, std::function<void()>>;

class Game : public IExecutor
{
public:
    Game(CreateGameRequest&& pConfig, std::weak_ptr<IConnectionSession> pGmSession, bfc::ThreadPool<>& pTp, bfc::Timer<>& pTimer);
    Game (const Game&) = delete;
    void operator=(const Game&) = delete;

    void join(JoinRequest& pMsg, std::weak_ptr<IConnectionSession> pPlayerSession);

    template <typename T>
    void onMsg(T&& pMsg)
    {
        std::unique_lock<std::mutex> lg(mGameEventsMutex);
        mGameEvents.emplace_back(std::forward<T>(pMsg));
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

private:

    void trigger(bfc::LightFn<void()> pFn);
    void trigger(std::function<void()> pFn);
    void runEvent(GameEvent& pEvent);

    template<typename T>
    void handle(T&)
    {}

    void handle(bfc::LightFn<void()>& pFn);
    void handle(std::function<void()>& pFn);
    void handle(TetrisProtocol& pMsg);
    void handle(GameStartIndication& pMsg);
    void handle(PieceResponse& pMsg);
    void handle(PlayerActionIndication& pMsg);

    Termino onBcbGenerate(PlayerContext& pPlayer);
    void onBcbInsert(PlayerContext& pPlayer, std::vector<Line> pLines);
    void onBcbReplace(PlayerContext& pPlayer, std::vector<Line> pLines);
    void onBcbClear(PlayerContext& pPlayer, std::vector<uint8_t> pLines);
    void onBcbPiecePosition(PlayerContext& pPlayer, CellCoord pCoord);
    void onBcbPlacePiece(PlayerContext& pPlayer, Termino pPiece);
    void onBcbRotate(PlayerContext& pPlayer, uint8_t pRot);
    void onBcbPiecesAdded(PlayerContext& pPlayer, std::vector<Termino> pTerminos);
    void onBcbHold(PlayerContext& pPlayer, Termino pTermino);
    void onBcbCommit(PlayerContext& pPlayer);
    void onBcbGameover(PlayerContext& pPlayer);
    void onBcbIncomingAttack(PlayerContext& pPlayer, uint8_t);

    void startPlayerTimer(PlayerContext& pPlayer);
    void onLockingTimeout(PlayerContext& pPlayer);

    void send(TetrisProtocol& pMessage);
    void send(PlayerContext& pPlayer, TetrisProtocol& pMessage);

    std::deque<GameEvent> mGameEvents;
    std::mutex mGameEventsMutex;
    std::mutex mRunningWokerMutex;
    std::unique_lock<std::mutex> mRunningWokerLock = std::unique_lock<std::mutex>(mRunningWokerMutex);

    std::vector<Termino> mTerminoCache;

    TetrisBoardConfig mBoardConfig;
    CreateGameRequest mConfig;
    std::weak_ptr<IConnectionSession> mGmSession;

    bool mGameStarted = false;
    unsigned mPlayingCount;
    std::unordered_map<uint8_t, PlayerContext> mPlayers;
    std::vector<uint8_t> mFreePlayersId;
    uint8_t mPlayersIdCtr = 0;

    std::unique_ptr<IAttacker> mAttacker;

    std::unordered_map<int, bfc::LightFn<void()>> mTriggers;
    int mTriggersId = 0;

    bfc::ThreadPool<>& mTp;
    bfc::Timer<>& mTimer;
};

} // namespace tetris

#endif // __GAME_HPP__