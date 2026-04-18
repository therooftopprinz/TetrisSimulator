#ifndef __GAME_HPP__
#define __GAME_HPP__

#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <deque>

#include <sys/socket.h>
#include <netinet/in.h> 

#include <bfc/function.hpp>

#include <interface/protocol_export.hpp>
#include <tetris_log.hpp>

#include <common/StandardTetrisBoard.hpp>

#include <ITetrisSimulator.hpp>
#include <IConnectionSession.hpp>
#include <CommonAttacker.hpp>
#include <PlayerContext.hpp>

namespace tetris
{

class Game
{
public:
    Game(CreateGameRequest&& pConfig, std::weak_ptr<IConnectionSession> pGmSession, ITetrisSimulator& pSimulator);
    Game (const Game&) = delete;
    void operator=(const Game&) = delete;

    void setGameId(uint32_t pId) { mGameId = pId; }

    uint32_t gameId() const { return mGameId; }

    void join(JoinRequest& pMsg, std::weak_ptr<IConnectionSession> pPlayerSession);

    void onConnectionLost(std::shared_ptr<IConnectionSession> session);

    void onVoluntaryLeave(std::shared_ptr<IConnectionSession> session);

    template <typename T>
    void onMsg(T&& pMsg)
    {
        handle(pMsg);
    }

private:

    template<typename T>
    void handle(T& pMsg)
    {
        std::string stred;
        str("msg", pMsg, stred, true);
        logless::log(tetris_logger(), logless::WARNING, logless::LOGALL,
            "Game[gameId=%u;]: unhandled message: %s;", mGameId, stred.c_str());
    }

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

    void concludeMatchIfOneOrNoPlayersLeft();
    void shutdownDueToGmDisconnect();
    void removePlayerAt(std::unordered_map<uint8_t, PlayerContext>::iterator it, DeleteReason reason);

    void beginPlaying(PlayerContext& pPlayer);
    void startPlayerTimer(PlayerContext& pPlayer);
    void onLockingTimeout(PlayerContext& pPlayer);

    void send(TetrisProtocol& pMessage);
    void send(PlayerContext& pPlayer, TetrisProtocol& pMessage);

    std::vector<Termino> mTerminoCache;
    bool mTerminoRequested = false;

    TetrisBoardConfig mBoardConfig;
    CreateGameRequest mConfig;
    std::weak_ptr<IConnectionSession> mGmSession;

    uint32_t mGameId = 0;

    bool mGameStarted = false;
    unsigned mPlayingCount;
    std::unordered_map<uint8_t, PlayerContext> mPlayers;
    std::vector<uint8_t> mFreePlayersId;
    uint8_t mPlayersIdCtr = 0;

    std::unique_ptr<IAttacker> mAttacker;

    ITetrisSimulator& mSimulator;
};

} // namespace tetris

#endif // __GAME_HPP__