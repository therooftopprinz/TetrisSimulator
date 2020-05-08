#ifndef __TETRISCLIENT_HPP__
#define __TETRISCLIENT_HPP__

#include <cstring>
#include <stdexcept>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

#include <bfc/EpollReactor.hpp>
#include <bfc/CommandManager.hpp>

#include <interface/protocol.hpp>

namespace tetris
{

struct TetrisClientConfig
{
    enum Operation {CREATE, JOIN};
    Operation operation;
    uint32_t operand;
    uint32_t ip;
    uint16_t port;
};

class TetrisClient
{
public:
    TetrisClient(const TetrisClientConfig& pConfig)
    {
        mClientFd = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == mClientFd)
        {
            throw std::runtime_error(strerror(errno));
        }

        sockaddr_in server;
        std::memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = pConfig.ip;
        server.sin_port = ntohs(pConfig.port);

        char loc[24];
        inet_ntop(AF_INET, &pConfig.ip, loc, sizeof(loc));
        std::cout << "connecting " << loc << ":" << pConfig.port << "\n";

        auto res = connect(mClientFd, (sockaddr*)&server, sizeof(server));

        if (-1 == res)
        {
            throw std::runtime_error(strerror(errno));
        }
        mCmdMan.addCommand("exit", [this](bfc::ArgsMap&&) -> std::string {
                mRunning = false;
                return "exiting...";
            });
        mCmdMan.addCommand("create", [this](bfc::ArgsMap&&) -> std::string {
                createGame();
                return "creating game...";
            });
        mCmdMan.addCommand("join", [this](bfc::ArgsMap&& pArgs) -> std::string {
                auto id = pArgs.argAs<int>("id");
                if (!id)
                {
                    return "no id";
                }
                join(*id);
                return "joining...";
            });
    }

    void join(int pId)
    {
        TetrisProtocol message = JoinRequest();
        auto& joinRequest = std::get<JoinRequest>(message);
        joinRequest.gameId = pId;

        send(message);
    }

    void createGame()
    {
        TetrisProtocol message = CreateGameRequest();
        auto& createGameRequest = std::get<CreateGameRequest>(message);
        createGameRequest.boardHeight = 24;
        createGameRequest.boardWidth = 10;

        send(message);
    }

    void send(TetrisProtocol& pMessage)
    {
        std::byte buffer[512];
        auto& msgSize = *(new (buffer) uint16_t(0)); 
        cum::per_codec_ctx context(buffer + 2, sizeof(buffer) -2);
        encode_per(pMessage, context);

        msgSize = sizeof(buffer) - context.size();

        auto res = ::send(mClientFd, buffer, msgSize+2, 0);
        if (-1 == res)
        {
            throw std::runtime_error(strerror(errno));
        }
    }

    void run()
    {
        while (mRunning)
        {
            std::string line;
            std::cout << "$ ";
            std::getline(std::cin, line);
            auto res = mCmdMan.executeCommand(line);
            std::cout <<  res << "\n";
        }
    }

    void stop()
    {
        mRunning = false;
    }
private:
    bfc::CommandManager mCmdMan;
    int mClientFd;
    bool mRunning;
};

} // namespace tetris

#endif // __TETRISCLIENT_HPP__