#ifndef __TETRIS_BITMAP_HPP__
#define __TETRIS_BITMAP_HPP__

#include <cstring>

namespace tetris
{
    
class BitBuffer
{
public:
    BitBuffer() = delete;
    BitBuffer(size_t pSize)
        : mBitSize(pSize)
        , mSize{(pSize>>3)+bool(pSize%8)}
        , mData(pSize > sizeof(mDataSbo)*8 ? new uint8_t[mSize] : mDataSbo)
    {
        std::memset(mData, 0, mSize);
    }
    
    BitBuffer(BitBuffer&& pOther)
        : mBitSize(pOther.mBitSize)
        , mSize(pOther.mSize)
        , mData(pOther.mData)
    {
        pOther.mData = nullptr;
    }
    
    BitBuffer(const BitBuffer& pOther)
        : mBitSize(pOther.mBitSize)
        , mSize(pOther.mSize)
        , mData(mSize > sizeof(mDataSbo)*8 ? new uint8_t[mSize] : mDataSbo)
    {

        std::memcpy(mData, pOther.mData, mSize);
    }
    
    BitBuffer& operator=(BitBuffer&& pOther)
    {
        reset();

        mBitSize = pOther.mBitSize;
        mSize = pOther.mSize;
        mData = pOther.mData;

        pOther.mData = nullptr;
        pOther.reset();

        return *this;
    }
    
    BitBuffer& operator=(const BitBuffer& pOther)
    {
        reset();

        mBitSize = pOther.mBitSize;
        mSize = pOther.mSize;
        mData = mSize > sizeof(mDataSbo)*8 ? new uint8_t[mSize] : mDataSbo;

        std::memcpy(mData, pOther.mData, mSize);
        
        return *this;
    }

    ~BitBuffer()
    {
        reset();
    }

    bool get(size_t pIndex) const
    {
        return mData[pIndex>>3] & (0x80 >> pIndex%8);
    }
    
    void set(bool pValue, size_t pIndex)
    {
        auto& o = mData[pIndex>>3];
        uint8_t m = (0x80 >> pIndex%8);
        uint8_t c = pValue ? 0xFF : 0;
        o = (~m & ~o) | (m&c);
    }
    
    const uint8_t* raw() const
    {
        return mData;
    }

    uint8_t* raw()
    {
        return mData;
    }
    
    size_t size() const
    {
        return mSize;
    }
    
    size_t bitSize()
    {
        return mBitSize;
    }

private:
    void reset()
    {
        mBitSize = 0;
        mSize = 0;
        if (mData && mData != mDataSbo)
        {
            delete[] mData;    
        }
    }
    size_t mBitSize;
    size_t mSize;
    uint8_t mDataSbo[8];
    uint8_t* mData;
};

struct BitmapPatchLine
{
    int line;
    BitBuffer diff;
};

using BitmapPatch = std::vector<BitmapPatchLine>;

class Bitmap
{
public:
    std::pair<uint8_t, uint8_t> dimension();
    bool get(uint8_t x, uint8_t y);
    void set(bool value, uint8_t x, uint8_t y);
    const uint8_t* raw() const;
    uint8_t* raw();
    void snapshot();
    
private:
    // BitBuffer 
};

} // namespace tetris

#endif // __TETRIS_BITMAP_HPP__HPP__
