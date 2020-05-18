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
    virtual bool isGameOver() const = 0;
    virtual void reset() = 0;
};

struct TetrisBoardCallbacks
{
    bfc::LightFn<Termino()> generate;
    bfc::LightFn<void(std::vector<Line>)> replace;
    bfc::LightFn<void(std::vector<Line>)> insert;
    bfc::LightFn<void(std::vector<uint8_t>)> clear;
    bfc::LightFn<void(CellCoord)> piecePosition;
    bfc::LightFn<void(Termino)> placePiece;
    bfc::LightFn<void(uint8_t)> rotate;
    bfc::LightFn<void(std::vector<Termino>)> piecesAdded;
    bfc::LightFn<void()> hold;
    bfc::LightFn<void()> commit;
    bfc::LightFn<void()> gameOver;
    bfc::LightFn<void(uint8_t)> incomingAttack;
};


} // namespace tetris