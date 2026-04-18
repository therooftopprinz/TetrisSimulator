#ifndef __TETRISCLIENT_HPP__
#define __TETRISCLIENT_HPP__

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <random>
#include <sstream>
#include <string_view>
#include <variant>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <memory>

#include "NcursesUi.hpp"

#include <bfc/epoll_reactor.hpp>
#include <bfc/command_manager.hpp>

#include <tetris_log.hpp>

#include <interface/protocol.hpp>

#include <common/StandardTetrisBoard.hpp>

#include <ITetrisClient.hpp>
#include <GameMaster.hpp>
#include <Player.hpp>

namespace tetris
{

inline std::string generateRandomDisplayName()
{
    static constexpr char kVowels[] = "aeiou";
    static constexpr char kConsonants[] = "bcdfghjklmnpqrstvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    const int len = 4 + static_cast<int>(gen() % 2);
    const bool vowelFirst = (gen() % 2) == 0;
    std::uniform_int_distribution<int> vDist(0, 4);
    std::uniform_int_distribution<int> cDist(0, int(sizeof(kConsonants) - 2));
    std::string out;
    out.reserve(static_cast<size_t>(len));
    for (int i = 0; i < len; ++i)
    {
        const bool v = (i % 2 == 0) ? vowelFirst : !vowelFirst;
        out += v ? kVowels[vDist(gen)] : kConsonants[cDist(gen)];
    }
    return out;
}

struct TetrisClientConfig
{
    enum Operation {CREATE, JOIN};
    Operation operation;
    uint32_t operand;
    uint32_t ip;
    uint16_t port;
    std::optional<std::string> cmd;
};

class TetrisClient : public ITetrisClient
{
    friend class NcursesUi;

public:
    TetrisClient(const TetrisClientConfig& pConfig)
        : mClientName(generateRandomDisplayName())
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
        logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
            "TetrisClient: connecting to %s; port=%hu;", loc, pConfig.port);

        auto res = connect(mFd, (sockaddr*)&server, sizeof(server));

        if (-1 == res)
        {
            throw std::runtime_error(strerror(errno));
        }

        if (!mReactor.add_read_rdy(mFd, [this](){
                onClientReadAvailable();
            }))
        {
            throw std::runtime_error("Failed to add client socket to EpollReactor!");
        }

        initTerminal();

        mCmdMan.add("/help", [this](bfc::args_map&& pArgs) -> std::string {
                return cmdHelp(std::move(pArgs));
            });
        mCmdMan.add("/exit", [this](bfc::args_map&&) -> std::string {
                mReactor.stop();
                return "exiting...";
            });
        mCmdMan.add("/create", [this](bfc::args_map&& pArgs) -> std::string {
                return cmdCreate(std::move(pArgs));
            });
        mCmdMan.add("/join", [this](bfc::args_map&& pArgs) -> std::string {
                return cmdJoin(std::move(pArgs));
            });
        mCmdMan.add("/name", [this](bfc::args_map&& pArgs) -> std::string {
                return cmdName(std::move(pArgs));
            });
        mCmdMan.add("/start", [this](bfc::args_map&& pArgs) -> std::string {
                return cmdStart(std::move(pArgs));
            });
        mCmdMan.add("/lobby", [this](bfc::args_map&&) -> std::string {
                return cmdLobby();
            });

        enableConsole();
        pushDisplayNameToServer();
        
