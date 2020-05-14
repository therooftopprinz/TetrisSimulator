#ifndef __TETRIS_BITMAP_HPP__
#define __TETRIS_BITMAP_HPP__

#include <cstring>
#include <deque>
#include <utility>
#include <cstdint>
#include <vector>
#include <stdexcept>

namespace tetris
{

using Bitline = uint16_t;
using DiffBitline = std::pair<uint8_t, Bitline>;
using BitmapPatch = std::vector<DiffBitline>;

class Bitmap
{
public:
    Bitmap(uint8_t pWidth, uint8_t pHeight);
    Bitmap() = delete;

    std::pair<uint8_t, uint8_t> dimension() const;
    Bitline shiftUp(Bitline pValue);
    Bitline insertLine(uint8_t pLine, Bitline pValue);
    void replaceLine(uint8_t pLine, Bitline pValue);
    void clearLine(uint8_t pLine);
    uint64_t line(uint8_t pLine) const;
    bool get(int8_t pX, int8_t pY) const;
    bool set(bool pValue, uint8_t pX, uint8_t pY);
    void reset();

private: 
    uint8_t mWidth;
    uint8_t mHeight;
    std::deque<Bitline> mData;
};

} // namespace tetris

#endif // __TETRIS_BITMAP_HPP__HPP__
