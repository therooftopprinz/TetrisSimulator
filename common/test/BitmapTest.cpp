#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <common/Bitmap.hpp>

using namespace tetris;
using namespace testing;

TEST(BitBuffer, shouldCreate)
{
    BitBuffer b(50);
    auto res = std::vector<uint8_t>(b.raw(), b.raw() + b.size());
    EXPECT_THAT(res, ElementsAre(0,0,0,0,0,0,0));
}