#ifndef __PLAYER_HPP__
#define __PLAYER_HPP__

#include <interface/protocol_export.hpp>

#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

#include <common/Terminoes.hpp>
#include <common/StandardTetrisBoard.hpp>

#include <ITetrisClient.hpp>

namespace tetris
{

class NcursesUi;

struct PlayerContext
{
    PlayerContext(const JoinAccept& pConfig)
        : bitmap(pConfig.boardWidth, pConfig.boardHeight)
    {}

    uint8_t id;
    std::string name;
    Bitmap bitmap;
    std::deque<Termino> queue;
    std::optional<Termino> current;
    std::optional<Termino> hold;
    std::optional<uint8_t> incoming;
    uint8_t rotation;
    int8_t x;
    int8_t y;
    int8_t floor;
    bool dead = false;

    TransformFn transformer;
    traits::CheckerFn* checkerFn = nullptr;
    traits::ApplierFn applier = nullptr;

    void reset()
    {
        bitmap.reset();
        queue.clear();
        current = {};
        hold = {};
        incoming = {};

        transformer = {};
        applier = nullptr;
        checkerFn = nullptr;
        dead = false;
    }

    void initializeCurrentTermino(Termino pTerm)
    {
        current.emplace(pTerm);
        rotation = 0;
        auto& termino = traits::gTerminoTraitsMap[*current];
        auto& rotator = std::get<3>(termino);
        checkerFn = &std::get<2>(termino);
        applier = std::get<5>(termino);
        transformer = [this, &rotator](CellCoord pCoord) {
                return rotator(this->rotation, pCoord);
            };
    }
};

class Player
{
    friend class NcursesUi;

public:
    Player(ITetrisClient& pClient, JoinAccept&& pConfig)
        : mClient(pClient)
        , mPlayerId(pConfig.player)
        , mConfig(std::move(pConfig))
    {
        for (auto& i : mConfig.playerToAddList)
        {
            auto res = mPlayers.emplace(std::piecewise_construct, std::forward_as_tuple(i.id), std::forward_as_tuple(mConfig));
            auto& player = res.first->second;
            player.id = i.id;
            player.name = i.name;
        }

        bindGameplayInput();
        draw();
    }

    void bindGameplayInput()
    {
        mClient.disableConsole();
        mClient.setKeyHandler([this](char key) -> void {
                if (mClient.gameplayKeyRoutesToConsole(key))
                {
                    mClient.consoleIn(key);
                    return;
                }
                keyIn(key);
            });
    }

    Player() = delete;
    Player(const Player&&) = delete;

    bool isGameStarted() const noexcept
    {
        return mGameStarted;
    }

    template<typename T>
    void onMsg(T&& pMsg)
    {
        mLatencyMeasEnd = std::chrono::high_resolution_clock::now();
        if (mLatencyMeasEnabled)
        {
            mLatencyMeas = std::chrono::duration_cast<std::chrono::milliseconds>(mLatencyMeasEnd-mLatencyMeasStart);
        }
        mLatencyMeasEnabled = false;

        handle(std::move(pMsg));
    }

private:
    template<typename T>
    void handle(T&& pMsg)
    {
    }

    void onEvent(const board::SoftDrop& pEvent)
    {
        if (!mGameStarted)
        {
            return;
        }

        TetrisProtocol message = PlayerActionIndication{};
        auto& playerActionIndication = std::get<PlayerActionIndication>(message);
        playerActionIndication.player = mPlayerId;
        playerActionIndication.count = 1;
        playerActionIndication.action = Action::SOFT_DROP;
        mClient.send(message);
    }

    void onEvent(const board::Move& pEvent)
    {
        if (!mGameStarted)
        {
            return;
        }

        TetrisProtocol message = PlayerActionIndication{};
        auto& playerActionIndication = std::get<PlayerActionIndication>(message);
        playerActionIndication.player = mPlayerId;
        playerActionIndication.count = std::abs(pEvent.offset);
        if (pEvent.offset > 0)
        {
            playerActionIndication.action = Action::RIGHT;
        }
        else
        {
            playerActionIndication.action = Action::LEFT;
        }
        mClient.send(message);
    }

