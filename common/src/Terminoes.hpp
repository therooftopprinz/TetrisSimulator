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
using ApplyFn = bfc::LightFn<void(CellCoord)>;

template <typename... Cells>
struct TerminoOperator
{
    template<typename Bitmap>
    static size_t check(const Bitmap&, int8_t, int8_t, const TransformFn&)
    {
        return 0;
    }
    static bool set(Bitmap&, int8_t, int8_t, const TransformFn&)
    {
        return true;
    }
    static void apply(int8_t, int8_t, const ApplyFn&, const TransformFn&)
    {
    }
};

template <typename Cell, typename... Cells>
struct TerminoOperator<Cell, Cells...>
{
    template<typename Bitmap>
    static size_t check(const Bitmap& pBitmap, int8_t pX, int8_t pY, const TransformFn& pTransform = [](CellCoord p){return p;})
    {
        auto coord = pTransform(CellCoord{Cell::x, Cell::y});
        coord.first += pX;
        coord.second += pY;
        auto res = pBitmap.get(coord.first, coord.second);
        return res + TerminoOperator<Cells...>::check(pBitmap, pX, pY, pTransform);
    }
    template<typename Bitmap>
    static bool set(Bitmap& pBitmap, int8_t pX, int8_t pY, const TransformFn& pTransform = [](CellCoord p){return p;})
    {
        auto coord = pTransform(CellCoord{Cell::x, Cell::y});
        coord.first += pX;
        coord.second += pY;
        auto res = pBitmap.set(true, coord.first, coord.second);
        return res && TerminoOperator<Cells...>::set(pBitmap, pX, pY, pTransform);
    }

    static void apply(int8_t pX, int8_t pY, const ApplyFn& pApplyer , const TransformFn& pTransform = [](CellCoord p){return p;})
    {
        auto coord = pTransform(CellCoord{Cell::x, Cell::y});
        coord.first += pX;
        coord.second += pY;
        pApplyer(coord);
        TerminoOperator<Cells...>::apply(pX, pY, pApplyer, pTransform);
    }
};

template <typename T>
struct TerminoTraits;

