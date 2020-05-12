#ifndef __PLAYER_HPP__
#define __PLAYER_HPP__

#include <interface/protocol.hpp>

#include <common/Terminoes.hpp>

#include <ITetrisClient.hpp>

namespace tetris
{

struct PlayerContext
{
    Bitmap bitmap;
    std::vector<Termino> queue;
    std::optional<Termino> hold;
    uint8_t x;
    uint8_t y;
};

class Player
{
public:
    Player(uint8_t pPlayerId, ITetrisClient& pClient, const TetrisBoardConfig& pConfig)
        : mClient(pClient)
        , mPlayerId(pPlayerId)
        , mConfig(pConfig)
    {
        mClient.disableConsole();
        mClient.setKeyHandler([this](char key) -> void {
                keyIn(key);
            });
    }

    Player() = delete;
    Player(const Player&&) = delete;

    template<typename T>
    void onMsg(T& pMsg)
    {}

private:

    void draw()
    {
        std::cout << "\x1b[2J";
        setCursor(0, mHeight);
        std::cout << "Tetris Simulator Client";

        // PLAYER BOARD
        unsigned boardBaseX = 1;
        unsigned boardBaseY = mHeight+1;
        setCursor(boardBaseX, boardBaseY);
        for (unsigned i = 0; i<mConfig.width+2u; i++) std::cout << "#";
        setCursor(boardBaseX, boardBaseY+mConfig.height+2);
        for (unsigned i = 0; i<mConfig.width+2u; i++) std::cout << "#";
        for (unsigned i = 0; i<mConfig.height+2u; i++)
        {
            setCursor(boardBaseX, boardBaseY+i);
            std::cout << "#";
        }
        for (unsigned i = 0; i<mConfig.height+2u; i++)
        {
            setCursor(boardBaseX+mConfig.width, boardBaseY+i);
            std::cout << "#";
        }
    }

    void setCursor(uint8_t x, uint8_t y)
    {
        uint8_t ypos = mHeight - y;
        std::cout << "\x1b[" << ypos << ";" << x << "f";
    }

    void keyIn(char pKey)
    {
        if ('x' == pKey)
        {
            
        }
    }

    unsigned mWidth = 50;
    unsigned mHeight = 50;

    ITetrisClient& mClient;
    uint8_t mPlayerId;
    TetrisBoardConfig mConfig;
    std::unordered_map<uint8_t, PlayerContext> mPlayers;
};

} // namespace tetris

#endif // __PLAYER_HPP__