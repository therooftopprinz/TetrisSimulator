#ifndef __PLAYER_HPP__
#define __PLAYER_HPP__

#include <interface/protocol.hpp>

#include <cstdio>
#include <deque>
#include <optional>
#include <unordered_map>

#include <common/Terminoes.hpp>
#include <common/TetrisBoard.hpp>

#include <ITetrisClient.hpp>

namespace tetris
{

struct PlayerContext
{
    PlayerContext(const TetrisBoardConfig& pConfig)
        : bitmap(pConfig.width, pConfig.height)
    {}

    Bitmap bitmap;
    std::vector<Termino> queue;
    std::optional<Termino> current;
    std::optional<Termino> hold;
    uint8_t rotation;
    int8_t x;
    int8_t y;

    TransformFn transformer;
    traits::ApplierFn applier = nullptr;

    void initializeCurrentTermino(Termino pTerm)
    {
        current.emplace(pTerm);
        rotation = 0;
        auto& termino = traits::gTerminoTraitsMap[*current];
        auto& rotator = std::get<3>(termino);
        applier = std::get<5>(termino);
        transformer = [this, &rotator](CellCoord pCoord) {
                return rotator(this->rotation, pCoord);
            };
    }
};

class Player
{
public:
    Player(uint8_t pPlayerId, ITetrisClient& pClient, const TetrisBoardConfig& pConfig)
        : mClient(pClient)
        , mPlayerId(pPlayerId)
        , mConfig(pConfig)
    {
        mPlayers.emplace(std::piecewise_construct, std::forward_as_tuple(pPlayerId), std::forward_as_tuple(mConfig));
        mClient.disableConsole();
        mClient.setKeyHandler([this](char key) -> void {
                keyIn(key);
            });
        draw();
    }

    Player() = delete;
    Player(const Player&&) = delete;

    template<typename T>
    void onMsg(T& pMsg)
    {
        mLatencyMeasEnd = std::chrono::high_resolution_clock::now();
        if (mLatencyMeasEnabled)
        {
            mLatencyMeas = std::chrono::duration_cast<std::chrono::milliseconds>(mLatencyMeasEnd-mLatencyMeasStart);
        }
        mLatencyMeasEnabled = false;

        handle(pMsg);
    }

private:
    template<typename T>
    void handle(T& pMsg)
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

    void handle(GameStartNotification& pMsg)
    {
        mGameStarted = true;
    }

    void handle(BoardUpdateNotification& pMsg)
    {
        if (!mGameStarted)
        {
            return;
        }

        auto foundIt = mPlayers.find(pMsg.player);
        if (mPlayers.end() != foundIt)
        {
            auto& player = foundIt->second;
            if (pMsg.placement)
            {
                player.initializeCurrentTermino(Termino(*pMsg.placement));
            }
            if (pMsg.rotation)
            {
                player.rotation = *pMsg.rotation;
            }
            if (auto& pos = pMsg.position)
            {
                player.x = pos->x; 
                player.y = pos->y; 
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
        print("Tetris Simulator Client [%dx%d]", mConfig.width, mConfig.height);

        // main player
        auto& player = mPlayers.find(mPlayerId)->second;
        drawBoard(0,3, player);

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
        for (unsigned i = 0; i<mConfig.width+2u; i++) putchar('#');

        setCursor(pX, pY+mConfig.height+1u);
        for (unsigned i = 0; i<mConfig.width+2u; i++) putchar('#');

        for (unsigned i = 0; i<mConfig.height+1u; i++)
        {
            setCursor(pX, pY+i);
            putchar('#');
        }

        for (unsigned i = 0; i<mConfig.height+1u; i++)
        {
            setCursor(pX+mConfig.width+1, pY+i);
            putchar('#');
        }

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

    unsigned mWidth = 50;
    unsigned mHeight = 30;

    uint32_t mFrame[50*30];
    size_t mCursor;

    std::deque<char> mKeyPressHistory = std::deque<char>(3);

    std::chrono::high_resolution_clock::time_point mLatencyMeasStart;
    std::chrono::high_resolution_clock::time_point mLatencyMeasEnd;
    std::chrono::milliseconds mLatencyMeas{};
    bool mLatencyMeasEnabled = false;

    ITetrisClient& mClient;
    uint8_t mPlayerId; 
    TetrisBoardConfig mConfig;
    std::unordered_map<uint8_t, PlayerContext> mPlayers;

    bool mGameStarted = false;
};

} // namespace tetris

#endif // __PLAYER_HPP__