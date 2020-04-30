#ifndef __TETRIS_TETRISBOARD_HPP__
#define __TETRIS_TETRISBOARD_HPP__

#include <cstdint>
#include <deque>

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
    virtual void piecePositionUpdate(CellCoord) = 0;
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
        : mData(pConfig.width, pConfig.height)
    {}

    void onEvent(const Move& pEvent)
    {
        // auto xpos = mXY.first + pEvent.offset;
        // auto& termino = traits::gTerminoTraitsMap[mCurrent];
        // auto& checker = std::get<2>(termino);
        // auto& rotator = std::get<3>(termino);

        // TransformFn rot = [this, &rotator](CellCoord pCoord)
        // {
        //     return rotator(this->mRot, pCoord);
        // }

        // checker(mData, xpos, mXY.second,)
    }

    void onEvent(const Rotate&);
    void onEvent(const Hold&);
    void onEvent(const Drop&);
    void onEvent(const SoftDrop&);

    void onEvent(const Lock&)
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
        mPieceList.pop_front();

        requestPiece();

        auto x = mConfig.height - std::get<traits::HEIGHT>(traits::gTerminoTraitsMap[mCurrent]);
        auto y = mConfig.width - std::get<traits::WIDTH>(traits::gTerminoTraitsMap[mCurrent])/2;
        mXY = CellCoord{x,y};
    }

    void requestPiece()
    {
        while (5 == mPieceList.size())
        {
            mPieceList.push_back(mCallbacks->generate());
        }
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