#ifndef __GAMEMASTER_HPP__
#define __GAMEMASTER_HPP__

#include <interface/protocol.hpp>

#include <common/Terminoes.hpp>

#include <ITetrisClient.hpp>

namespace tetris
{


class GameMaster
{
public:
    GameMaster(ITetrisClient& pClient)
        : mClient(pClient)
    {}

    GameMaster() = delete;
    GameMaster(const GameMaster&&) = delete;

    void onMsg(PieceRequest& pMsg)
    {
        TetrisProtocol message;
        message = PieceResponse{};
        auto& pieceResponse = std::get<PieceResponse>(message);

        if (!mStarted)
        {
            mClient.consoleLog("[GameMaster]: PieceRequest invalid when game is not started!");
            throw std::runtime_error("server-client expectation mismatch!");
        }

        for (size_t i=0; i<pMsg.count; i++)
        {
            unsigned p = i%Termino::MAX;
            pieceResponse.pieceToAddList.emplace_back(Piece(p));
        }

        mClient.send(message);
    }

    void onMsg(BoardUpdateNotification& pMsg)
    {

    }

    void onMsg(PlayerUpdateNotification& pMsg)
    {
    }

    bool start()
    {
        if (mStarted)
        {
            mClient.consoleLog("[GameMaster]: Game already started!");
            return false;
        }

        TetrisProtocol message;
        message = GameStartIndication{};
        mClient.send(message);

        mStarted = true;
        return true;
    }

private:
    Termino mCurrentPiece = I;
    bool mStarted = false;
    ITetrisClient& mClient;
};

} // namespace tetris

#endif // __GAMEMASTER_HPP__