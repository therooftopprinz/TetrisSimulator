#ifndef __TETRISCLIENT_HPP__
#define __TETRISCLIENT_HPP__

#include <cstring>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <termios.h>

#include <bfc/EpollReactor.hpp>
#include <bfc/CommandManager.hpp>

#include <logless/Logger.hpp>

#include <interface/protocol.hpp>

#include <ITetrisClient.hpp>
#include <GameMaster.hpp>

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

class TetrisClient : ITetrisClient
{
public:
    TetrisClient(const TetrisClientConfig& pConfig)
    {
        mFd = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == mFd)
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
        Logless("TetrisClient : connecting to _:_ ", loc, pConfig.port);

        auto res = connect(mFd, (sockaddr*)&server, sizeof(server));

        if (-1 == res)
        {
            throw std::runtime_error(strerror(errno));
        }

        if (!mReactor.addHandler(mFd, [this](){
                onClientReadAvailable();
            }))
        {
            throw std::runtime_error("Failed to add client socket to EpollReactor!");
        }

        initTerminal();

        mCmdMan.addCommand("exit", [this](bfc::ArgsMap&&) -> std::string {
                mReactor.stop();
                return "exiting...";
            });
        mCmdMan.addCommand("create", [this](bfc::ArgsMap&& pArgs) -> std::string {
                return cmdCreate(std::move(pArgs));
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

        mKeyHandler = [this](char key) -> void{
                consoleIn(key);
            };
    }

    void run()
    {
        mReactor.run();
    }

private:
    void onClientReadAvailable()
    {
        int readSize = 0;
        if (WAIT_HEADER == mReadState)
        {
            readSize = 2;
        }
        else
        {
            readSize = mExpectedReadSize - mBuffIdx;
        }

        auto res = read(mFd, mBuff+mBuffIdx, readSize);

        if (-1 == res)
        {
            throw std::runtime_error(strerror(errno));
        }
        if (0 == res)
        {
            throw std::runtime_error("Server disconnected!");
        }

        mBuffIdx += res;

        if (WAIT_HEADER == mReadState)
        {
            std::memcpy(&mExpectedReadSize, mBuff, 2);
            mBuffIdx = 0;
            mReadState = WAIT_REMAINING;
            return;
        }

        if (mExpectedReadSize == mBuffIdx)
        {
            decodeMessage();
            mReadState = WAIT_HEADER;
            mBuffIdx = 0;
        }
    }

    void decodeMessage()
    {
        TetrisProtocol message;
        cum::per_codec_ctx context(mBuff, mBuffIdx);
        decode_per(message, context);

        std::string stred;
        str("root", message, stred, true);

        Logless("ConnectionSession[fd=_] : Receive raw=_ decoded=_", mFd, BufferLog(mBuffIdx, mBuff), stred.c_str());

        std::visit([this](auto& pMsg){
                onMsg(pMsg);
            }, message);
    }

    template <typename T>
    void onMsg(T& pMsg)
    {
        throw std::runtime_error("Unhandled Message");
    }

    void onMsg(CreateGameAccept& pMsg)
    {
        if (GM_INIT != mState)
        {
            consoleLog("[client]: Unexpected CreateGameAccept from server!");
            return;
        }

        consoleLog("[client]: Game created! id=", pMsg.gameId);
    }

    void onMsg(CreateGameReject& pMsg)
    {
        mGm.reset();
    }

    void onMsg(JoinAccept& pMsg)
    {

    }

    void onMsg(JoinReject& pMsg)
    {

    }

    void join(int pId)
    {
        TetrisProtocol message = JoinRequest();
        auto& joinRequest = std::get<JoinRequest>(message);
        joinRequest.gameId = pId;

        send(message);
    }

    std::string cmdCreate(bfc::ArgsMap&& pArgs)
    {
        if (IDLE != mState)
        {
            return "Can't create game!";
        }

        TetrisProtocol message = CreateGameRequest();
        auto& createGameRequest = std::get<CreateGameRequest>(message);
        createGameRequest.boardHeight = 24;
        createGameRequest.boardWidth = 10;

        send(message);
        mState = GM_INIT;

        mGm.emplace((ITetrisClient&)*this);

        return "Creating game...";
    }

    void send(TetrisProtocol& pMessage)
    {
        std::byte buffer[512];
        auto& msgSize = *(new (buffer) uint16_t(0));
        cum::per_codec_ctx context(buffer + 2, sizeof(buffer) -2);
        encode_per(pMessage, context);

        msgSize = sizeof(buffer) - context.size();

        auto res = ::send(mFd, buffer, msgSize+2, 0);
        if (-1 == res)
        {
            throw std::runtime_error(strerror(errno));
        }
    }

    void consoleIn(char pKey)
    {
        if (mConsolseWaitChar)
        {
            mConsolseWaitChar--;
            return;
        }

        if (27 == pKey)
        {
            mConsolseWaitChar = 2;
            return;
        }

        if (9 == pKey)
        {
            return;
        }
        if (127 == pKey)
        {
            if (0 == mConsoleInputBufferIdx)
            {
                return;
            }

            mConsoleInputBufferIdx--;
            std::cout << "\b \b" << std::flush;

            return;
        }

        if (mConsoleInputBufferIdx>sizeof(mConsoleInputBuffer)-1)
        {
            return;
        }

        if (10 == pKey)
        {
            std::cout << "\n";
            mConsoleInputBuffer[mConsoleInputBufferIdx] = 0;
            mConsoleInputBufferIdx = 0;
            std::string command = mConsoleInputBuffer;
            auto res = mCmdMan.executeCommand(command);
            consoleLog(command, " : ", res);
            return;
        }

        std::cout << pKey << std::flush;
        mConsoleInputBuffer[mConsoleInputBufferIdx++] = pKey;
    }

    void onInputAvailable()
    {
        char key;
        read(STDIN_FILENO, &key, 1);
        mKeyHandler(key);
    }

    void initTerminal()
    {
        termios term = {0};

        if (tcgetattr(0, &term) < 0)
        {
            throw std::runtime_error(strerror(errno));
        }

        term.c_lflag &= ~ICANON;
        term.c_lflag &= ~ECHO;
        term.c_cc[VMIN] = 1;
        term.c_cc[VTIME] = 0;

        if (tcsetattr(0, TCSANOW, &term) < 0)
        {
            throw std::runtime_error(strerror(errno));
        }

        if (!mReactor.addHandler(STDIN_FILENO, [this](){
                onInputAvailable();
            }))
        {
            throw std::runtime_error("Failed to register STDIN to EpollReactor.");
        }

        consoleLog();
    }

    template<typename... T>
    void consoleLog(T&&... ts)
    {
        std::stringstream ss;
        consoleLog_(ss, ts...);
        std::cout << "\r\033[K" << ss.str() << "\n";
        std::string_view currentCmd(mConsoleInputBuffer, mConsoleInputBufferIdx);
        std::cout << "$user: " << currentCmd << std::flush;
    }

    template<typename... T>
    void consoleLog_(T&&... ts)
    {
        (... << std::forward<T>(ts));
    }

    void consoleLog(const std::string& pStr)
    {
        consoleLog(pStr.c_str());
    }

    char mConsoleInputBuffer[128];
    size_t mConsoleInputBufferIdx = 0;
    int mConsolseWaitChar = 0;

    enum ClientState {IDLE, GM_INIT, GM_ACTIVE, PLAYER};
    ClientState mState = IDLE;

    bfc::CommandManager mCmdMan;

    bfc::EpollReactor mReactor;
    bfc::LightFn<void(char)> mKeyHandler;

    std::byte mBuff[512];
    uint16_t mBuffIdx = 0;
    enum ReadState {WAIT_HEADER, WAIT_REMAINING};
    ReadState mReadState = WAIT_HEADER;
    int mExpectedReadSize = 0;
    int mFd;
    int mEventFd;

    std::optional<GameMaster> mGm;
};

} // namespace tetris

#endif // __TETRISCLIENT_HPP__