#ifndef __TETRIS_TERMINOES_HPP__
#define __TETRIS_TERMINOES_HPP__

namespace tetris
{

struct Move
{
    int8_t offset;
};

struct Rotate
{
    int8_t count;
};

struct Hold {};
struct Drop {};
struct SoftDrop {};
struct Lock {};

template<int8_t x, int8_t y>
struct TerminoCell
{
    static constexpr int8_t x = x;
    static constexpr int8_t y = y;
};

template <typename... Cells>
struct TerminoChecker
{
    template<typename Board>
    static bool check(const Board& board, int x, int y)
    {
        return false;
    }
}

template <typename Cell, typename... Cells>
struct TerminoChecker<Cell, Cells...>
{
    using CellCoord = std::pair<int8_t,int8_t>
    template<typename Board>
    static bool check(const Board& pBoard, int pX, int pY, bfc::LightFn<CellCoord(CellCoord)> pTransform)
    {
        px = x+Cell::x;
        py = y+Cell::y;

        CellCoord coord{pX, pY};

        if (pTransform)
        {
            coord = pTransform(CellCoord{pX, pY});
        }

        return board.get(coord.fist, coord.second) || TerminoChecker<Cells...>::check(board,x,y);
    }
}

TerminoChecker<
    TerminoCell<>
>


} // namespace tetris

#endif // __TETRIS_TERMINOES_HPP__
