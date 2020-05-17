#include <common/StandardTetrisBoard.hpp>

namespace tetris
{

StandardTetrisBoard::StandardTetrisBoard(const TetrisBoardConfig& pConfig, TetrisBoardCallbacks& pCallbacks)
    : mConfig(pConfig)
    , mCallbacks(pCallbacks)
    , mData(pConfig.width, pConfig.height)
{}

void StandardTetrisBoard::onEvent(const board::TerminoAvailable&)
{
    if (NONE == mCurrent)
    {
        nextPiece();
    }
    else
    {
        requestPiece();
    }
    mCallbacks.commit();
}

const Bitmap& StandardTetrisBoard::bitmap() const
{
    return mData;
}

Bitmap& StandardTetrisBoard::bitmap()
{
    return mData;
}

bool StandardTetrisBoard::isGameOver() const
{
    return mGameOver;
}

void StandardTetrisBoard::reset()
{
    mPieceList.clear();
    mData.reset();
    mGameOver = false;

    nextPiece();
    mCallbacks.commit();
}

void StandardTetrisBoard::onEvent(const board::Move& pEvent)
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
    mCallbacks.piecePosition(mXY);
    mCallbacks.commit();
}

void StandardTetrisBoard::onEvent(const board::Rotate& pEvent)
{
    if (0 == pEvent.count)
    {
        return;
    }

    // TODO: Extract rotation logic via liskov
    // proposed interface
    // class IRotator:
    // + CellCoord check(Bitmap pBitmap, Termino pTermino, uint8_t pOrient, uin8_t pEnd)

    uint8_t origRot = mRot;
    uint8_t count = std::abs(pEvent.count) % 4;
    mRot = pEvent.count > 0 ? mRot + count : mRot + (4 - count);
    mRot %= 4;

    CellCoord accepted = mXY;
    bool isAccepted = false;

    auto checkPos = [this, &accepted, &isAccepted](auto& pCoords) -> bool {
        for (auto& i : pCoords)
        {
            CellCoord pos{mXY.first + i.first, mXY.second + i.second};
            auto res = (*mCurrentCheckerFn)(mData, pos.first, pos.second, mCurrentTransformer);
            if (res)
            {
                continue;
            }

            accepted = pos;
            isAccepted = true;
            
            return true;
        }
        return false;
    };

    enum Direction{RIGHT, LEFT};

    auto check = [this, &checkPos] (uint8_t pRot, Direction pDir)
    {
            pRot %= 4;
            auto& wallKick = traits::gSrsWallKicks[traits::TerminoRotation(mCurrent, pRot)];
            auto& coords = pDir == RIGHT ? wallKick.second : wallKick.first;
            checkPos(coords);
    };

    if (count%2)
    {
        check(origRot, pEvent.count > 0 ? RIGHT : LEFT );
    }
    else
    {
        {
            check(mRot+1, LEFT);
        }
        if (!isAccepted)
        {
            check(mRot+3, RIGHT);
        }
    }

    if (!isAccepted)
    {
        mRot = origRot;
        return;
    }

    mXY = accepted;
    mCallbacks.rotate(mRot);
    mCallbacks.piecePosition(mXY);
    mCallbacks.commit();
}

void StandardTetrisBoard::onEvent(const board::Hold&)
{
    if (!mHasHoldCredit)
    {
        return;
    }

    mHasHoldCredit = false;

    if (mHold)
    {
        auto current = mCurrent;
        auto held = *mHold;
        mHold.emplace(current);
        nextPiece(held);
    }
    else
    {
        mHold.emplace(mCurrent);
        nextPiece();
    }
    mCallbacks.commit();
}

void StandardTetrisBoard::onEvent(const board::Drop&)
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

void StandardTetrisBoard::onEvent(const board::SoftDrop&)
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
            mXY.second = ypos+1;
            break;
        }
        ypos -= 1;
    }
    mCallbacks.piecePosition(mXY);
    mCallbacks.commit();
}

