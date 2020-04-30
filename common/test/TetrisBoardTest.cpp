#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <common/TetrisBoard.hpp>

#include <iostream>

using namespace tetris;
using namespace testing;

struct TetrisBoardCallbacksMock : ITetrisBoardCallbacks
{
    Termino generate()
    {
        return Termino((generateCounter++)%7);
    }

    MOCK_METHOD1(clear, void(std::vector<uint8_t>));
    MOCK_METHOD1(piecePositionUpdate, void(CellCoord));

    int generateCounter = 0;
};

struct TetrisBoardTest : Test
{
    TetrisBoardCallbacksMock callbacks;
    TetrisBoard sut = TetrisBoard(TetrisBoardConfig{10,24}, &callbacks);
};

TEST_F(TetrisBoardTest, shouldMove)
{
    sut.restart();
}
// TODO: TEST_F(TetrisBoardTest, shouldLock)
// TODO: TEST_F(TetrisBoardTest, shouldSoftDrop)
// TODO: TEST_F(TetrisBoardTest, shouldHardDrop)
// TODO: TEST_F(TetrisBoardTest, shoudClearLine)
// TODO: TEST_F(TetrisBoardTest, shouldRotate)

