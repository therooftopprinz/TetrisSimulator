#ifndef __TETRIS_TETRISBOARD_HPP__
#define __TETRIS_TETRISBOARD_HPP__

#include <cstdint>
#include <deque>
#include <cmath>

#include <bfc/FixedFunctionObject.hpp>

#include <common/Bitmap.hpp>
#include <common/TetrisBoardEvents.hpp>
#include <common/Terminoes.hpp>

namespace tetris
{

struct ITetrisBoardCallbacks
{
    virtual Termino generate() = 0;
    virtual void clear(std::vector<uint8_t>) = 0;
    virtual void piecePosition(CellCoord) = 0;
    virtual void newPiece(Termino) = 0;
};

struct TetrisBoardConfig
{
    uint8_t width;
    uint8_t height;
};

class TetrisBoard
{
public:

    TetrisBoard(const TetrisBoardConfig& pConfig, ITetrisBoardCallbacks* pCallbacks)
        : mConfig(pConfig)
        , mCallbacks(pCallbacks)
        , mData(pConfig.width, pConfig.height)
    {}

    void onEvent(const board::Move& pEvent)
    {
        auto& termino = traits::gTerminoTraitsMap[mCurrent];
        auto& checker = std::get<2>(termino);
        auto& rotator = std::get<3>(termino);
        auto currentRotation = createRotator(rotator);
        int direction = pEvent.offset > 0 ? 1 : -1;
        int count = std::abs(pEvent.offset);
        int xpos = mXY.first;
        int res = 0;

        for ( ; count>0 ; count--)
        {
            auto checkxpos = xpos + direction;
            res = checker(mData, checkxpos, mXY.second, currentRotation);

            if (0 != res)
            {
                break;
            }

            xpos = checkxpos;
        }

        mXY.first = xpos;
        mCallbacks->piecePosition(mXY);
    }

    void onEvent(const board::Rotate& pEvent)
    {
        // auto origRot = mRot;
        mRot = pEvent.count;
        // TODO check wall kicks;
    }
    void onEvent(const board::Hold&);
    void onEvent(const board::Drop&);
    void onEvent(const board::SoftDrop&);

    void onEvent(const board::Lock&)
    {  
        
    }

    const Bitmap& bitmap() const
    {
        return mData;
    }
    
    Bitmap& bitmap()
    {
        return mData;
    }

    bool isGameOver();
    void restart()
    {
        mPieceList.clear();
        mData.reset();

        nextPiece();
    }

private:

    void nextPiece()
    {
        if (!mPieceList.size())
        {
            requestPiece();
        }

        mCurrent = mPieceList.front();
        mCallbacks->newPiece(mCurrent);
        mPieceList.pop_front();

        requestPiece();

        auto x = mConfig.width/2 - std::get<traits::WIDTH>(traits::gTerminoTraitsMap[mCurrent])/2;
        auto y = mConfig.height - std::get<traits::HEIGHT>(traits::gTerminoTraitsMap[mCurrent]);
        mXY = CellCoord{x,y};
        mCallbacks->piecePosition(mXY);
    }

    void requestPiece()
    {
        while (5 > mPieceList.size())
        {
            mPieceList.push_back(mCallbacks->generate());
        }
    }

    TransformFn createRotator(traits::RotatorFn& pRotator)
    {
        return [this, &pRotator](CellCoord pCoord)
            {
                return pRotator(this->mRot, pCoord);
            };
    }

    uint8_t mWidth;
    uint8_t mHeight;

    CellCoord mXY;
    uint8_t mRot = 0;
    Termino mCurrent;
    std::optional<Termino> mHold;
    std::deque<Termino> mPieceList;

    TetrisBoardConfig mConfig;
    ITetrisBoardCallbacks* mCallbacks;
    Bitmap mData;
};

} // namespace tetris

#endif // __TETRIS_TETRISBOARD_HPP__