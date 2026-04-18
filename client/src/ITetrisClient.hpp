#ifndef __ITETRISCLIENT_HPP__
#define __ITETRISCLIENT_HPP__

#include <chrono>
#include <string>

#include <bfc/function.hpp>

#include <interface/protocol_export.hpp>

namespace tetris
{

struct ITetrisClient
{
    virtual ~ITetrisClient() = default;

    virtual void send(TetrisProtocol&) = 0;
    virtual void consoleLog(const std::string&) = 0;
    virtual void enableConsole() = 0;
    virtual void disableConsole() = 0;
    virtual void setKeyHandler(bfc::light_function<void(char)>) = 0;
    virtual void requestExitToLobby() = 0;
    virtual void sendLeaveIndication() = 0;
    virtual void consoleIn(char) {}

    virtual void paintGameView() {}
    virtual void notifyMatchEnded() {}
    virtual void notifyMatchStarted() {}

    virtual bool tryHandleGameplayChat(char)
    {
        return false;
    }

    /** While gameplay owns the key handler, still route /commands to the console line editor. */
    virtual bool gameplayKeyRoutesToConsole(char) const
    {
        return false;
    }
};

} // tetris

#endif // __ITETRISCLIENT_HPP__