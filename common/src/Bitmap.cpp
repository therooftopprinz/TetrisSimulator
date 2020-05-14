#include <common/Bitmap.hpp>

namespace tetris
{

Bitmap::Bitmap(uint8_t pWidth, uint8_t pHeight)
    : mWidth(pWidth)
    , mHeight(pHeight)
    , mData(pHeight)
{
    if (pWidth >= sizeof(Bitline)*8)
    {
        throw std::invalid_argument("unsupported width");
    }
}

std::pair<uint8_t, uint8_t> Bitmap::dimension() const
{
    return {mWidth, mHeight};
}

Bitline Bitmap::shiftUp(Bitline pValue)
{
    mData.emplace_front(pValue);
    auto rv = std::move(mData.back());
    mData.pop_back();
    return rv;
}

Bitline Bitmap::insertLine(uint8_t pLine, Bitline pValue)
{
    auto it = mData.begin();
    std::advance(it, pLine);
    mData.emplace(it, pValue);

    auto rv = std::move(mData.back());
    mData.pop_back();
    return rv;
}

void Bitmap::replaceLine(uint8_t pLine, Bitline pValue)
{
    mData[pLine] = pValue;
}

void Bitmap::clearLine(uint8_t pLine)
{
    auto it = mData.begin();
    std::advance(it, pLine);
    mData.erase(it);
    mData.emplace_back();
}

uint64_t Bitmap::line(uint8_t pLine) const
{
    return mData[pLine];
}

bool Bitmap::get(int8_t pX, int8_t pY) const
{
    if (pX<0 || pX >= mWidth || pY<0 || pY >= mHeight)
    {
        return true;
    }

    return mData[pY] & (1 << (mWidth-1-pX));
}

bool Bitmap::set(bool pValue, uint8_t pX, uint8_t pY)
{
    if (pX<0 || pX >= mWidth || pY<0 || pY >= mHeight)
    {
        return false;
    }

    uint64_t m = 1 << (mWidth-1-pX);
    uint64_t c = pValue ? ~uint64_t() : 0;
    auto& o = mData[pY];
    o = (~m & o) | (m & c);

    return true;
}

void Bitmap::reset()
{
    mData = decltype(mData)(mHeight);
}

} // namespace tetris
