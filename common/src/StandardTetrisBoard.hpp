#ifndef __TETRIS_TETRISBOARD_HPP__
#define __TETRIS_TETRISBOARD_HPP__

#include <cstdint>
#include <deque>
#include <cmath>
#include <optional>

#include <bfc/FixedFunctionObject.hpp>

#include <interface/protocol.hpp>

#include <common/ITetrisBoard.hpp>
#include <common/Terminoes.hpp>

namespace tetris
{

struct TetrisBoardConfig
{
    uint8_t width;
    uint8_t height;
};

class StandardTetrisBoard : public ITetrisBoard
{
public:

    StandardTetrisBoard(const TetrisBoardConfig& pConfig, TetrisBoardCallbacks& pCallbacks);

    void onEvent(const board::Move& pEvent);
    void onEvent(const board::Rotate& pEvent);
    void onEvent(const board::Hold&);
    void onEvent(const board::Drop&);
    void onEvent(const board::SoftDrop&);
    void onEvent(const board::Lock&);
    void onEvent(const board::TerminoAvailable&);
    void onEvent(const board::IncomingAttack&);
    void onEvent(const board::Attack&);

    const Bitmap& bitmap() const;
    Bitmap& bitmap();
    void reset();

private:

    void lock();
    void initializeCurrentTermino(Termino pTerm);
    void nextPiece(std::optional<Termino> pNext = std::nullopt);
    bool requestPiece();
    TransformFn createTransformerFromRotator(traits::RotatorFn& pRotator);

    uint8_t mWidth;
    uint8_t mHeight;

    CellCoord mXY = {};
    uint8_t mRot = 0;

    Termino mCurrent = NONE;
    std::optional<Termino> mHold;
    bool mHasHoldCredit = false;

    traits::CheckerFn* mCurrentCheckerFn = nullptr;
    TransformFn mCurrentTransformer;
    traits::SetterFn* mCurrentSetterFn = nullptr;

    std::deque<Termino> mPieceList;

    TetrisBoardConfig mConfig;
    TetrisBoardCallbacks& mCallbacks;
    Bitmap mData;
};

} // namespace tetris

#endif // __TETRIS_TETRISBOARD_HPP__