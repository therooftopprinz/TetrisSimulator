#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>

#include <ITetrisClient.hpp>
#include <Player.hpp>

using namespace testing;
using namespace tetris;

struct TetrisClientMock : ITetrisClient
{
    MOCK_METHOD1(send, void(TetrisProtocol&));
    MOCK_METHOD1(consoleLog, void(const std::string&));
    MOCK_METHOD0(enableConsole, void());
    MOCK_METHOD0(disableConsole, void());
    MOCK_METHOD1(setKeyHandler, void(bfc::LightFn<void(char)>));
};

struct PlayerTest : Test
{
    void SetUp()
    {
        EXPECT_CALL(mClientMock, setKeyHandler(_))
            .WillOnce(SaveArg<0>(&mPlayerKeyer));
        sut.emplace(0, mClientMock, mBoardConfig);
    }

    bfc::LightFn<void(char)> mPlayerKeyer;
    NiceMock<TetrisClientMock> mClientMock;
    TetrisBoardConfig mBoardConfig = {10,24};
    std::optional<tetris::Player> sut;
};

TEST_F(PlayerTest, Player)
{
    BoardUpdateNotification msg;
    msg.placement.emplace(Piece::I);
    msg.position.emplace(PiecePosition{3,20});
    msg.linesToInsertList.emplace_back(Line{0, 0x11});
    msg.linesToInsertList.emplace_back(Line{0, 0x22});
    msg.linesToInsertList.emplace_back(Line{0, 0x33});
    msg.linesToInsertList.emplace_back(Line{0, 0x3f});
    msg.linesToInsertList.emplace_back(Line{0, 0x1ff});
    msg.linesToInsertList.emplace_back(Line{0, 0x3ff});
    msg.player = 0;

    sut->onMsg(msg);

    msg = {};

    for (int i=0; i<15; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        int8_t y = 20-i;
        msg.position.emplace(PiecePosition{0,y});
        sut->onMsg(msg);
    }
}