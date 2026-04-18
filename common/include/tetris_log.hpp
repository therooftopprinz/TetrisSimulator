#ifndef __TETRIS_LOG_HPP__
#define __TETRIS_LOG_HPP__

#include <cstdint>
#include <utility>

#include <logless/logger.hpp>

inline logless::logger& tetris_logger()
{
    static logless::logger instance("log.bin");
    return instance;
}

inline logless::buffer_log_t BufferLog(uint16_t p_size, const void* p_data)
{
    return {p_size, p_data};
}

#endif