        if (pConfig.cmd)
        {
            auto cmd = *pConfig.cmd + "\n"; 
            for (auto i : cmd)
            {
                consoleIn(i);
            }
        }
    }

    void run()
    {
        mReactor.run();
    }

    ~TetrisClient() override
    {
        if (mNcurses)
        {
            mNcurses->shutdown();
        }
    }

    void paintGameView() override
    {
        if (mNcurses)
        {
            mNcurses->fullRedraw(*this);
        }
    }

    void notifyMatchEnded() override
    {
        mMatchEndedAwaitingLobby = true;
        enableConsoleInternal(false);
        if (mNcurses)
        {
            mNcurses->fullRedraw(*this);
        }
    }

    void notifyMatchStarted() override
    {
        mMatchEndedAwaitingLobby = false;
        if (mPlayer)
        {
            mPlayer->bindGameplayInput();
        }
        if (mNcurses)
        {
            mNcurses->fullRedraw(*this);
        }
    }

    bool tryHandleGameplayChat(char key) override
    {
        if (!mNcurses || !mPlayer || mMatchEndedAwaitingLobby)
        {
            return false;
        }
        if (!mPlayer->isGameStarted())
        {
            consoleIn(key);
            return true;
        }
        if (key == '\t')
        {
            mGameplayChatCompose = !mGameplayChatCompose;
            mNcurses->fullRedraw(*this);
            return true;
        }
        if (mGameplayChatCompose)
        {
            consoleIn(key);
            return true;
        }
        if (key == '\n' || key == '\r')
        {
            mGameplayChatCompose = true;
            mNcurses->fullRedraw(*this);
            return true;
        }
        return false;
    }

    bool gameplayChatCompose() const noexcept
    {
        return mGameplayChatCompose;
    }

    void requestExitToLobby() override
    {
        mPendingLobbyExit = true;
    }

    void sendLeaveIndication() override
    {
        if (-1 == mFd || 0 == mLobbyGameId)
        {
            return;
        }
        TetrisProtocol message = LeaveIndication{};
        std::get<LeaveIndication>(message).gameId = mLobbyGameId;
        send(message);
    }

