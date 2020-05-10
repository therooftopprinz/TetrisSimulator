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

template <typename T>
class Player : public ITetrisBoardCallbacks
{
public:
    Player(uint8_t pPlayerId, T& pGame, IConnectionSession& pConnectionSession)
        : mPlayerId(pPlayerId)
        , mBoard(pGame.getBoardConfig(), this)
        , mGame(pGame)
    {}

    std::pair<TetrisBoard&, std::mutex&> getBoard()
    {
        return std::pair<TetrisBoard&, std::mutex&>(mBoard, mMutex);
    }

    uint32_t getCurrentPiece()
    {
        return mCurrentPieceIndex;
    }

    IConnectionSession& getConnectionSession()
    {
        return *mConnectionSession;
    }

    uint8_t getId()
    {
        return mPlayerId;
    }

private:
    Termino generate()
    {
        auto term = mGame.generate(*this);
        if (NONE != term)
        {
            mCurrentPieceIndex++;
        }
        return term;
    }

    void clear(std::vector<uint8_t> pLines)
    {
        mGame.clear(*this, std::move(pLines));
    }

    void piecePosition(CellCoord pCoord)
    {
        mGame.piecePosition(*this, pCoord);
    }

    void newPiece(Termino pPiece)
    {
        mGame.newPiece(*this, pPiece);
    }

    void hold()
    {
        mGame.hold(*this);
    }

    std::mutex mMutex;

    uint8_t mPlayerId;
    uint32_t mCurrentPieceIndex = 0;
    TetrisBoard mBoard;
    T& mGame;
    IConnectionSession* mConnectionSession;
};

class Game
{
public:
    using GamePlayer = Player<Game>;
    Game(const GameConfig& pConfig, IConnectionSession& pGmSession, bfc::ThreadPool<>& pTp, bfc::Timer<>& pTimer)
        : mConfig(pConfig)
        , mGmSession(&pGmSession)
        , mTp(pTp)
        , mTimer(pTimer)
    {}

    Game (const Game&) = delete;
    void operator=(const Game&) = delete;

    uint8_t join(IConnectionSession& pPlayerSession)
    {
        uint8_t id = mPlayersIdCtr++;

        mPlayers.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(id, *this, pPlayerSession));
        return id;
    }

    template <typename T>
    void onMsg(T&)
    {
    }

    template <typename T>
    void onMsg(GameStartIndication& pMsg)
    {
        std::unique_lock<std::mutex> lg(mPlayersMutex);
        for (auto& i : mPlayers)
        {
            auto board = i.second.getBoard();
            board.first.restart();
        }
    }

    void onMsg(PieceResponse& pMsg)
    {
        {
            std::unique_lock<std::mutex> lg(mTerminoCacheMutex);
            for (auto i : pMsg.pieceToAddList)
            {
                mTerminoCache.emplace_back((Termino)i);
            }
        }
        {
            std::unique_lock<std::mutex> lg(mPlayersMutex);
            for (auto& i : mPlayers)
            {
                auto board = i.second.getBoard();
                std::unique_lock<std::mutex> lg(board.second);
                board.first.onEvent(board::TerminoAvailable{});
            }
        }
    }

    Termino generate(GamePlayer& pPlayer)
    {
        std::unique_lock<std::mutex> lg(mTerminoCacheMutex);
        auto index = pPlayer.getCurrentPiece();
        if (index >= mTerminoCache.size())
        {
            TetrisProtocol message;
            message = PieceRequest{};
            auto& pieceRequest = std::get<PieceRequest>(message);
            pieceRequest.count = 100;

            mGmSession->send(message);
            return NONE;
        }
        return mTerminoCache[index];
    }

    void clear(GamePlayer& pPlayer, std::vector<uint8_t> pLines)
    {
    }

    void piecePosition(GamePlayer& pPlayer, CellCoord pCoord)
    {

    }

    void newPiece(GamePlayer& pPlayer, Termino pPiece)
    {
    }

    void hold(GamePlayer& pPlayer)
    {
    }

    TetrisBoardConfig& getBoardConfig()
    {
        return mBoardConfig;
    }

private:
    std::vector<Termino> mTerminoCache;
    std::mutex mTerminoCacheMutex;

    TetrisBoardConfig mBoardConfig;
    GameConfig mConfig;
    IConnectionSession* mGmSession;

    std::unordered_map<uint8_t, GamePlayer> mPlayers;
    std::vector<uint8_t> mFreePlayersId;
    uint8_t mPlayersIdCtr = 0;
    std::mutex mPlayersMutex;

    bfc::ThreadPool<>& mTp;
    bfc::Timer<>& mTimer;
};

} // namespace tetris

#endif // __GAME_HPP__