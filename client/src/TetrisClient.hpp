#ifndef __TETRISCLIENT_HPP__
#define __TETRISCLIENT_HPP__

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string_view>
#include <variant>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>

#include <memory>
#include <optional>

#include "NcursesUi.hpp"

#include <bfc/epoll_reactor.hpp>
#include <bfc/command_manager.hpp>

#include <tetris_log.hpp>

#include <interface/protocol_export.hpp>

#include <common/StandardTetrisBoard.hpp>

#include <ITetrisClient.hpp>
#include <GameMaster.hpp>
#include <Player.hpp>

namespace tetris
{

namespace detail
{

inline int g_clientShutdownPipeWrite = -1;

inline void clientShutdownSignalHandler(int)
{
    unsigned char b = 1;
    if (g_clientShutdownPipeWrite >= 0)
    {
        (void)::write(g_clientShutdownPipeWrite, &b, 1);
    }
}

} // namespace detail

struct TetrisClientConfig
{
    enum Operation {CREATE, JOIN};
    Operation operation;
    uint32_t operand;
    uint32_t ip;
    uint16_t port;
    std::string username;
    std::optional<std::string> cmd;
};

class TetrisClient : public ITetrisClient
{
    friend class NcursesUi;

public:
    TetrisClient(const TetrisClientConfig& pConfig)
        : mClientName(pConfig.username)
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

        performBlockingLogin(pConfig.username);

        if (!mReactor.add_read_rdy(mFd, [this](){
                onClientReadAvailable();
            }))
        {
            throw std::runtime_error("Failed to add client socket to EpollReactor!");
        }

        initTerminal();

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
        mCmdMan.add("/start", [this](bfc::args_map&& pArgs) -> std::string {
                return cmdStart(std::move(pArgs));
            });
        mCmdMan.add("/leave", [this](bfc::args_map&&) -> std::string {
                return cmdLeave();
            });

        enableConsole();
        consoleLog("[client] ", interactiveHelpText());

        if (pConfig.cmd)
        {
            auto cmd = *pConfig.cmd + "\n"; 
            for (auto i : cmd)
            {
                consoleIn(i);
            }
        }

        installShutdownSignals();
    }

    void run()
    {
        mReactor.run();
    }

    ~TetrisClient() override
    {
        if (mShutdownPipe[0] != -1)
        {
            struct sigaction saDefault{};
            saDefault.sa_handler = SIG_DFL;
            sigaction(SIGINT, &saDefault, nullptr);
            sigaction(SIGTERM, &saDefault, nullptr);
            if (detail::g_clientShutdownPipeWrite == mShutdownPipe[1])
            {
                detail::g_clientShutdownPipeWrite = -1;
            }
            ::close(mShutdownPipe[0]);
            ::close(mShutdownPipe[1]);
            mShutdownPipe[0] = mShutdownPipe[1] = -1;
        }
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

    bool gameplayKeyRoutesToConsole(char key) const override
    {
        return (mConsoleInputBufferIdx > 0 && mConsoleInputBuffer[0] == '/')
            || key == '/';
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
        if (-1 == mFd || !mLobbyGameId.has_value())
        {
            return;
        }
        TetrisProtocol message = LeaveIndication{};
        std::get<LeaveIndication>(message).gameId = *mLobbyGameId;
        send(message);
    }

private:
    void installShutdownSignals()
    {
        if (mShutdownPipe[0] != -1)
        {
            return;
        }
        if (pipe2(mShutdownPipe, O_CLOEXEC) != 0)
        {
            return;
        }
        detail::g_clientShutdownPipeWrite = mShutdownPipe[1];
        if (!mReactor.add_read_rdy(mShutdownPipe[0], [this]() {
                onShutdownPipeReadable();
            }))
        {
            detail::g_clientShutdownPipeWrite = -1;
            ::close(mShutdownPipe[0]);
            ::close(mShutdownPipe[1]);
            mShutdownPipe[0] = mShutdownPipe[1] = -1;
            return;
        }
        struct sigaction sa{};
        sa.sa_handler = detail::clientShutdownSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);
    }

