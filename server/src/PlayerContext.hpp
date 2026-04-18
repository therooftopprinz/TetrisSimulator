#ifndef __PLAYERCONTEXT_HPP
#define __PLAYERCONTEXT_HPP

#include <functional>
#include <optional>

#include <GameTimerId.hpp>

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
        lockTimerId.reset();
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
    std::optional<GameTimerId> lockTimerId;
    TetrisBoardCallbacks callbacks;
    uint32_t currentPieceIndex = 0;
    std::unique_ptr<ITetrisBoard> board;
    std::weak_ptr<IConnectionSession> connectionSession;
    BoardUpdateNotification boardUpdates;
    unsigned boardUpdatesCount = 0;
};

} // tetris

#endif // __PLAYERCONTEXT_HPP