#ifndef __TETRIS_TERMINOES_HPP__
#define __TETRIS_TERMINOES_HPP__

#include <cstdint>
#include <utility>
#include <bfc/FixedFunctionObject.hpp>

namespace tetris
{ 

template<unsigned X, unsigned Y>
struct TerminoCell
{
    static constexpr uint8_t x = X;
    static constexpr uint8_t y = Y;
};

 using CellCoord = std::pair<int8_t,int8_t>;

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
    static size_t check(const Board& pBoard, int8_t pX, int8_t pY, const bfc::LightFn<CellCoord(CellCoord)>& pTransform = [](CellCoord p){return p;})
    {
        auto coord = pTransform(CellCoord{pX + Cell::x, pY + Cell::y});
        auto res = pBoard.get(coord.first, coord.second);
        return res + TerminoChecker<Cells...>::check(pBoard, pX, pY, pTransform);
    }
};

template <typename T>
struct RotationTrait;

template <typename T>
struct TerminoRotator
{
    CellCoord operator()(CellCoord pCoord) const
    {
        for (uint8_t i=0; i<rotation; i++)
        {
            auto tmp = pCoord;
            tmp.first = pCoord.second; 
            auto adj = RotationTrait<T>::adjust;
            tmp.second = -(pCoord.first + adj);
            pCoord = tmp;
        }
        return pCoord;
    }
    uint8_t rotation;
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

struct Rot4x4
{
    static constexpr int8_t adjust = -3;
};

struct Rot3x3
{
    static constexpr int8_t adjust = -2;
};

template <> struct RotationTrait<TerminoI> : Rot4x4{};
template <> struct RotationTrait<TerminoL> : Rot3x3{};
template <> struct RotationTrait<TerminoJ> : Rot3x3{};
template <> struct RotationTrait<TerminoO> : Rot4x4{};
template <> struct RotationTrait<TerminoS> : Rot3x3{};
template <> struct RotationTrait<TerminoZ> : Rot3x3{};
template <> struct RotationTrait<TerminoT> : Rot3x3{};
template <> struct RotationTrait<Rot4x4> : Rot4x4{};
template <> struct RotationTrait<Rot3x3> : Rot3x3{};

} // namespace tetris

#endif // __TETRIS_TERMINOES_HPP__
