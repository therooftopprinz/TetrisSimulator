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
        if (autoGeneratePieceEnabled)
        {
            return Termino((generateCounter++)%7);
        }
        return generate_();
    }

    MOCK_METHOD0(generate_, Termino());
    MOCK_METHOD1(clear, void(std::vector<uint8_t>));
    MOCK_METHOD1(piecePosition, void(CellCoord));
    MOCK_METHOD1(newPiece, void(Termino));
    MOCK_METHOD0(hold, void());

    bool autoGeneratePieceEnabled = false;
    int generateCounter = 0;
};

struct TetrisBoardTest : Test
{
    TetrisBoardCallbacksMock callbacks;
    TetrisBoard sut = TetrisBoard(TetrisBoardConfig{10,24}, &callbacks);
};

TEST_F(TetrisBoardTest, shouldMoveClear)
{
    EXPECT_CALL(callbacks, newPiece(Termino::I));
    EXPECT_CALL(callbacks, piecePosition(CellCoord{3, 20})); // Initial piece placement

    EXPECT_CALL(callbacks, piecePosition(CellCoord{2, 20})); // Move -1
    EXPECT_CALL(callbacks, piecePosition(CellCoord{0, 20})); // Move -5
    EXPECT_CALL(callbacks, piecePosition(CellCoord{5, 20})); // Move 5
    EXPECT_CALL(callbacks, piecePosition(CellCoord{6, 20})); // Move 10

    callbacks.autoGeneratePieceEnabled = true;
    sut.restart();

    sut.onEvent(board::Move{-1});
    sut.onEvent(board::Move{-5});
    sut.onEvent(board::Move{5});
    sut.onEvent(board::Move{10});
}

TEST_F(TetrisBoardTest, shouldMoveRestricted)
{
    EXPECT_CALL(callbacks, newPiece(Termino::I));
    EXPECT_CALL(callbacks, piecePosition(CellCoord{3, 20})); // Initial piece placement
    EXPECT_CALL(callbacks, piecePosition(CellCoord{2, 20})); // Move -10
    EXPECT_CALL(callbacks, piecePosition(CellCoord{4, 20})); // Move 10

    callbacks.autoGeneratePieceEnabled = true;
    sut.restart();

    auto& bitmap = sut.bitmap();
    bitmap.replaceLine(22, 0b1100000011);

    sut.onEvent(board::Move{-10});
    sut.onEvent(board::Move{10});
}

TEST_F(TetrisBoardTest, shouldMoveExtended)
{
    EXPECT_CALL(callbacks, newPiece(Termino::I));
    EXPECT_CALL(callbacks, piecePosition(CellCoord{3, 20})); // Initial piece placement
    EXPECT_CALL(callbacks, piecePosition(CellCoord{-2, 20})); // Move -10
    EXPECT_CALL(callbacks, piecePosition(CellCoord{7, 20})); // Move 10

    callbacks.autoGeneratePieceEnabled = true;
    sut.restart();

    sut.onEvent(board::Rotate{1});
    sut.onEvent(board::Move{-10});
    sut.onEvent(board::Move{10});
}

TEST_F(TetrisBoardTest, shouldLockDrop)
{
    EXPECT_CALL(callbacks, newPiece(Termino::I));
    EXPECT_CALL(callbacks, piecePosition(CellCoord{3, 20})); // Initial piece placement
    EXPECT_CALL(callbacks, piecePosition(CellCoord{3, 19}));

    callbacks.autoGeneratePieceEnabled = true;
    sut.restart();

    sut.onEvent(board::Lock{});
}

TEST_F(TetrisBoardTest, shouldSoftDrop)
{
    EXPECT_CALL(callbacks, newPiece(Termino::I));
    EXPECT_CALL(callbacks, piecePosition(CellCoord{3, 20})); // Initial piece placement
    EXPECT_CALL(callbacks, piecePosition(CellCoord{3, -2}));

    callbacks.autoGeneratePieceEnabled = true;
    sut.restart();

    sut.onEvent(board::SoftDrop{});
}

TEST_F(TetrisBoardTest, shouldLockSet)
{
    InSequence seq;
    EXPECT_CALL(callbacks, newPiece(Termino::I));
    EXPECT_CALL(callbacks, piecePosition(CellCoord{3, 20})); // Initial piece placement
    EXPECT_CALL(callbacks, piecePosition(CellCoord{3, -2}));
    EXPECT_CALL(callbacks, newPiece(Termino::L));
    EXPECT_CALL(callbacks, piecePosition(CellCoord{3, 21})); // Second piece placement
    EXPECT_CALL(callbacks, piecePosition(CellCoord{3, 0}));
    EXPECT_CALL(callbacks, newPiece(Termino::J));
    EXPECT_CALL(callbacks, piecePosition(CellCoord{3, 21})); // Third piece placement

    callbacks.autoGeneratePieceEnabled = true;
    sut.restart();

    sut.onEvent(board::SoftDrop{});
    sut.onEvent(board::Lock{});
    sut.onEvent(board::SoftDrop{});
    sut.onEvent(board::Lock{});

    auto& bitmap = sut.bitmap();

    EXPECT_EQ(0b001000000u, bitmap.line(2));
    EXPECT_EQ(0b001110000u, bitmap.line(1));
    EXPECT_EQ(0b001111000u, bitmap.line(0));
}


// TODO: TEST_F(TetrisBoardTest, shouldHardDrop)
// TODO: TEST_F(TetrisBoardTest, shoudClearLine)
// TODO: TEST_F(TetrisBoardTest, shouldRotate)

