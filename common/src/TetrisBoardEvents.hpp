#ifndef __TETRIS_TETRISBOARDEVENTS_HPP__
#define __TETRIS_TETRISBOARDEVENTS_HPP__

#include <cstdint>

namespace tetris
{

namespace board
{

struct Move
{
    int8_t offset;
};

struct Rotate
{
    int8_t count;
};

struct Attack
{
    uint8_t count;
};

struct IncomingAttack
{
    uint8_t count;
};

struct Hold {};
struct Drop {};
struct SoftDrop {};
struct Lock {};
struct TerminoAvailable {};

} // namespace board

} // namespace tetris

#endif // __TETRIS_TETRISBOARDEVENTS_HPP__