private:
    void applyLobbyExitIfPending(bool force = false)
    {
        if (!force && !mPendingLobbyExit)
        {
            return;
        }
        mPendingLobbyExit = false;
        mLobbyGameId = 0;
        mMatchEndedAwaitingLobby = false;
        mPlayer.reset();
        mGm.reset();
        if (GM_INIT == mState || GM_ACTIVE == mState || PLAYER_INIT == mState || PLAYER_ACTIVE == mState)
        {
            mState = IDLE;
        }
        enableConsoleInternal(false);
    }

    void onClientReadAvailable()
    {
        if (mNcurses && mNcurses->pollResize())
        {
            mNcurses->fullRedraw(*this);
        }

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
            if (-1 != mFd)
            {
                mReactor.rem_read_rdy(mFd);
                close(mFd);
                mFd = -1;
            }
            applyLobbyExitIfPending(true);
            consoleLog("[client]: server disconnected.");
            mReactor.stop();
            return;
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

        logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
            "TetrisClient: receive: raw=%%; decoded=%s;", BufferLog(mBuffIdx, mBuff), stred.c_str());
        tetris_logger().flush();

        std::visit([this](auto& pMsg){
                onMsg(std::move(pMsg));
            }, message);
        applyLobbyExitIfPending();
        if (mNcurses)
        {
            mNcurses->fullRedraw(*this);
        }
    }

    void onMsg(ClientChat&& pMsg)
    {
        consoleLog("[chat] ", pMsg.name, ": ", pMsg.message);
    }

    template <typename T>
    void onMsg(T&& pMsg)
    {
        if (GM_ACTIVE == mState)
        {
            mGm->onMsg(std::move(pMsg));
        }
        else if(PLAYER_ACTIVE == mState)
        {
            mPlayer->onMsg(std::move(pMsg));
        }
        else
        {
            throw std::runtime_error("Unhandled Message");
        }
    }

    void onMsg(CreateGameAccept&& pMsg)
    {
        if (GM_INIT != mState)
        {
            throw std::runtime_error("server-client expectation mismatch!");
        }

        mState = GM_ACTIVE;
        mLobbyGameId = pMsg.gameId;
        consoleLog("[client]: Game created! id=", pMsg.gameId);
    }

    void onMsg(CreateGameReject&& pMsg)
    {
        if (GM_INIT != mState)
        {
            throw std::runtime_error("server-client expectation mismatch!");
        }

        mState = IDLE;
        mLobbyGameId = 0;
        mGm.reset();
    }

    void onMsg(JoinAccept&& pMsg)
    {
        if (PLAYER_INIT != mState)
        {
            throw std::runtime_error("server-client expectation mismatch!");
        }

        mState = PLAYER_ACTIVE;
        consoleLog("[client]: Player joined id=", unsigned(pMsg.player), "!");

        mPlayer.emplace((ITetrisClient&)*this,  std::move(pMsg));
    }

    void onMsg(JoinReject&& pMsg)
    {
        if (PLAYER_INIT != mState)
        {
            throw std::runtime_error("server-client expectation mismatch!");
        }

        mState = IDLE;
        mLobbyGameId = 0;
        mPlayer.reset();
    }

    static std::string toAsciiUpper(std::string s)
    {
        for (char& ch : s)
        {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        return s;
    }

    static std::string parseCreateAttackMode(const bfc::args_map& pArgs, CreateGameRequest& req)
    {
        auto raw = pArgs.arg("attack");
        if (!raw)
        {
            return {};
        }
        const std::string u = toAsciiUpper(*raw);
        if (u == "RANDOM")
        {
            Random r{};
            r.seed = pArgs.as<std::uint64_t>("seed").value_or(0);
            req.attackMode = r;
            return {};
        }
        AttackModeEnum e{};
        if (u == "SEQUENTIAL") { e = AttackModeEnum::SEQUENTIAL; }
        else if (u == "DIVIDE") { e = AttackModeEnum::DIVIDE; }
        else if (u == "TO_ALL") { e = AttackModeEnum::TO_ALL; }
        else if (u == "TO_MOST") { e = AttackModeEnum::TO_MOST; }
        else if (u == "TO_SELF") { e = AttackModeEnum::TO_SELF; }
        else if (u == "ROULETTE") { e = AttackModeEnum::ROULETTE; }
        else
        {
            return std::string("Unknown attack mode `") + *raw
                + "`. Use SEQUENTIAL, DIVIDE, TO_ALL, TO_MOST, TO_SELF, ROULETTE, or RANDOM (with optional seed=).";
        }
        req.attackMode = e;
        return {};
    }

    static std::string parseCreateCountering(const bfc::args_map& pArgs, CreateGameRequest& req)
    {
        auto raw = pArgs.arg("counter");
        if (!raw)
        {
            return {};
        }
        const std::string u = toAsciiUpper(*raw);
        if (u == "FULL") { req.attackModeCommon.counteringType = CounteringType::FULL; }
        else if (u == "LIMITED") { req.attackModeCommon.counteringType = CounteringType::LIMITED; }
        else if (u == "INSTANT") { req.attackModeCommon.counteringType = CounteringType::INSTANT; }
        else
        {
            return std::string("Unknown countering type `") + *raw + "`. Use FULL, LIMITED, or INSTANT.";
        }
        return {};
    }

    static std::string parseCreateBoardType(const bfc::args_map& pArgs, CreateGameRequest& req)
    {
        auto raw = pArgs.arg("board");
        if (!raw)
        {
            return {};
        }
        const std::string u = toAsciiUpper(*raw);
        if (u == "SRS") { req.boardType = BoardType::SRS; }
        else if (u == "ARS") { req.boardType = BoardType::ARS; }
        else if (u == "CULTRIS") { req.boardType = BoardType::CULTRIS; }
        else
        {
            return std::string("Unknown board type `") + *raw + "`. Use SRS, ARS, or CULTRIS.";
        }
        return {};
    }

    static std::string checkU16(const char* name, int v)
    {
        if (v < 0 || v > 65535)
        {
            return std::string(name) + " out of range (0..65535).";
        }
        return {};
    }

    std::string cmdHelp(bfc::args_map&&)
    {
        std::ostringstream o;
        o << "Interactive commands start with /. Plain text (no leading /) sends chat to all connections.\n"
          << "Arguments are key=value, space-separated:\n\n"
          << "  /help\n"
          << "      Show this reference.\n\n"
          << "  /exit\n"
          << "      Stop the client and exit the process.\n\n"
          << "  /name name=<name>\n"
          << "      Set display name (alphanumeric, max 32). Sent to the server for join and chat.\n\n"
          << "  /create [width=<n>] [height=<n>] [lock=<ms>] [clear=<ms>] [board=<type>]\n"
          << "          [target=<ms>] [attackDelay=<ms>] [attack=<mode>] [counter=<type>]\n"
          << "          [seed=<n>] (seed only with attack=random)\n"
          << "      Request a new game (become game master). Only from idle state.\n"
          << "      width        Board width in cells (default 10).\n"
          << "      height       Board height in cells (default 24).\n"
          << "      lock         Piece lock timeout in milliseconds (default 1000).\n"
          << "      clear        Line clear timeout in ms (default 0).\n"
          << "      board        SRS, ARS, or CULTRIS (default SRS).\n"
          << "      target       Attack target-change timeout in ms (default 2000).\n"
          << "      attackDelay  Delay before outgoing attacks in ms (default 0).\n"
          << "      attack       SEQUENTIAL, DIVIDE, TO_ALL, TO_MOST, TO_SELF, ROULETTE,\n"
          << "                   or RANDOM (protocol Random seed; optional seed=).\n"
          << "      counter      Countering: FULL, LIMITED, or INSTANT (default FULL).\n"
          << "      Example: /create attack=divide counter=limited attackDelay=100 target=3000\n\n"
          << "  /join id=<gameId>\n"
          << "      Join an existing game as a player. id is required. Only from idle.\n"
          << "      Your current display name (random on startup, see /name) is used as the player name.\n"
          << "      Example: /join id=1\n\n"
          << "  /start\n"
          << "      Tell the server to start the match (game master only, after create is accepted).\n\n"
          << "  /lobby\n"
          << "      After the match ends, leave the post-game screen (chat + frozen boards) and return to the lobby.\n";
        return o.str();
    }

    std::string cmdStart(bfc::args_map&&)
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

    std::string cmdLobby()
    {
        if (!mMatchEndedAwaitingLobby)
        {
            return "Nothing to dismiss. /lobby is only used after a match ends to return to the lobby.";
        }
        mMatchEndedAwaitingLobby = false;
        requestExitToLobby();
        return "Returning to lobby...";
    }

    std::string cmdCreate(bfc::args_map&& pArgs)
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
        createGameRequest.clearTimeoutMs = 0;
        createGameRequest.boardType = BoardType::SRS;
        createGameRequest.attackMode = AttackModeEnum::SEQUENTIAL;
        createGameRequest.attackModeCommon.targetChangeTimeoutMs = 2000;
        createGameRequest.attackModeCommon.attackDelayMs = 0;
        createGameRequest.attackModeCommon.counteringType = CounteringType::FULL;

        auto width = pArgs.as<int>("width");
        auto height = pArgs.as<int>("height");
        auto lockTimeout = pArgs.as<int>("lock");
        auto clearTimeout = pArgs.as<int>("clear");
        auto targetTimeout = pArgs.as<int>("target");
        auto attackDelay = pArgs.as<int>("attackDelay");

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

        if (clearTimeout)
        {
            if (auto err = checkU16("clear", *clearTimeout); !err.empty())
            {
                return err;
            }
            createGameRequest.clearTimeoutMs = static_cast<uint16_t>(*clearTimeout);
        }

        if (targetTimeout)
        {
            if (auto err = checkU16("target", *targetTimeout); !err.empty())
            {
                return err;
            }
            createGameRequest.attackModeCommon.targetChangeTimeoutMs = static_cast<uint16_t>(*targetTimeout);
        }

        if (attackDelay)
        {
            if (auto err = checkU16("attackDelay", *attackDelay); !err.empty())
            {
                return err;
            }
            createGameRequest.attackModeCommon.attackDelayMs = static_cast<uint16_t>(*attackDelay);
        }

        if (auto err = parseCreateAttackMode(pArgs, createGameRequest); !err.empty())
        {
            return err;
        }
        if (auto err = parseCreateCountering(pArgs, createGameRequest); !err.empty())
        {
            return err;
        }
        if (auto err = parseCreateBoardType(pArgs, createGameRequest); !err.empty())
        {
            return err;
        }

        send(message);
        mState = GM_INIT;

        mGm.emplace((ITetrisClient&)*this);

        return "Creating game...";
    }

    std::string cmdName(bfc::args_map&& pArgs)
    {
        auto n = pArgs.arg("name");
        if (!n || n->empty())
        {
            return "Usage: /name name=<name> (letters and digits only, max 32).";
        }
        if (n->size() > 32)
        {
            return "Name too long (max 32).";
        }
        for (unsigned char ch : *n)
        {
            if (!std::isalnum(ch))
            {
                return "Name must contain only letters and digits.";
            }
        }
        mClientName = *n;
        pushDisplayNameToServer();
        return std::string("Name set to ") + mClientName + ".";
    }

    std::string cmdJoin(bfc::args_map&& pArgs)
    {
        if (IDLE != mState)
        {
            return "Can't join game!";
        }

        auto id = pArgs.as<int>("id");
        if (!id)
        {
            return "id not provided.";
        }

        TetrisProtocol message;
        message = JoinRequest();
        auto& joinRequest = std::get<JoinRequest>(message);
        joinRequest.gameId = *id;
        joinRequest.secret = 0;
        mLobbyGameId = joinRequest.gameId;
        send(message);

        mState = PLAYER_INIT;
        return "Joining...";
    }

    void pushDisplayNameToServer()
    {
        TetrisProtocol message = ClientChat{};
        auto& chat = std::get<ClientChat>(message);
        chat.name = mClientName;
        chat.message.clear();
        send(message);
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
        logless::log(tetris_logger(), logless::DEBUG, logless::LOGALL,
            "TetrisClient: send: raw=%%; encoded=%s;", BufferLog(msgSize, buffer), stred.c_str());
        tetris_logger().flush();

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
        if (3 == static_cast<unsigned char>(pKey))
        {
            if (0 != mLobbyGameId && GM_ACTIVE == mState)
            {
                sendLeaveIndication();
                requestExitToLobby();
            }
            return;
        }
        if (127 == pKey)
        {
            if (0 == mConsoleInputBufferIdx)
            {
                return;
            }

            mConsoleInputBufferIdx--;
            if (mNcurses)
            {
                mNcurses->syncInputLine(mClientName, mConsoleInputBuffer, mConsoleInputBufferIdx, mGameplayChatCompose);
                mNcurses->fullRedraw(*this);
            }
            else
            {
                std::cout << "\b \b" << std::flush;
            }

            return;
        }

        if (mConsoleInputBufferIdx >= sizeof(mConsoleInputBuffer) - 1)
        {
            return;
        }

        // With ncurses nonl(), Enter is often '\r' (13), not '\n' (10).
        if (10 == pKey || 13 == pKey)
        {
            if (!mNcurses)
            {
                std::cout << "\n";
            }
            mConsoleInputBuffer[mConsoleInputBufferIdx] = 0;
            std::string command{mConsoleInputBuffer, mConsoleInputBufferIdx};
            mConsoleInputBufferIdx = 0;
            auto notSpace = [](unsigned char c) { return !std::isspace(c); };
            auto b = std::find_if(command.begin(), command.end(), notSpace);
            auto e = std::find_if(command.rbegin(), command.rend(), notSpace).base();
            if (b == e)
            {
                return;
            }
            command.assign(b, e);
            if (command.front() != '/')
            {
                TetrisProtocol message = ClientChat{};
                auto& chat = std::get<ClientChat>(message);
                chat.name = mClientName;
                chat.message = command;
                send(message);
                consoleLog("[You]: ", command);
                return;
            }
            auto res = mCmdMan.execute(command);
            consoleLog(command, " : ", res);
            return;
        }

        if (mNcurses)
        {
            mConsoleInputBuffer[mConsoleInputBufferIdx++] = pKey;
            mNcurses->syncInputLine(mClientName, mConsoleInputBuffer, mConsoleInputBufferIdx, mGameplayChatCompose);
            mNcurses->fullRedraw(*this);
        }
        else
        {
            std::cout << pKey << std::flush;
            mConsoleInputBuffer[mConsoleInputBufferIdx++] = pKey;
        }
    }

    void onInputAvailable()
    {
        if (mNcurses)
        {
            mNcurses->pumpStdinKeys([this](char key) {
                mKeyHandler(key);
            });
            if (mNcurses->pollResize())
            {
                mNcurses->fullRedraw(*this);
            }
        }
        else
        {
            char key;
            if (read(STDIN_FILENO, &key, 1) != 1)
            {
                return;
            }
            mKeyHandler(key);
        }
        applyLobbyExitIfPending();
        if (mNcurses)
        {
            mNcurses->fullRedraw(*this);
        }
    }

    void initTerminal()
    {
        mNcurses = std::make_unique<NcursesUi>();
        if (!mNcurses->init())
        {
            throw std::runtime_error("Failed to initialize ncurses display.");
        }

        if (!mReactor.add_read_rdy(STDIN_FILENO, [this]() {
                onInputAvailable();
            }))
        {
            throw std::runtime_error("Failed to register STDIN to EpollReactor.");
        }

        mNcurses->fullRedraw(*this);
    }

    template<typename... T>
    void consoleLog(T&&... ts)
    {
        std::stringstream ss;
        consoleLog_(ss, std::forward<T>(ts)...);
        const std::string out = ss.str();
        if (mNcurses)
        {
            if (!out.empty())
            {
                mNcurses->appendLog(out);
            }
            mNcurses->fullRedraw(*this);
            return;
        }
        if (!mEnableConsole)
        {
            return;
        }
        std::cout << "\r\033[K" << out << "\n";
        std::string_view currentCmd(mConsoleInputBuffer, mConsoleInputBufferIdx);
        std::cout << mClientName << " : " << currentCmd << std::flush;
    }

    void disableConsole()
    {
        mEnableConsole = false;
        mGameplayChatCompose = false;
    }

    void setKeyHandler(bfc::light_function<void(char)> pHandler)
    {
        mKeyHandler = std::move(pHandler);
    }

    void enableConsole() override
    {
        enableConsoleInternal(true);
    }

    void enableConsoleInternal(bool announce)
    {
        setKeyHandler([this](char key) -> void {
                consoleIn(key);
            });
        mEnableConsole = true;
        mGameplayChatCompose = false;
        if (announce)
        {
            consoleLog("[console]: enabling...");
        }
    }

    template<typename... T>
    void consoleLog_(std::stringstream& p_ss, T&&... ts)
    {
        if constexpr (sizeof...(T) > 0)
        {
            static_cast<void>((p_ss << ... << std::forward<T>(ts)));
        }
    }

    void consoleLog(const std::string& pStr)
    {
        consoleLog(pStr.c_str());
    }

    bool mEnableConsole = true;
    bool mGameplayChatCompose = false;
    char mConsoleInputBuffer[128];
    size_t mConsoleInputBufferIdx = 0;
    int mConsolseWaitChar = 0;

    enum ClientState {IDLE, GM_INIT, GM_ACTIVE, PLAYER_INIT, PLAYER_ACTIVE};
    ClientState mState = IDLE;
    bool mPendingLobbyExit = false;
    uint64_t mLobbyGameId = 0;

    bfc::command_manager<> mCmdMan;

    bfc::epoll_reactor<> mReactor;
    bfc::light_function<void(char)> mKeyHandler;

    std::byte mBuff[512];
    uint16_t mBuffIdx = 0;
    enum ReadState {WAIT_HEADER, WAIT_REMAINING};
    ReadState mReadState = WAIT_HEADER;
    int mExpectedReadSize = 0;
    int mFd;

    std::optional<GameMaster> mGm;
    std::optional<Player> mPlayer;

    std::string mClientName;

    std::unique_ptr<NcursesUi> mNcurses;
    bool mMatchEndedAwaitingLobby = false;
};

} // namespace tetris

#endif // __TETRISCLIENT_HPP__