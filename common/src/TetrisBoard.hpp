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
    virtual void hold() = 0;
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

    template <typename T>
    void onEvent(const T& pEvent)
    {
        if (NONE == mCurrent)
        {
            return;
        }
        doEvent(pEvent);
    }

    void onEvent(const board::TerminoAvailable&)
    {
        if (NONE == mCurrent)
        {
            nextPiece();
        }
        else
        {
            requestPiece();
        }
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
    void doEvent(const board::Move& pEvent)
    {
        int direction = pEvent.offset > 0 ? 1 : -1;
        int count = std::abs(pEvent.offset);
        int xpos = mXY.first;
        int res = 0;

        for ( ; count>0 ; count--)
        {
            auto checkxpos = xpos + direction;

            res = (*mCurrentCheckerFn)(mData, checkxpos, mXY.second, mCurrentTransformer);

            if (0 != res)
            {
                break;
            }

            xpos = checkxpos;
        }

        mXY.first = xpos;
        mCallbacks->piecePosition(mXY);
    }

    void doEvent(const board::Rotate& pEvent)
    {
        // auto origRot = mRot;
        mRot = pEvent.count;
        // TODO check wall kicks;
    }

    void doEvent(const board::Hold&);
    void doEvent(const board::Drop&)
    {
        auto ypos = mXY.second;
        while (true)
        {
            auto res = (*mCurrentCheckerFn)(mData, mXY.first, ypos-1, mCurrentTransformer);
            if (res)
            {
                mXY.second = ypos;
                break;
            }
            ypos -= 1;
        }
        lock();
    }

    void doEvent(const board::SoftDrop&)
    {
        auto ypos = mXY.second;
        while (true)
        {
            auto res = (*mCurrentCheckerFn)(mData, mXY.first, ypos-1, mCurrentTransformer);
            if (res)
            {
                if (ypos == mXY.second)
                {
                    lock();
                    return;
                }
                mXY.second = ypos;
                break;
            }
            ypos -= 1;
        }
        mCallbacks->piecePosition(mXY);
    }

    void doEvent(const board::Lock&)
    {
        auto res = (*mCurrentCheckerFn)(mData, mXY.first, mXY.second-1, mCurrentTransformer);
        if (res)
        {
            lock();
        }
        else
        {
            mXY.second -= 1;
            mCallbacks->piecePosition(mXY);
        }
    }

    void lock()
    {
        (*mCurrentSetterFn)(mData, mXY.first, mXY.second, mCurrentTransformer);
        nextPiece();
    }

    void initializeCurrentTermino(Termino pTerm)
    {
        mCurrent = pTerm;
        auto& termino = traits::gTerminoTraitsMap[mCurrent];
        mCurrentCheckerFn = &std::get<2>(termino);
        auto& rotator = std::get<3>(termino);
        mCurrentSetterFn = &std::get<4>(termino);
        mCurrentTransformer = createTransformerFromRotator(rotator);
    }

    void nextPiece()
    {
        mCurrent = NONE;

        if (!mPieceList.size())
        {
            if (!requestPiece())
            {
                return;
            }
        }

        initializeCurrentTermino(mPieceList.front());

        mCallbacks->newPiece(mCurrent);
        mPieceList.pop_front();

        requestPiece();

        auto x = std::floor(double(mConfig.width)/2 - double(std::get<traits::WIDTH>(traits::gTerminoTraitsMap[mCurrent]))/2);
        auto y = mConfig.height - std::get<traits::HEIGHT>(traits::gTerminoTraitsMap[mCurrent]);
        mXY = CellCoord{x,y};
        mCallbacks->piecePosition(mXY);
    }

    bool requestPiece()
    {
        while (5 > mPieceList.size())
        {
            auto termino = mCallbacks->generate();
            if (NONE == termino)
            {
                return false;
            }
            mPieceList.emplace_back(termino);
        }
        return true;
    }

    TransformFn createTransformerFromRotator(traits::RotatorFn& pRotator)
    {
        return [this, &pRotator](CellCoord pCoord)
            {
                return pRotator(this->mRot, pCoord);
            };
    }

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
    ITetrisBoardCallbacks* mCallbacks;
    Bitmap mData;
};

} // namespace tetris

#endif // __TETRIS_TETRISBOARD_HPP__