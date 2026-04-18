#include <common/Bitmap.hpp>
#include <common/TetrisBoardEvents.hpp>
#include <common/Terminoes.hpp>

namespace tetris
{

struct ITetrisBoard
{
    virtual void onEvent(const board::Move& pEvent) = 0;
    virtual void onEvent(const board::Rotate& pEvent) = 0;
    virtual void onEvent(const board::Hold&) = 0;
    virtual void onEvent(const board::Drop&) = 0;
    virtual void onEvent(const board::SoftDrop&) = 0;
    virtual void onEvent(const board::Lock&) = 0;
    virtual void onEvent(const board::TerminoAvailable&) = 0;
    virtual void onEvent(const board::IncomingAttack&) = 0;
    virtual void onEvent(const board::Attack&) = 0;

    virtual const Bitmap& bitmap() const = 0;
    virtual Bitmap& bitmap() = 0;
    virtual void reset() = 0;
};

struct TetrisBoardCallbacks
{
    bfc::light_function<Termino()> generate;
    bfc::light_function<void(std::vector<Line>)> replace;
    bfc::light_function<void(std::vector<Line>)> insert;
    bfc::light_function<void(std::vector<uint8_t>)> clear;
    bfc::light_function<void(CellCoord)> piecePosition;
    bfc::light_function<void(Termino)> placePiece;
    bfc::light_function<void(uint8_t)> rotate;
    bfc::light_function<void(std::vector<Termino>)> piecesAdded;
    bfc::light_function<void(Termino)> hold;
    bfc::light_function<void()> commit;
    bfc::light_function<void()> gameOver;
    bfc::light_function<void(uint8_t)> incomingAttack;
};


} // namespace tetris