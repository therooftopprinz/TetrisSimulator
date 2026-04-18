#ifndef __IEXECUTOR_HPP__
#define __IEXECUTOR_HPP__

#include <functional>

#include <bfc/function.hpp>

namespace tetris
{

struct IExecutor
{
    virtual void trigger(std::function<void()>) = 0;
    virtual void trigger(bfc::light_function<void()>) = 0;
};

} // tetris

#endif // __IEXECUTOR_HPP__