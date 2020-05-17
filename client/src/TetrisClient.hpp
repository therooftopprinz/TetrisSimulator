#ifndef __TETRISCLIENT_HPP__
#define __TETRISCLIENT_HPP__

#include <cstring>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <variant>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <termios.h>

#include <bfc/EpollReactor.hpp>
#include <bfc/CommandManager.hpp>

#include <logless/Logger.hpp>

#include <interface/protocol.hpp>

#include <common/StandardTetrisBoard.hpp>

#include <ITetrisClient.hpp>
#include <GameMaster.hpp>
#include <Player.hpp>

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
                return cmdJoin(std::move(pArgs));
            });
        mCmdMan.addCommand("start", [this](bfc::ArgsMap&& pArgs) -> std::string {
                return cmdStart(std::move(pArgs));
            });

        enableConsole();
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

        Logless("TetrisClient: receive: raw=_ decoded=_", BufferLog(mBuffIdx, mBuff), stred.c_str());
        Logger::getInstance().flush();

        std::visit([this](auto& pMsg){
                onMsg(pMsg);
            }, message);
    }

    template <typename T>
    void onMsg(T& pMsg)
    {
        if (GM_ACTIVE == mState)
        {
            mGm->onMsg(pMsg);
        }
        else if(PLAYER_ACTIVE == mState)
        {
            mPlayer->onMsg(pMsg);
        }
        else
        {
            throw std::runtime_error("Unhandled Message");
        }
    }

    void onMsg(CreateGameAccept& pMsg)
    {
        if (GM_INIT != mState)
        {
            throw std::runtime_error("server-client expectation mismatch!");
        }

        mState = GM_ACTIVE;
        consoleLog("[client]: Game created! id=", pMsg.gameId);
    }

    void onMsg(CreateGameReject& pMsg)
    {
        if (GM_INIT != mState)
        {
            throw std::runtime_error("server-client expectation mismatch!");
        }

        mState = IDLE;
        mGm.reset();
    }

    void onMsg(JoinAccept& pMsg)
    {
        if (PLAYER_INIT != mState)
        {
            throw std::runtime_error("server-client expectation mismatch!");
        }

        auto id = pMsg.playerId;

        mState = PLAYER_ACTIVE;
        consoleLog("[client]: Player joined id=", unsigned(id), "!");

        TetrisBoardConfig config{};
        config.height = pMsg.boardHeight;
        config.width = pMsg.boardWidth;

        mPlayer.emplace(id, (ITetrisClient&)*this, config);
    }

    void onMsg(JoinReject& pMsg)
    {
        if (PLAYER_INIT != mState)
        {
            throw std::runtime_error("server-client expectation mismatch!");
        }

        mState = IDLE;
        mPlayer.reset();
    }

    std::string cmdStart(bfc::ArgsMap&&)
    {
        if (GM_ACTIVE != mState)
        {
            return "Can't start game!";
        }

        if (mGm->start())
        {
            return "Started!";
        }
        else
        {
            return "Already started!";
        }
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
        createGameRequest.lockingTimeoutMs = 1000;
        createGameRequest.attackMode = SequentialTargeting{};
        auto& attackMode = std::get<SequentialTargeting>(createGameRequest.attackMode);
        attackMode.targetChangeTimeoutMs = 2000;
        createGameRequest.attackModeCommon.attackDelayMs = 0;
        createGameRequest.attackModeCommon.counteringType = CounteringType::FULL;

        auto width = pArgs.argAs<int>("width");
        auto height = pArgs.argAs<int>("height");
        auto lockTimeout = pArgs.argAs<int>("lock");

        if (width)
        {
            createGameRequest.boardWidth = *width;
        }

        if (height)
        {
            createGameRequest.boardHeight = *height;
        }

        if (lockTimeout)
        {
            createGameRequest.lockingTimeoutMs = *lockTimeout;
        }

        send(message);
        mState = GM_INIT;

        mGm.emplace((ITetrisClient&)*this);

        return "Creating game...";
    }

    std::string cmdJoin(bfc::ArgsMap&& pArgs)
    {
        if (IDLE != mState)
        {
            return "Can't join game!";
        }

        auto id = pArgs.argAs<int>("id");
        if (!id)
        {
            return "id not provided.";
        }

        TetrisProtocol message;
        message = JoinRequest();
        auto& joinRequest = std::get<JoinRequest>(message);
        joinRequest.gameId = *id;
        send(message);

        mState = PLAYER_INIT;
        return "Joining...";
    }

    void send(TetrisProtocol& pMessage)
    {
        std::byte buffer[512];
        auto& msgSize = *(new (buffer) uint16_t(0));
        cum::per_codec_ctx context(buffer+sizeof(msgSize), sizeof(buffer)-sizeof(msgSize));
        encode_per(pMessage, context);

        msgSize = sizeof(buffer)-context.size()-2;

        std::string stred;
        str("root", pMessage, stred, true);
        Logless("TetrisClient: send: raw=_ encoded=_", BufferLog(msgSize, buffer), stred.c_str());
        Logger::getInstance().flush();

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
        if (!mEnableConsole)
        {
            return;
        }

        std::stringstream ss;
        consoleLog_(ss, ts...);
        std::cout << "\r\033[K" << ss.str() << "\n";
        std::string_view currentCmd(mConsoleInputBuffer, mConsoleInputBufferIdx);
        std::cout << "$user: " << currentCmd << std::flush;
    }

    void disableConsole()
    {
        mEnableConsole = false;
    }

    void setKeyHandler(bfc::LightFn<void(char)> pHandler)
    {
        mKeyHandler = std::move(pHandler);
    }

    void enableConsole()
    {
        setKeyHandler([this](char key) -> void {
                consoleIn(key);
            });

        mEnableConsole = true;
        consoleLog("[console]: enabling...");
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

    bool mEnableConsole = true;
    char mConsoleInputBuffer[128];
    size_t mConsoleInputBufferIdx = 0;
    int mConsolseWaitChar = 0;

    enum ClientState {IDLE, GM_INIT, GM_ACTIVE, PLAYER_INIT, PLAYER_ACTIVE};
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
    std::optional<Player> mPlayer;
};

} // namespace tetris

#endif // __TETRISCLIENT_HPP__