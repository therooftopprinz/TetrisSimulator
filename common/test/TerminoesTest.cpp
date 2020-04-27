#include <iostream>
#include <variant>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <common/Bitmap.hpp>
#include <common/Terminoes.hpp>

using namespace tetris;
using namespace testing;

struct TerminoRotatorTest : TestWithParam<std::tuple<uint8_t, CellCoord, CellCoord>> {};

TEST_P(TerminoRotatorTest, 4By4)
{
    auto rot = std::get<0>(GetParam());
    auto src = std::get<1>(GetParam());
    auto dstExpected = std::get<2>(GetParam());

    TerminoRotator<Rot4x4> rotate{rot};
    auto dstActual = rotate(src);
    EXPECT_EQ(dstExpected, dstActual);
}

auto TerminoRotatorTestValVal = std::vector<std::tuple<uint8_t, CellCoord, CellCoord>>
{
    std::make_tuple(1, CellCoord{0,3}, CellCoord{3,3}),
    std::make_tuple(1, CellCoord{3,3}, CellCoord{3,0}),
    std::make_tuple(1, CellCoord{3,0}, CellCoord{0,0}),
    std::make_tuple(1, CellCoord{0,0}, CellCoord{0,3}),

    std::make_tuple(2, CellCoord{0,3}, CellCoord{3,0}),
    std::make_tuple(2, CellCoord{3,3}, CellCoord{0,0}),
    std::make_tuple(2, CellCoord{3,0}, CellCoord{0,3}),
    std::make_tuple(2, CellCoord{0,0}, CellCoord{3,3}),

    std::make_tuple(1, CellCoord{1,2}, CellCoord{2,2}),
    std::make_tuple(1, CellCoord{2,2}, CellCoord{2,1}),
    std::make_tuple(1, CellCoord{2,1}, CellCoord{1,1}),
    std::make_tuple(1, CellCoord{1,1}, CellCoord{1,2})
};

INSTANTIATE_TEST_CASE_P(TerminoRotatorTester, TerminoRotatorTest, ValuesIn(TerminoRotatorTestValVal));

using Terminoes = std::variant<TerminoI, TerminoJ, TerminoL, TerminoO, TerminoS, TerminoZ, TerminoT>;
using TerminoesTestParam = std::tuple<
    Terminoes,
    uint8_t,
    std::vector<uint64_t>>;

struct TerminoesTest : TestWithParam<TerminoesTestParam>
{};

TEST_P(TerminoesTest, check)
{
    auto& blocks = std::get<2>(GetParam());
    auto dim = blocks.size();
    Bitmap bitmap(dim, dim);
    Bitmap bitmapInv(dim, dim);

    for (auto i : blocks)
    {
        bitmap.shiftUp(i);
        bitmapInv.shiftUp(~i);
    }

    auto tester = [&bitmap, &bitmapInv, this](auto t)
    {
        using Termino = decltype(t);
        TerminoRotator<Termino> rot{std::get<1>(GetParam())};
        auto res = Termino::check(bitmap, 0, 0, rot);
        auto res2 = Termino::check(bitmapInv, 0, 0, rot);
        EXPECT_EQ(4u, res);
        EXPECT_EQ(0u, res2);
    };

    std::visit(tester, std::get<0>(GetParam()));
}