    void onEvent(const board::Rotate& pEvent)
    {
        if (!mGameStarted)
        {
            return;
        }

        TetrisProtocol message = PlayerActionIndication{};
        auto& playerActionIndication = std::get<PlayerActionIndication>(message);
        playerActionIndication.player = mPlayerId;
        playerActionIndication.count = 1;
        if (1==pEvent.count)
        {
            playerActionIndication.action = Action::ROT_CLOCK;
        }
        else if (2==pEvent.count)
        {
            playerActionIndication.action = Action::ROT_180;
        }
        else if (3==pEvent.count)
        {
            playerActionIndication.action = Action::ROT_CCLOCK;
        }
        mClient.send(message);
    }

    void onEvent(const board::Drop& pEvent)
    {
        if (!mGameStarted)
        {
            return;
        }

        TetrisProtocol message = PlayerActionIndication{};
        auto& playerActionIndication = std::get<PlayerActionIndication>(message);
        playerActionIndication.player = mPlayerId;
        playerActionIndication.count = 1;
        playerActionIndication.action = Action::HARD_DROP;
        mClient.send(message);
    }

    void onEvent(const board::Hold& pEvent)
    {
        if (!mGameStarted)
        {
            return;
        }

        TetrisProtocol message = PlayerActionIndication{};
        auto& playerActionIndication = std::get<PlayerActionIndication>(message);
        playerActionIndication.player = mPlayerId;
        playerActionIndication.count = 1;
        playerActionIndication.action = Action::HOLD;
        mClient.send(message);
    }

    void handle(GameStartNotification&& pMsg)
    {
        mClient.notifyMatchStarted();
        mGameStarted = true;
        mGameOverSequence.clear();
        mCurrentTarget.reset();
        for (auto& i : mPlayers)
        {
            i.second.reset();
        }
    }

    void handle(GameOverNotification&& pMsg)
    {
        if (!mGameStarted)
        {
            return;
        }
        mGameOverSequence.push_back(pMsg.player);

        auto foundIt = mPlayers.find(pMsg.player);
        if (foundIt != mPlayers.end())
        {
            // Multiplayer: the server sends a final GameOver for the last standing player;
            // handle(GameEndNotification) clears dead for that id.
            foundIt->second.dead = true;
        }
        if (mCurrentTarget && *mCurrentTarget == pMsg.player)
        {
            mCurrentTarget.reset();
        }
        draw();
    }

    void handle(GameEndNotification&& pMsg)
    {
        if (mPlayers.size() > 1 && !mGameOverSequence.empty())
        {
            const uint8_t last = mGameOverSequence.back();
            auto winIt = mPlayers.find(last);
            if (winIt != mPlayers.end())
            {
                winIt->second.dead = false;
            }
        }
        mGameOverSequence.clear();

        mGameStarted = false;
        mClient.consoleLog("[client] Match ended.");
        mClient.notifyMatchEnded();
    }

    void handle(PlayerUpdateNotification&& pMsg)
    {
        for (auto& i : pMsg.playeToAddList)
        {
            auto res = mPlayers.emplace(std::piecewise_construct, std::forward_as_tuple(i.id), std::forward_as_tuple(mConfig));
            auto& player = res.first->second;
            player.id = i.id;
            player.name = i.name;
        }

        for (const auto& rem : pMsg.playerToDelete)
        {
            const bool self = (rem.id == mPlayerId);
            if (mCurrentTarget && *mCurrentTarget == rem.id)
            {
                mCurrentTarget.reset();
            }
            mPlayers.erase(rem.id);
            if (self)
            {
                if (DeleteReason::DISCONNECTED == rem.reason)
                {
                    mClient.consoleLog("[client] you were removed (peer disconnected).");
                }
                else if (DeleteReason::VOLUNTARY == rem.reason)
                {
                    mClient.consoleLog("[client] left the room.");
                }
                else
                {
                    mClient.consoleLog("[client] you were removed from the game.");
                }
                mClient.requestExitToLobby();
                return;
            }
        }
        if (!pMsg.playerToDelete.empty())
        {
            draw();
        }
    }

