#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <common/Bitmap.hpp>

#include <iostream>

using namespace tetris;
using namespace testing;

struct BitmapTest : Test
{
    Bitmap sut = Bitmap(10,24);
};

TEST_F(BitmapTest, shouldSet)
{
    EXPECT_FALSE(sut.get(0, 0));
    sut.set(true, 0, 0);
    EXPECT_TRUE(sut.get(0, 0));
}

TEST_F(BitmapTest, shouldShift)
{
    EXPECT_FALSE(sut.get(0, 1));
    sut.set(true, 0, 0);
    sut.shiftUp(0x100);
    EXPECT_FALSE(sut.get(0, 0));
    EXPECT_TRUE(sut.get(1, 0));
    EXPECT_TRUE(sut.get(0, 1));
}

TEST_F(BitmapTest, shouldClearLine)
{
    sut.shiftUp(0x101);
    sut.shiftUp(0x102);
    sut.shiftUp(0x103);
    EXPECT_EQ(0x101u, sut.line(2));
    EXPECT_EQ(0x102u, sut.line(1));
    EXPECT_EQ(0x103u, sut.line(0));
    sut.clearLine(2);
    EXPECT_EQ(0u,     sut.line(2));
    EXPECT_EQ(0x102u, sut.line(1));
    EXPECT_EQ(0x103u, sut.line(0));
    sut.clearLine(0);
    EXPECT_EQ(0u,     sut.line(2));
    EXPECT_EQ(0u,     sut.line(1));
    EXPECT_EQ(0x102u, sut.line(0));
}