template <typename T>
struct TerminoRotator
{
    static CellCoord rotate(uint8_t pCount, CellCoord pCoord)
    {
        for (uint8_t i=0; i < pCount; i++)
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

using TerminoI = TerminoOperator<
    TerminoCell<0,2>,
    TerminoCell<1,2>,
    TerminoCell<2,2>,
    TerminoCell<3,2>>;

using TerminoL = TerminoOperator<
    TerminoCell<0,1>,
    TerminoCell<1,1>,
    TerminoCell<2,1>,
    TerminoCell<2,2>>;

using TerminoJ = TerminoOperator<
    TerminoCell<0,2>,
    TerminoCell<0,1>,
    TerminoCell<1,1>,
    TerminoCell<2,1>>;

using TerminoO = TerminoOperator<
    TerminoCell<1,2>,
    TerminoCell<1,1>,
    TerminoCell<2,1>,
    TerminoCell<2,2>>;

using TerminoS = TerminoOperator<
    TerminoCell<0,1>,
    TerminoCell<1,1>,
    TerminoCell<1,2>,
    TerminoCell<2,2>>;

using TerminoZ = TerminoOperator<
    TerminoCell<0,2>,
    TerminoCell<1,2>,
    TerminoCell<1,1>,
    TerminoCell<2,1>>;

using TerminoT = TerminoOperator<
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

enum Termino {I, L, J, O, S, Z, T, NONE};

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
    using SetterFn = bool(*)(Bitmap&, int8_t, int8_t, const bfc::LightFn<CellCoord(CellCoord)>&);
    using ApplierFn = void(*)(int8_t, int8_t, const ApplyFn&, const TransformFn&);
    using TraitsTuple = std::tuple<uint8_t, uint8_t, CheckerFn, RotatorFn, SetterFn, ApplierFn>;


    inline std::unordered_map<Termino, TraitsTuple> gTerminoTraitsMap = {
        {I, std::make_tuple(TerminoI::width, TerminoI::height, &tetris::TerminoI::check<Bitmap>, &TerminoRotator<tetris::TerminoI>::rotate, &tetris::TerminoI::set<Bitmap>, &tetris::TerminoI::apply)},
        {L, std::make_tuple(TerminoL::width, TerminoL::height, &tetris::TerminoJ::check<Bitmap>, &TerminoRotator<tetris::TerminoJ>::rotate, &tetris::TerminoJ::set<Bitmap>, &tetris::TerminoJ::apply)},
        {J, std::make_tuple(TerminoJ::width, TerminoJ::height, &tetris::TerminoL::check<Bitmap>, &TerminoRotator<tetris::TerminoL>::rotate, &tetris::TerminoL::set<Bitmap>, &tetris::TerminoL::apply)},
        {O, std::make_tuple(TerminoO::width, TerminoO::height, &tetris::TerminoO::check<Bitmap>, &TerminoRotator<tetris::TerminoO>::rotate, &tetris::TerminoO::set<Bitmap>, &tetris::TerminoO::apply)},
        {S, std::make_tuple(TerminoS::width, TerminoS::height, &tetris::TerminoS::check<Bitmap>, &TerminoRotator<tetris::TerminoS>::rotate, &tetris::TerminoS::set<Bitmap>, &tetris::TerminoS::apply)},
        {Z, std::make_tuple(TerminoZ::width, TerminoZ::height, &tetris::TerminoZ::check<Bitmap>, &TerminoRotator<tetris::TerminoZ>::rotate, &tetris::TerminoZ::set<Bitmap>, &tetris::TerminoZ::apply)},
        {T, std::make_tuple(TerminoT::width, TerminoT::height, &tetris::TerminoT::check<Bitmap>, &TerminoRotator<tetris::TerminoT>::rotate, &tetris::TerminoT::set<Bitmap>, &tetris::TerminoT::apply)}
    };

    struct PairHasher { 
        template <class T1, class T2> 
        size_t operator()(const std::pair<T1, T2>& p) const
        { 
            auto hash1 = std::hash<T1>{}(p.first); 
            auto hash2 = std::hash<T2>{}(p.second); 
            return hash1 ^ hash2; 
        } 
    };

    using TerminoRotation = std::pair<Termino, uint8_t>;
    using _TR = TerminoRotation;
    using _RD = std::pair<std::vector<CellCoord>, std::vector<CellCoord>>;

    inline _RD commonRot0 = _RD{{{ 0, 0}, { 1, 0}, { 1, 1}, { 0,-2}, { 1,-2}}, {{ 0, 0}, {-1, 0}, {-1, 1}, { 0,-2}, {-1,-2}}};
    inline _RD commonRot1 = _RD{{{ 0, 0}, { 1, 0}, { 1,-1}, { 0, 2}, { 1, 2}}, {{ 0, 0}, { 1, 0}, { 1,-1}, { 0, 2}, { 1, 2}}};
    inline _RD commonRot2 = _RD{{{ 0, 0}, {-1, 0}, {-1, 1}, { 0,-2}, {-1,-2}}, {{ 0, 0}, { 1, 0}, { 1, 1}, { 0,-2}, { 1,-2}}};
    inline _RD commonRot3 = _RD{{{ 0, 0}, {-1, 0}, {-1,-1}, { 0, 2}, {-1, 2}}, {{ 0, 0}, {-1, 0}, {-1,-1}, { 0, 2}, {-1, 2}}};

    inline _RD terminoIRot0 = _RD{{{ 0, 0}, {-1, 0}, { 2, 0}, {-1, 2}, { 2,-1}}, {{ 0, 0}, {-2, 0}, { 1, 0}, {-2,-1}, { 1, 2}}};
    inline _RD terminoIRot1 = _RD{{{ 0, 0}, { 2, 0}, {-1, 0}, { 2, 1}, {-1,-2}}, {{ 0, 0}, {-1, 0}, { 2, 0}, {-1, 2}, { 2,-1}}};
    inline _RD terminoIRot2 = _RD{{{ 0, 0}, { 1, 0}, {-2, 0}, { 1,-2}, {-2, 1}}, {{ 0, 0}, { 2, 0}, {-1, 0}, { 2, 1}, {-1,-2}}};
    inline _RD terminoIRot3 = _RD{{{ 0, 0}, {-2, 0}, { 1, 0}, {-2,-1}, { 1, 2}}, {{ 0, 0}, { 1, 0}, {-2, 0}, { 1,-2}, {-2, 1}}};


    inline std::unordered_map<TerminoRotation, _RD, PairHasher> gSrsWallKicks = {
        {_TR(L, 0), commonRot0},
        {_TR(J, 0), commonRot0},
        {_TR(O, 0), commonRot0},
        {_TR(S, 0), commonRot0},
        {_TR(Z, 0), commonRot0},
        {_TR(T, 0), commonRot0},

        {_TR(L, 1), commonRot1},
        {_TR(J, 1), commonRot1},
        {_TR(O, 1), commonRot1},
        {_TR(S, 1), commonRot1},
        {_TR(Z, 1), commonRot1},
        {_TR(T, 1), commonRot1},

        {_TR(L, 2), commonRot2},
        {_TR(J, 2), commonRot2},
        {_TR(O, 2), commonRot2},
        {_TR(S, 2), commonRot2},
        {_TR(Z, 2), commonRot2},
        {_TR(T, 2), commonRot2},

        {_TR(L, 3), commonRot3},
        {_TR(J, 3), commonRot3},
        {_TR(O, 3), commonRot3},
        {_TR(S, 3), commonRot3},
        {_TR(Z, 3), commonRot3},
        {_TR(T, 3), commonRot3},

        {_TR(I, 0), terminoIRot0},
        {_TR(I, 1), terminoIRot1},
        {_TR(I, 2), terminoIRot2},
        {_TR(I, 3), terminoIRot3},
    };

} // namespace traits

} // namespace tetris

#endif // __TETRIS_TERMINOES_HPP__
