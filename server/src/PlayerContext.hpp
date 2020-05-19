#ifndef __PLAYERCONTEXT_HPP
#define __PLAYERCONTEXT_HPP

#include <functional>

#include <bfc/FixedFunctionObject.hpp>

#include <common/StandardTetrisBoard.hpp>

#include <IConnectionSession.hpp>

namespace tetris
{

struct PlayerContext
{
public:
    PlayerContext(uint8_t pPlayerId, TetrisBoardConfig& pConfig, std::weak_ptr<IConnectionSession> pConnectionSession)
        : id(pPlayerId)
        , board(std::make_unique<StandardTetrisBoard>(pConfig, callbacks))
        , connectionSession(pConnectionSession)
    {}

    PlayerContext() = delete;

    void restart()
    {
        boardUpdates = {};
        lockTimerId = -1;
        internalMode = PLAYING; 
        currentPieceIndex = 0;
        receivedLines = 0;
        board->reset();
    }

    uint8_t id;
    std::string name = "Noname";
    enum InternalMode{PLAYING, GAMEOVER} internalMode = GAMEOVER;
    PlayerMode playerMode = PlayerMode::PLAYER;
    int receivedLines;
    int lockTimerId = -1;
    TetrisBoardCallbacks callbacks;
    uint32_t currentPieceIndex = 0;
    std::unique_ptr<ITetrisBoard> board;
    std::weak_ptr<IConnectionSession> connectionSession;
    BoardUpdateNotification boardUpdates;
    unsigned boardUpdatesCount = 0;
};

} // tetris

#endif // __PLAYERCONTEXT_HPP