#ifndef __TETRIS_TERMINOES_HPP__
#define __TETRIS_TERMINOES_HPP__

#include <cstdint>
#include <utility>
#include <bfc/FixedFunctionObject.hpp>

#include <common/Bitmap.hpp>

namespace tetris
{ 

template<unsigned X, unsigned Y>
struct TerminoCell
{
    static constexpr uint8_t x = X;
    static constexpr uint8_t y = Y;
};

using CellCoord = std::pair<int8_t,int8_t>;
using TransformFn = bfc::LightFn<CellCoord(CellCoord)>;

template <typename... Cells>
struct TerminoChecker
{
    template<typename Board>
    static size_t check(const Board& pBoard, int8_t pX, int8_t pY, const bfc::LightFn<CellCoord(CellCoord)>&)
    {
        return 0;
    }
};

template <typename Cell, typename... Cells>
struct TerminoChecker<Cell, Cells...>
{
    template<typename Board>
    static size_t check(const Board& pBoard, int8_t pX, int8_t pY, const TransformFn& pTransform = [](CellCoord p){return p;})
    {
        auto coord = pTransform(CellCoord{pX + Cell::x, pY + Cell::y});
        auto res = pBoard.get(coord.first, coord.second);
        return res + TerminoChecker<Cells...>::check(pBoard, pX, pY, pTransform);
    }
};

template <typename T>
struct TerminoTraits;

template <typename T>
struct TerminoRotator
{
    static CellCoord rotate(uint8_t pRotation, CellCoord pCoord)
    {
        for (uint8_t i=0; i < pRotation; i++)
        {
            auto tmp = pCoord;
            tmp.first = pCoord.second; 
            auto adj = TerminoTraits<T>::rotation_adjust;
            tmp.second = -(pCoord.first + adj);
            pCoord = tmp;
        }
        return pCoord;
    }
};

using TerminoI = TerminoChecker<
    TerminoCell<0,2>,
    TerminoCell<1,2>,
    TerminoCell<2,2>,
    TerminoCell<3,2>>;

using TerminoL = TerminoChecker<
    TerminoCell<0,1>,
    TerminoCell<1,1>,
    TerminoCell<2,1>,
    TerminoCell<2,2>>;

using TerminoJ = TerminoChecker<
    TerminoCell<0,2>,
    TerminoCell<0,1>,
    TerminoCell<1,1>,
    TerminoCell<2,1>>;

using TerminoO = TerminoChecker<
    TerminoCell<1,2>,
    TerminoCell<1,1>,
    TerminoCell<2,1>,
    TerminoCell<2,2>>;

using TerminoS = TerminoChecker<
    TerminoCell<0,1>,
    TerminoCell<1,1>,
    TerminoCell<1,2>,
    TerminoCell<2,2>>;

using TerminoZ = TerminoChecker<
    TerminoCell<0,2>,
    TerminoCell<1,2>,
    TerminoCell<1,1>,
    TerminoCell<2,1>>;

using TerminoT = TerminoChecker<
    TerminoCell<0,1>,
    TerminoCell<1,2>,
    TerminoCell<1,1>,
    TerminoCell<2,1>>;

struct Block4x4
{
    static constexpr int8_t rotation_adjust = -3;
    static constexpr uint8_t width  = 4;
    static constexpr uint8_t height = 4;
};

struct Block3x3
{
    static constexpr int8_t rotation_adjust = -2;
    static constexpr uint8_t width  = 3;
    static constexpr uint8_t height = 3;
};

template <> struct TerminoTraits<TerminoI> : Block4x4{};
template <> struct TerminoTraits<TerminoL> : Block3x3{};
template <> struct TerminoTraits<TerminoJ> : Block3x3{};
template <> struct TerminoTraits<TerminoO> : Block4x4{};
template <> struct TerminoTraits<TerminoS> : Block3x3{};
template <> struct TerminoTraits<TerminoZ> : Block3x3{};
template <> struct TerminoTraits<TerminoT> : Block3x3{};
template <> struct TerminoTraits<Block4x4> : Block4x4{};
template <> struct TerminoTraits<Block3x3> : Block3x3{};

using TerminoTraitsI = TerminoTraits<TerminoI>;
using TerminoTraitsL = TerminoTraits<TerminoL>;
using TerminoTraitsJ = TerminoTraits<TerminoJ>;
using TerminoTraitsO = TerminoTraits<TerminoO>;
using TerminoTraitsS = TerminoTraits<TerminoS>;
using TerminoTraitsZ = TerminoTraits<TerminoZ>;
using TerminoTraitsT = TerminoTraits<TerminoT>;

enum Termino {I, L, J, O, S, Z, T};

namespace traits
{
    using TerminoI = TerminoTraits<TerminoI>;
    using TerminoL = TerminoTraits<TerminoL>;
    using TerminoJ = TerminoTraits<TerminoJ>;
    using TerminoO = TerminoTraits<TerminoO>;
    using TerminoS = TerminoTraits<TerminoS>;
    using TerminoZ = TerminoTraits<TerminoZ>;
    using TerminoT = TerminoTraits<TerminoT>;

    enum {WIDTH, HEIGHT};

    using CheckerFn = size_t (*)(const Bitmap&, int8_t, int8_t, const bfc::LightFn<CellCoord(CellCoord)>&);
    using RotatorFn = CellCoord (*)(uint8_t, CellCoord);
    using TraitsTuple = std::tuple<uint8_t, uint8_t, CheckerFn, RotatorFn>;

    inline std::unordered_map<Termino, TraitsTuple> gTerminoTraitsMap = {
        {I, std::make_tuple(TerminoI::width, TerminoI::height, &tetris::TerminoI::check<Bitmap>, &TerminoRotator<tetris::TerminoI>::rotate)},
        {L, std::make_tuple(TerminoL::width, TerminoL::height, &tetris::TerminoJ::check<Bitmap>, &TerminoRotator<tetris::TerminoJ>::rotate)},
        {J, std::make_tuple(TerminoJ::width, TerminoJ::height, &tetris::TerminoL::check<Bitmap>, &TerminoRotator<tetris::TerminoL>::rotate)},
        {O, std::make_tuple(TerminoO::width, TerminoO::height, &tetris::TerminoO::check<Bitmap>, &TerminoRotator<tetris::TerminoO>::rotate)},
        {S, std::make_tuple(TerminoS::width, TerminoS::height, &tetris::TerminoS::check<Bitmap>, &TerminoRotator<tetris::TerminoS>::rotate)},
        {Z, std::make_tuple(TerminoZ::width, TerminoZ::height, &tetris::TerminoZ::check<Bitmap>, &TerminoRotator<tetris::TerminoZ>::rotate)},
        {T, std::make_tuple(TerminoT::width, TerminoT::height, &tetris::TerminoT::check<Bitmap>, &TerminoRotator<tetris::TerminoT>::rotate)}
    };
} // namespace traits

} // namespace tetris

#endif // __TETRIS_TERMINOES_HPP__
