#ifndef __IATTACKED_HPP__
#define __IATTACKED_HPP__

#include <functional>

#include <bfc/FixedFunctionObject.hpp>

namespace tetris
{

class PlayerContext;

struct IAttacker
{
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void attack(PlayerContext&, uint8_t) = 0;
};

} // tetris

#endif // __IATTACKED_HPP__