#ifndef __GAMEMASTER_HPP__
#define __GAMEMASTER_HPP__

#include <interface/protocol_export.hpp>

#include <common/Terminoes.hpp>

#include <ITetrisClient.hpp>

namespace tetris
{

// TODO: StandarBoardGameMaster
class GameMaster
{
public:
    GameMaster(ITetrisClient& pClient)
        : mClient(pClient)
    {}

    GameMaster() = delete;
    GameMaster(const GameMaster&&) = delete;

    template<typename T>
    void onMsg(T&& pMsg)
    {}

    void onMsg(PieceRequest&& pMsg)
    {
        TetrisProtocol message;
        message = PieceResponse{};
        auto& pieceResponse = std::get<PieceResponse>(message);

        if (!mStarted)
        {
            // Stale PieceRequest after match end or during lobby transition — unblock server (empty list).
            mClient.send(message);
            return;
        }

        for (size_t i=0; i<pMsg.count; i++)
        {
            Piece p = Piece(mCurrentPiece);
            mCurrentPiece = Termino((mCurrentPiece+1)%7);
            pieceResponse.pieceToAddList.emplace_back(p);
        }

        mClient.send(message);
    }

    void onMsg(BoardUpdateNotification&& pMsg)
    {

    }

    void onMsg(PlayerUpdateNotification&& pMsg)
    {
    }

    void onMsg(GameEndNotification&& pMsg)
    {
        mStarted = false;
        mClient.consoleLog("[client] Game Ended!");
        mClient.notifyMatchEnded();
    }

    bool start()
    {
        if (mStarted)
        {
            mClient.consoleLog("[client] Game already started!");
            return false;
        }

        mCurrentPiece = I;

        TetrisProtocol message;
        message = GameStartIndication{};
        mClient.send(message);

        mStarted = true;
        mClient.notifyMatchStarted();
        return true;
    }

private:
    Termino mCurrentPiece;
    bool mStarted = false;
    ITetrisClient& mClient;
};

} // namespace tetris

#endif // __GAMEMASTER_HPP__