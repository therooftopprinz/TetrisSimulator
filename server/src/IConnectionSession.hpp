#ifndef __ICONNECTIONSESSION_HPP__
#define __ICONNECTIONSESSION_HPP__

#include <interface/protocol.hpp>

namespace tetris
{

struct IConnectionSession
{
    IConnectionSession() {}
    virtual ~IConnectionSession() {}
    virtual void send(TetrisProtocol& pMessage) = 0;
    virtual void disassociateGame() = 0;
};

} // namespace tetris

#endif // __ICONNECTIONSESSION_HPP__