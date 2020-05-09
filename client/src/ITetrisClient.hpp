#ifndef __ITETRISCLIENT_HPP__
#define __ITETRISCLIENT_HPP__

#include <chrono>
#include <string>

#include <bfc/EpollReactor.hpp>
#include <bfc/CommandManager.hpp>

#include <interface/protocol.hpp>

namespace tetris
{

struct ITetrisClient
{
    virtual void send(TetrisProtocol& pMessage) = 0;
    virtual void consoleLog(const std::string&) = 0;
};

} // tetris

#endif // __ITETRISCLIENT_HPP__