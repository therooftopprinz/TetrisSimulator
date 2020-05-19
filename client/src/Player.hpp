#ifndef __PLAYER_HPP__
#define __PLAYER_HPP__

#include <interface/protocol.hpp>

#include <cstdio>
#include <deque>
#include <optional>
#include <unordered_map>

#include <common/Terminoes.hpp>
#include <common/StandardTetrisBoard.hpp>

#include <ITetrisClient.hpp>

namespace tetris
{

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

    TransformFn transformer;
    traits::CheckerFn* checkerFn = nullptr;
    traits::ApplierFn applier = nullptr;

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

        mClient.disableConsole();
        mClient.setKeyHandler([this](char key) -> void {
                keyIn(key);
            });
        draw();
    }

    Player() = delete;
    Player(const Player&&) = delete;

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
        mGameStarted = true;
    }

    void handle(GameEndNotification&& pMsg)
    {
        mGameStarted = false;
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
                if (player.queue.size())
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
        clearScreen();
        setCursor(0, mHeight-1);
        print("Tetris Simulator Client [%dx%d]", mConfig.boardWidth, mConfig.boardHeight);

        // main player
        auto& player = mPlayers.find(mPlayerId)->second;
        drawBoard(0,3, player);

        // current target
        if (mCurrentTarget)
        {
            auto foundIt = mPlayers.find(*mCurrentTarget);
            if (mPlayers.end() != foundIt)
            {
                auto& player = foundIt->second;
                drawBoard(30,3, player);
            }
        }

        setCursor(0, 1);
        print("key_input >> %3d %3d %3d", mKeyPressHistory[2], mKeyPressHistory[1], mKeyPressHistory[0]);

        setCursor(0, 0);
        print("command latency: %4dms", mLatencyMeas.count());

        printf("\x1b[2J");
        for (auto i=0u; i<mHeight; i++)
        {
            for (auto j=0u; j<mWidth; j++)
            {
                unsigned cursor = (mHeight-i-1)*mWidth + j;
                uint8_t pixel = mFrame[cursor] & 0xFF;
                pixel = pixel ? pixel : ' ';
                printf("%c", pixel);
            }
            printf("\n");
        }

        printf("\x1b[0;0f");
    }

    void drawBoard(uint8_t pX, uint8_t pY, PlayerContext& pPlayer)
    {
        // PLAYER BOARD
        setCursor(pX, pY);
        for (unsigned i = 0; i<mConfig.boardWidth+2u; i++) putchar('#');

        setCursor(pX, pY+mConfig.boardHeight+1u);
        for (unsigned i = 0; i<mConfig.boardWidth+2u; i++) putchar('#');

        for (unsigned i = 0; i<mConfig.boardHeight+1u; i++)
        {
            setCursor(pX, pY+i);
            putchar('#');
        }

        for (unsigned i = 0; i<mConfig.boardHeight+1u; i++)
        {
            setCursor(pX+mConfig.boardWidth+1, pY+i);
            putchar('#');
        }

        setCursor(pX, pY + mConfig.boardHeight + 3);
        print("id:%d name:\"%s\"", pPlayer.id, pPlayer.name.c_str());

        char hold= ' ';
        char current = ' ';
        std::string next;
        int incoming = 0;

        auto toChar = [](Termino pTerm) {
                static const char* map = "ILJOSZT";
                return map[(int)pTerm];
            };

        if (pPlayer.hold)
        {
            hold = toChar(*pPlayer.hold);
        }
        if (pPlayer.current)
        {
            hold = toChar(*pPlayer.current);
        }
        for (auto i : pPlayer.queue)
        {
            next.push_back(toChar(i));
        }
        if (pPlayer.current)
        {
            hold = toChar(*pPlayer.current);
        }
        if (pPlayer.incoming)
        {
            incoming = *pPlayer.incoming;
        } 

        setCursor(pX, pY + mConfig.boardHeight + 2);
        print("hold:%c current:%c", hold, current);
        print("next:%s incoming:%d", next.c_str(), incoming);

        auto bitmap = pPlayer.bitmap;
        for (auto i=0u; i<bitmap.dimension().second; i++)
        {
            for (auto j=0u; j<bitmap.dimension().first; j++)
            {
                auto x = pX+1+j;
                auto y = pY+1+i;
                if (!bitmap.get(j, i))
                {
                    continue;
                }

                setCursor(x, y);
                putchar('@');
            }
        }
        if (pPlayer.current)
        {
            auto x = pX + pPlayer.x + 1;
            auto y = pY + pPlayer.y + 1;

            pPlayer.applier(x, y, [this](CellCoord pCoord)
                {
                    setCursor(pCoord.first, pCoord.second);
                    putchar('@');
                }, pPlayer.transformer);

            auto ghosty = pY + pPlayer.floor + 1;

            pPlayer.applier(x, ghosty, [this](CellCoord pCoord)
                {
                    setCursor(pCoord.first, pCoord.second);
                    putchar('-');
                }, pPlayer.transformer);
        }
    }

    template <typename... T>
    void print(const char* pFmt, T&&... pArgs)
    {
        static char buff[512];
        auto len = sprintf(buff, pFmt, std::forward<T>(pArgs)...);
        for (int i=0; i<len; i++)
        {
            putchar(buff[i]);
        }
    }

    void putchar(uint8_t pChar)
    {
        mFrame[mCursor] &= 0xFFFFFF00;
        mFrame[mCursor] |= pChar;
        mCursor++;
    }

    void clearScreen()
    {
        std::memset(mFrame, 0, sizeof(mFrame));
    }

    void setCursor(uint8_t x, uint8_t y)
    {
        mCursor = y*mWidth + x;
    }

    void keyIn(char pKey)
    {
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

    unsigned mWidth = 80;
    unsigned mHeight = 40;

    uint32_t mFrame[80*40];
    size_t mCursor;

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

    bool mGameStarted = false;
};

} // namespace tetris

#endif // __PLAYER_HPP__