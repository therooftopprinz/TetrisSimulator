#ifndef __GAMETIMERID_HPP__
#define __GAMETIMERID_HPP__

#include <bfc/function.hpp>
#include <bfc/timer.hpp>

namespace tetris
{

using GameTimerId = bfc::timer<bfc::light_function<void()>>::timer_id_t;

} // namespace tetris

#endif
