#ifndef __TETRIS_TETRISBOARD_HPP__
#define __TETRIS_TETRISBOARD_HPP__

#include <cstdint>
#include <deque>
#include <cmath>
#include <optional>

#include <bfc/FixedFunctionObject.hpp>

#include <interface/protocol.hpp>

#include <common/Bitmap.hpp>
#include <common/TetrisBoardEvents.hpp>
#include <common/Terminoes.hpp>

namespace tetris
{

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
};

struct TetrisBoardConfig
{
    uint8_t width;
    uint8_t height;
};

class TetrisBoard
{
public:

    TetrisBoard(const TetrisBoardConfig& pConfig, TetrisBoardCallbacks& pCallbacks);

    template <typename T>
    void onEvent(const T& pEvent)
    {
        if (NONE == mCurrent)
        {
            return;
        }
        doEvent(pEvent);
        mCallbacks.commit();
    }

    void onEvent(const board::TerminoAvailable&);
    const Bitmap& bitmap() const;
    Bitmap& bitmap();
    bool isGameOver() const;
    void reset();

private:
    void doEvent(const board::Move& pEvent);
    void doEvent(const board::Rotate& pEvent);
    void doEvent(const board::Hold&);
    void doEvent(const board::Drop&);
    void doEvent(const board::SoftDrop&);
    void doEvent(const board::Lock&);

    void lock();
    void initializeCurrentTermino(Termino pTerm);
    void nextPiece();
    bool requestPiece();
    TransformFn createTransformerFromRotator(traits::RotatorFn& pRotator);

    uint8_t mWidth;
    uint8_t mHeight;

    CellCoord mXY = {};
    uint8_t mRot = 0;

    Termino mCurrent = NONE;
    std::optional<Termino> mHold;

    traits::CheckerFn* mCurrentCheckerFn = nullptr;
    TransformFn mCurrentTransformer;
    traits::SetterFn* mCurrentSetterFn = nullptr;

    std::deque<Termino> mPieceList;

    TetrisBoardConfig mConfig;
    TetrisBoardCallbacks& mCallbacks;
    Bitmap mData;
    bool mGameOver;
};

} // namespace tetris

#endif // __TETRIS_TETRISBOARD_HPP__