#ifndef __TETRIS_BITMAP_HPP__
#define __TETRIS_BITMAP_HPP__

#include <cstring>
#include <deque>
#include <utility>

namespace tetris
{

using Bitline = uint16_t;
using DiffBitline = std::pair<uint8_t, Bitline>;
using BitmapPatch = std::vector<DiffBitline>;

class Bitmap
{
public:
    Bitmap(uint8_t pWidth, uint8_t pHeight)
        : mWidth(pWidth)
        , mHeight(pHeight)
        , mData(pHeight)
    {
        if (pWidth >= sizeof(Bitline)*8)
        {
            throw std::invalid_argument("unsupported width");
        }
    }

    Bitmap() = delete;

    std::pair<uint8_t, uint8_t> dimension() const
    {
        return {mWidth, mHeight};
    }

    Bitline shiftUp(Bitline pValue)
    {
        mData.emplace_front(pValue);
        auto rv = std::move(mData.back());
        mData.pop_back();
        return rv;
    }

    Bitline insertLine(uint8_t pLine, Bitline pValue)
    {
        auto it = mData.begin();
        std::advance(it, pLine);
        mData.emplace(it, pValue);

        auto rv = std::move(mData.back());
        mData.pop_back();
        return rv;
    }

    void replaceLine(uint8_t pLine, Bitline pValue)
    {
        mData[pLine] = pValue;
    }

    void clearLine(uint8_t pLine)
    {
        auto it = mData.begin();
        std::advance(it, pLine);
        mData.erase(it);
        mData.emplace_back();
    }

    uint64_t line(uint8_t pLine) const
    {
        return mData[pLine];
    }

    bool get(int8_t pX, int8_t pY) const
    {
        if (pX<0 || pY >= mWidth || pY<0 || pY >= mHeight)
        {
            return true;
        }

        return mData[pY] & (1 << (mWidth-1-pX));
    }

    void set(bool pValue, uint8_t pX, uint8_t pY)
    {
        uint64_t m = 1 << (mWidth-1-pX);
        uint64_t c = pValue ? ~uint64_t() : 0;
        auto& o = mData[pY];
        o = (~m & o) | (m & c);
    }

    void reset()
    {
        mData = decltype(mData)(mHeight);
    }

private: 
    uint8_t mWidth;
    uint8_t mHeight;
    std::deque<Bitline> mData;
};

} // namespace tetris

#endif // __TETRIS_BITMAP_HPP__HPP__