    void handle(BoardUpdateNotification&& pMsg)
    {
        if (!mGameStarted)
        {
            return;
        }

        auto foundIt = mPlayers.find(pMsg.player);
        if (mPlayers.end() != foundIt)
        {
            auto& player = foundIt->second;

            if (pMsg.hold)
            {
                player.hold.emplace((Termino)*pMsg.hold);
            }
            if (pMsg.attackIndicator)
            {
                mCurrentTarget.emplace(player.id);
                player.incoming.emplace(*pMsg.attackIndicator);
            }
            for (auto i : pMsg.pieceToAddList)
            {
                player.queue.emplace_back((Termino)i);
            }
            if (pMsg.placement)
            {
                player.initializeCurrentTermino(Termino(*pMsg.placement));
                if (player.queue.size() && !pMsg.hold)
                {
                    player.queue.pop_front();
                }
            }
            if (pMsg.rotation)
            {
                player.rotation = *pMsg.rotation;
            }
            if (auto& pos = pMsg.position)
            {
                player.x = pos->x; 
                player.y = pos->y; 
                int8_t ypos = player.y;
                while (true)
                {
                    auto res =(*player.checkerFn)(player.bitmap, player.x, ypos-1, player.transformer);
                    if (res)
                    {
                        break;
                    }
                    ypos--;
                }
                player.floor = ypos;
            }
            for (auto i : pMsg.linesToReplaceList)
            {
                player.bitmap.replaceLine(i.line, i.diff);
            }
            for (auto i : pMsg.linesToRemoveList)
            {
                player.bitmap.clearLine(i);
            }
            for (auto i : pMsg.linesToInsertList)
            {
                player.bitmap.insertLine(i.line, i.diff);
            }
        }
        draw();
    }

    void draw()
    {
        mClient.paintGameView();
    }

    void keyIn(char pKey)
    {
        if (mClient.tryHandleGameplayChat(pKey))
        {
            return;
        }

        if (3 == static_cast<unsigned char>(pKey))
        {
            mClient.sendLeaveIndication();
            mClient.requestExitToLobby();
            return;
        }

        mKeyPressHistory.emplace_front(pKey);
        if (4 == mKeyPressHistory.size())
        {
            mKeyPressHistory.pop_back();
        }
        draw();

        mLatencyMeasStart = std::chrono::high_resolution_clock::now();
        mLatencyMeasEnabled = true;

        if (27 == mKeyPressHistory[2] && 91 == mKeyPressHistory[1])
        {
            if (68 == pKey)
            {
                onEvent(board::Move{-1});
            }
            else if (67 == pKey)
            {
                onEvent(board::Move{1});
            }
            else if (66 == pKey)
            {
                onEvent(board::SoftDrop{});
            }
        }
        else if ('w' == pKey)
        {
            onEvent(board::Hold{});
        }
        else if ('a' == pKey)
        {
            onEvent(board::Rotate{3});
        }
        else if ('d' == pKey)
        {
            onEvent(board::Rotate{1});
        }
        else if ('s' == pKey)
        {
            onEvent(board::Rotate{2});
        }
        else if (' ' == pKey)
        {
            onEvent(board::Drop{});
        }
    }

    std::deque<char> mKeyPressHistory = std::deque<char>(3);

    std::chrono::high_resolution_clock::time_point mLatencyMeasStart;
    std::chrono::high_resolution_clock::time_point mLatencyMeasEnd;
    std::chrono::milliseconds mLatencyMeas{};
    bool mLatencyMeasEnabled = false;

    ITetrisClient& mClient;
    uint8_t mPlayerId; 
    JoinAccept mConfig;
    std::unordered_map<uint8_t, PlayerContext> mPlayers;

    std::optional<uint8_t> mCurrentTarget;
    std::vector<uint8_t> mGameOverSequence;

    bool mGameStarted = false;
};

} // namespace tetris

#endif // __PLAYER_HPP__