#ifndef __TETRIS_TETRISBOARD_HPP__
#define __TETRIS_TETRISBOARD_HPP__

namespace tetris
{

enum class Piece {S, Z, L, J, O, I};

class TetrisBoard
{
public:
    TetrisBoard(uint8_t width, uint8_t height, bfc::LightFunctionObject<Piece()> generator)
    
    void onEvent(const Move&);
    void onEvent(const Rotate&);
    void onEvent(const Hold&);
    void onEvent(const Drop&);
    void onEvent(const SoftDrop&);
    Bitmap& get();
    const Patch& diff();
    void apply(const Patch&);
    enum class State {OK, GAME_OVER};
    State status();
private:
    bfc::Buffer 
};


} // namespace tetris

#endif // __TETRIS_TETRISBOARD_HPP__