auto TerminoesTesterVal = std::vector<TerminoesTestParam>{
    std::make_tuple(Terminoes(TerminoI()), 0u, std::vector<uint64_t>{
        0b0000,
        0b1111,
        0b0000,
        0b0000
    }),
    std::make_tuple(Terminoes(TerminoI()), 1u, std::vector<uint64_t>{
        0b0010,
        0b0010,
        0b0010,
        0b0010
    }),
    std::make_tuple(Terminoes(TerminoI()), 2u, std::vector<uint64_t>{
        0b0000,
        0b0000,
        0b1111,
        0b0000
    }),
    std::make_tuple(Terminoes(TerminoI()), 3u, std::vector<uint64_t>{
        0b0100,
        0b0100,
        0b0100,
        0b0100
    }),
    std::make_tuple(Terminoes(TerminoJ()), 0u, std::vector<uint64_t>{
        0b100,
        0b111,
        0b000
    }),
    std::make_tuple(Terminoes(TerminoJ()), 1u, std::vector<uint64_t>{
        0b011,
        0b010,
        0b010
    }),
    std::make_tuple(Terminoes(TerminoJ()), 2u, std::vector<uint64_t>{
        0b000,
        0b111,
        0b001
    }),
    std::make_tuple(Terminoes(TerminoJ()), 3u, std::vector<uint64_t>{
        0b010,
        0b010,
        0b110
    }),
    std::make_tuple(Terminoes(TerminoL()), 0u, std::vector<uint64_t>{
        0b001,
        0b111,
        0b000
    }),
    std::make_tuple(Terminoes(TerminoL()), 1u, std::vector<uint64_t>{
        0b010,
        0b010,
        0b011
    }),
    std::make_tuple(Terminoes(TerminoL()), 2u, std::vector<uint64_t>{
        0b000,
        0b111,
        0b100
    }),
    std::make_tuple(Terminoes(TerminoL()), 3u, std::vector<uint64_t>{
        0b110,
        0b010,
        0b010
    }),
    std::make_tuple(Terminoes(TerminoO()), 0u, std::vector<uint64_t>{
        0b0000,
        0b0110,
        0b0110,
        0b0000
    }),
    std::make_tuple(Terminoes(TerminoO()), 1u, std::vector<uint64_t>{
        0b0000,
        0b0110,
        0b0110,
        0b0000
    }),
    std::make_tuple(Terminoes(TerminoO()), 2u, std::vector<uint64_t>{
        0b0000,
        0b0110,
        0b0110,
        0b0000
    }),
    std::make_tuple(Terminoes(TerminoO()), 3u, std::vector<uint64_t>{
        0b0000,
        0b0110,
        0b0110,
        0b0000
    }),
    std::make_tuple(Terminoes(TerminoS()), 0u, std::vector<uint64_t>{
        0b011,
        0b110,
        0b000
    }),
    std::make_tuple(Terminoes(TerminoS()), 1u, std::vector<uint64_t>{
        0b010,
        0b011,
        0b001
    }),
    std::make_tuple(Terminoes(TerminoS()), 2u, std::vector<uint64_t>{
        0b000,
        0b011,
        0b110
    }),
    std::make_tuple(Terminoes(TerminoS()), 3u, std::vector<uint64_t>{
        0b100,
        0b110,
        0b010
    }),
    std::make_tuple(Terminoes(TerminoZ()), 0u, std::vector<uint64_t>{
        0b110,
        0b011,
        0b000
    }),
    std::make_tuple(Terminoes(TerminoZ()), 1u, std::vector<uint64_t>{
        0b001,
        0b011,
        0b010
    }),
    std::make_tuple(Terminoes(TerminoZ()), 2u, std::vector<uint64_t>{
        0b000,
        0b110,
        0b011
    }),
    std::make_tuple(Terminoes(TerminoZ()), 3u, std::vector<uint64_t>{
        0b010,
        0b110,
        0b100
    }),
    std::make_tuple(Terminoes(TerminoT()), 0u, std::vector<uint64_t>{
        0b010,
        0b111,
        0b000
    }),
    std::make_tuple(Terminoes(TerminoT()), 1u, std::vector<uint64_t>{
        0b010,
        0b011,
        0b010
    }),
    std::make_tuple(Terminoes(TerminoT()), 2u, std::vector<uint64_t>{
        0b000,
        0b111,
        0b010
    }),
    std::make_tuple(Terminoes(TerminoT()), 3u, std::vector<uint64_t>{
        0b010,
        0b110,
        0b010
    })
};

INSTANTIATE_TEST_CASE_P(TerminoesTester, TerminoesTest, ValuesIn(TerminoesTesterVal));