    void onShutdownPipeReadable()
    {
        char drain[32];
        (void)::read(mShutdownPipe[0], drain, sizeof drain);
        try
        {
            sendLeaveIndication();
        }
        catch (...)
        {
        }
        if (-1 != mFd)
        {
            (void)::shutdown(mFd, SHUT_RDWR);
        }
        mReactor.stop();
    }

    void applyLobbyExitIfPending(bool force = false)
    {
        if (!force && !mPendingLobbyExit)
        {
            return;
        }
        mPendingLobbyExit = false;
        mLobbyGameId.reset();
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
            readSize = 2 - static_cast<int>(mBuffIdx);
        }
        else
        {
            readSize = mExpectedReadSize - static_cast<int>(mBuffIdx);
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
            consoleLog("[client] server disconnected.");
            mReactor.stop();
            return;
        }

        mBuffIdx += static_cast<uint16_t>(res);

        if (WAIT_HEADER == mReadState)
        {
            if (mBuffIdx < 2)
            {
                return;
            }
            std::memcpy(&mExpectedReadSize, mBuff, 2);
            mBuffIdx = 0;
            if (mExpectedReadSize <= 0 || mExpectedReadSize > static_cast<int>(sizeof(mBuff)))
            {
                if (-1 != mFd)
                {
                    mReactor.rem_read_rdy(mFd);
                    close(mFd);
                    mFd = -1;
                }
                applyLobbyExitIfPending(true);
                consoleLog("[client] invalid frame from server.");
                mReactor.stop();
                return;
            }
            mReadState = WAIT_REMAINING;
            return;
        }

