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

template<uint8_t x, uint8_t y>
struct TerminoCell
{
    static constexpr uint8_t x = x;
    static constexpr uint8_t y = y;
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
    template<typename Board>
    static bool check(const Board& board, int x, int y)
    {
        return board.get(x+Cell::x, y+Cell::y) || TerminoChecker<Cells...>::check(board,x,y);
    }
}


} // namespace tetris

#endif // __TETRIS_TERMINOES_HPP__