void StandardTetrisBoard::onEvent(const board::Lock&)
{
    auto res = (*mCurrentCheckerFn)(mData, mXY.first, mXY.second-1, mCurrentTransformer);
    if (res)
    {
        lock();
    }
    else
    {
        mXY.second -= 1;
        mCallbacks.piecePosition(mXY);
        mCallbacks.commit();
    }
}

void StandardTetrisBoard::lock()
{
    (*mCurrentSetterFn)(mData, mXY.first, mXY.second, mCurrentTransformer);
    auto startLine = mXY.second;
    auto& termino = traits::gTerminoTraitsMap[mCurrent];
    auto height = std::get<1>(termino);
    std::vector<Line> lineChanged;
    for (auto i=0u; i<height; i++)
    {
        int8_t pos = startLine + i;
        if (pos>=0)
        {
            uint8_t upos(pos);
            lineChanged.emplace_back(Line{upos, uint16_t(mData.line(upos))});
        }
    }

    mCallbacks.replace(std::move(lineChanged));

    Bitline clearMask = -1;
    clearMask >>= (sizeof(Bitline)*8) - mData.dimension().first;

    std::vector<uint8_t> lineRemoved;

    for (int8_t ypos = mData.dimension().second-1; ypos >= 0; ypos--)
    {
        if (clearMask == mData.line(ypos))
        {
            lineRemoved.emplace_back(ypos);
            mData.clearLine(ypos);
        }
    }

    mCallbacks.clear(std::move(lineRemoved));

    mHasHoldCredit = true;
    nextPiece();
}

void StandardTetrisBoard::initializeCurrentTermino(Termino pTerm)
{
    mCurrent = pTerm;
    mRot = 0;
    auto& termino = traits::gTerminoTraitsMap[mCurrent];
    mCurrentCheckerFn = &std::get<2>(termino);
    auto& rotator = std::get<3>(termino);
    mCurrentSetterFn = &std::get<4>(termino);
    mCurrentTransformer = createTransformerFromRotator(rotator);

    auto x = std::floor(double(mConfig.width)/2 - double(std::get<traits::WIDTH>(traits::gTerminoTraitsMap[mCurrent]))/2);
    auto y = mConfig.height - std::get<traits::HEIGHT>(traits::gTerminoTraitsMap[mCurrent]);
    mXY = CellCoord{x,y};
}

void StandardTetrisBoard::nextPiece(std::optional<Termino> pNext)
{
    if (!pNext && !mPieceList.size())
    {
        if (!requestPiece())
        {
            mCallbacks.commit();
            return;
        }
    }

    if (!pNext)
    {
        initializeCurrentTermino(mPieceList.front());
        mPieceList.pop_front();
    }
    else
    {
        initializeCurrentTermino(*pNext);
    }

    auto res = (*mCurrentCheckerFn)(mData, mXY.first, mXY.second, mCurrentTransformer);
    if (0 != res)
    {
        mCallbacks.commit();
        mCallbacks.gameOver();
        return;
    }

    mCallbacks.placePiece(mCurrent);
    mCallbacks.piecePosition(mXY);

    requestPiece();
    mCallbacks.commit();
}

bool StandardTetrisBoard::requestPiece()
{
    std::vector<Termino> pieces;
    while (5 > mPieceList.size())
    {
        auto termino = mCallbacks.generate();
        if (NONE == termino)
        {
            break;
        }
        mPieceList.emplace_back(termino);
        pieces.emplace_back(termino);
    }

    mCallbacks.piecesAdded(std::move(pieces));
    return mPieceList.size();
}

TransformFn StandardTetrisBoard::createTransformerFromRotator(traits::RotatorFn& pRotator)
{
    return [this, &pRotator](CellCoord pCoord)
        {
            return pRotator(this->mRot, pCoord);
        };
}

} // namespace tetris