        if (mExpectedReadSize == static_cast<int>(mBuffIdx))
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
        std::string who = pMsg.username;
        if (who.empty())
        {
            who = "?";
        }
        consoleLog("[chat] ", who, ": ", pMsg.message);
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
            // Late or duplicate server messages (e.g. after /leave before socket drain) — safe to ignore.
            (void)pMsg;
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
        consoleLog("[client] Game created! id=", pMsg.gameId);
    }

    void onMsg(CreateGameReject&& pMsg)
    {
        if (GM_INIT != mState)
        {
            throw std::runtime_error("server-client expectation mismatch!");
        }

        mState = IDLE;
        mLobbyGameId.reset();
        mGm.reset();
    }

    void onMsg(JoinAccept&& pMsg)
    {
        if (PLAYER_INIT != mState)
        {
            throw std::runtime_error("server-client expectation mismatch!");
        }

        mState = PLAYER_ACTIVE;
        consoleLog("[client] Player joined id=", unsigned(pMsg.player), "!");

        mPlayer.emplace((ITetrisClient&)*this,  std::move(pMsg));
    }

    void onMsg(JoinReject&& pMsg)
    {
        if (PLAYER_INIT != mState)
        {
            throw std::runtime_error("server-client expectation mismatch!");
        }

        mState = IDLE;
        mLobbyGameId.reset();
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

    static std::string_view trimView(std::string_view sv)
    {
        auto b = std::find_if(sv.begin(), sv.end(), [](unsigned char c) { return !std::isspace(c); });
        auto e = std::find_if(sv.rbegin(), sv.rend(), [](unsigned char c) { return !std::isspace(c); }).base();
        if (b >= e)
        {
            return {};
        }
        return std::string_view(&*b, static_cast<size_t>(e - b));
    }

    static std::string firstToken(std::string_view sv)
    {
        sv = trimView(sv);
        if (sv.empty())
        {
            return {};
        }
        auto sp = std::find_if(sv.begin(), sv.end(), [](unsigned char c) { return std::isspace(c); });
        return std::string(sv.begin(), sp);
    }

    static std::string toLowerAscii(std::string s)
    {
        for (char& c : s)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return s;
    }

    static std::string normalizeHelpTopic(std::string topic)
    {
        topic = toLowerAscii(std::move(topic));
        if (!topic.empty() && topic.front() == '/')
        {
            topic.erase(0, 1);
        }
        return topic;
    }

    /** One-line list for startup and bare /help. */
    static std::string interactiveHelpText()
    {
        std::ostringstream o;
        o << "Interactive commands start with /. Plain text (no leading /) sends chat to all connections.\n"
          << "Arguments are key=value, space-separated. Use /help <command> for full syntax.\n\n"
          << "  /help [command] - List commands, or detailed help for one command.\n"
          << "  /exit - Stop the client and exit the process.\n"
          << "  /create - Request a new game (game master); optional width, height, lock, board, attack, ...\n"
          << "  /join - Join a game by id=<gameId> (idle only).\n"
          << "  /start - Start the match (GM: first time after /create, or /start again for another round in the same room).\n"
          << "  /leave - Leave the game (disconnect from the room); use before /create if you want a new game id.\n";
        return o.str();
    }

    static std::string helpDetailForTopic(const std::string& topicIn)
    {
        const std::string t = normalizeHelpTopic(topicIn);
        if (t.empty() || t == "help")
        {
            return std::string(
                "/help\n"
                "    Show a short list of commands with one-line descriptions.\n"
                "/help <command>\n"
                "    Full documentation for one command. Commands: help, exit, create, join, start, leave.\n"
                "    Example: /help create\n");
        }
        if (t == "exit")
        {
            return std::string(
                "/exit\n"
                "    Stop the client reactor and exit the process.\n");
        }
        if (t == "create")
        {
            return std::string(
                "/create [width=<n>] [height=<n>] [lock=<ms>] [clear=<ms>] [board=<type>]\n"
                "        [target=<ms>] [attackDelay=<ms>] [attack=<mode>] [counter=<type>]\n"
                "        [seed=<n>] (seed only with attack=random)\n"
                "    Request a new game (become game master). Only from idle (use /leave to leave a room first).\n"
                "    width        Board width in cells (default 10).\n"
                "    height       Board height in cells (default 24).\n"
                "    lock         Piece lock timeout in milliseconds (default 1000).\n"
                "    clear        Line clear timeout in ms (default 0).\n"
                "    board        SRS, ARS, or CULTRIS (default SRS).\n"
                "    target       Attack target-change timeout in ms (default 2000).\n"
                "    attackDelay  Delay before outgoing attacks in ms (default 0).\n"
                "    attack       SEQUENTIAL, DIVIDE, TO_ALL, TO_MOST, TO_SELF, ROULETTE,\n"
                "                 or RANDOM (optional seed=<n>).\n"
                "    counter      Countering: FULL, LIMITED, or INSTANT (default FULL).\n"
                "    Example: /create attack=divide counter=limited attackDelay=100 target=3000\n");
        }
        if (t == "join")
        {
            return std::string(
                "/join id=<gameId>\n"
                "    Join an existing game as a player. id is required. Only from idle.\n"
                "    Your login name (--username=) is used as the player name.\n"
                "    Example: /join id=1\n");
        }
        if (t == "start")
        {
            return std::string(
                "/start\n"
                "    Tell the server to start the match (game master only).\n"
                "    Use once after /create for the first round; after a match ends, /start again for a rematch in the same room.\n"
                "    Use /leave first if you want to leave the room or call /create for a new game (requires idle).\n");
        }
        if (t == "leave")
        {
            return std::string(
                "/leave\n"
                "    Leave the current room at any time (waiting room, during a match, or after a match ends).\n"
                "    After a match ends, also dismisses the post-game screen (chat + frozen boards).\n");
        }
        return "Unknown command `" + topicIn + "`. Type /help for a list; topics: help, exit, create, join, start, leave.\n";
    }

    std::string cmdStart(bfc::args_map&&)
    {
        if (GM_ACTIVE != mState)
        {
            return "Can't start game!";
        }
        if (!mGm)
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

    std::string cmdLeave()
    {
        const bool inRoom = (GM_INIT == mState || GM_ACTIVE == mState
            || PLAYER_INIT == mState || PLAYER_ACTIVE == mState);
        if (!inRoom)
        {
            return "Not in a game. /leave only applies while you are in a room (after /create or /join).";
        }
        mMatchEndedAwaitingLobby = false;
        if (inRoom)
        {
            sendLeaveIndication();
        }
        requestExitToLobby();
        applyLobbyExitIfPending();
        return "Leaving game...";
    }

    std::string cmdCreate(bfc::args_map&& pArgs)
    {
        if (IDLE != mState)
        {
            if (mMatchEndedAwaitingLobby
                && (GM_ACTIVE == mState || PLAYER_ACTIVE == mState || GM_INIT == mState || PLAYER_INIT == mState))
            {
                return "Match ended. Use /leave first; then /create a new game. /start is only for the first start after /create.";
            }
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

    static void readFdFully(int pFd, void* pBuf, size_t pLen)
    {
        auto* p = static_cast<char*>(pBuf);
        size_t got = 0;
        while (got < pLen)
        {
            const ssize_t n = ::read(pFd, p + got, pLen - got);
            if (n < 0)
            {
                throw std::runtime_error(strerror(errno));
            }
            if (n == 0)
            {
                throw std::runtime_error("connection closed while reading");
            }
            got += static_cast<size_t>(n);
        }
    }

    void performBlockingLogin(const std::string& pUsername)
    {
        TetrisProtocol out = LoginRequest{};
        std::get<LoginRequest>(out).username = pUsername;
        send(out);

        uint16_t payloadSz = 0;
        readFdFully(mFd, &payloadSz, sizeof(payloadSz));
        std::byte body[512];
        if (payloadSz > sizeof(body))
        {
            std::cout << "Login failed: invalid response from server.\n" << std::flush;
            std::exit(1);
        }
        readFdFully(mFd, body, payloadSz);

        TetrisProtocol response;
        cum::per_codec_ctx ctx(body, payloadSz);
        decode_per(response, ctx);

        if (!std::holds_alternative<LoginResponse>(response))
        {
            std::cout << "Login failed: unexpected response from server.\n" << std::flush;
            std::exit(1);
        }
        const LoginResult r = std::get<LoginResponse>(response).result;
        if (r == LoginResult::ALREADY_EXIST)
        {
            std::cout << "Login failed: username already in use.\n" << std::flush;
            std::exit(1);
        }
        if (r != LoginResult::OK)
        {
            std::cout << "Login failed.\n" << std::flush;
            std::exit(1);
        }
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

        auto res = ::send(mFd, buffer, msgSize+2, MSG_NOSIGNAL);
        if (-1 == res)
        {
            if (errno == EPIPE || errno == ECONNRESET || errno == ENOTCONN)
            {
                mReactor.stop();
                return;
            }
            throw std::runtime_error(strerror(errno));
        }
    }

    void consoleIn(char pKey) override
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
            if (mLobbyGameId.has_value()
                && (GM_ACTIVE == mState || PLAYER_ACTIVE == mState || PLAYER_INIT == mState))
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
                mNcurses->syncInputLine(mConsoleInputBuffer, mConsoleInputBufferIdx, mGameplayChatCompose);
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
                chat.message = command;
                send(message);
                consoleLog("[chat] you: ", command);
                return;
            }
            if (command.rfind("/help", 0) == 0)
            {
                std::string_view rest{command};
                rest.remove_prefix(5);
                rest = trimView(rest);
                if (rest.empty())
                {
                    consoleLog("[client] ", interactiveHelpText());
                    return;
                }
                consoleLog("[client] ", helpDetailForTopic(firstToken(rest)));
                return;
            }
            auto res = mCmdMan.execute(command);
            consoleLog("[client] ", res);
            return;
        }

        if (mNcurses)
        {
            mConsoleInputBuffer[mConsoleInputBufferIdx++] = pKey;
            mNcurses->syncInputLine(mConsoleInputBuffer, mConsoleInputBufferIdx, mGameplayChatCompose);
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
                std::size_t i = 0;
                while (i < out.size())
                {
                    const std::size_t j = out.find('\n', i);
                    if (j == std::string::npos)
                    {
                        std::string line = out.substr(i);
                        while (!line.empty() && line.back() == '\r')
                        {
                            line.pop_back();
                        }
                        mNcurses->appendLog(std::move(line));
                        break;
                    }
                    std::string line = out.substr(i, j - i);
                    while (!line.empty() && line.back() == '\r')
                    {
                        line.pop_back();
                    }
                    mNcurses->appendLog(std::move(line));
                    i = j + 1;
                }
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
            consoleLog("[client] Ready.");
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
    std::optional<uint64_t> mLobbyGameId;

    bfc::command_manager<> mCmdMan;

    bfc::epoll_reactor<> mReactor;
    bfc::light_function<void(char)> mKeyHandler;

    int mShutdownPipe[2] = {-1, -1};

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