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
