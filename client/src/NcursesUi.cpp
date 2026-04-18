#include "NcursesUi.hpp"

#include "TetrisClient.hpp"
#include "Player.hpp"

#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <vector>

#include "ncurses_shim.h"

namespace tetris
{
namespace
{

volatile sig_atomic_t g_resizePending = 0;

inline int termLines()
{
    return getmaxy(stdscr);
}

inline int termCols()
{
    return getmaxx(stdscr);
}

void onSigWinch(int)
{
    g_resizePending = 1;
}

char terminoToChar(Termino t)
{
    static const char* map = "ILJOSZT";
    const int i = static_cast<int>(t);
    if (i < 0 || i >= 7)
    {
        return '?';
    }
    return map[i];
}

enum : short
{
    kPairI = 1,
    kPairL,
    kPairJ,
    kPairO,
    kPairS,
    kPairZ,
    kPairT,
    kPairLocked,
    kPairBorder
};

short terminoColorPair(Termino t)
{
    switch (t)
    {
    case I:
        return kPairI;
    case L:
        return kPairL;
    case J:
        return kPairJ;
    case O:
        return kPairO;
    case S:
        return kPairS;
    case Z:
        return kPairZ;
    case T:
        return kPairT;
    default:
        return kPairLocked;
    }
}

void initTerminoPalette()
{
    static bool done = false;
    if (done || !has_colors())
    {
        return;
    }
    (void)use_default_colors();
    // Tetris-style hues on black (256-color not assumed).
    init_pair(kPairI, COLOR_CYAN, COLOR_BLACK);
    init_pair(kPairL, COLOR_WHITE, COLOR_BLACK);
    init_pair(kPairJ, COLOR_BLUE, COLOR_BLACK);
    init_pair(kPairO, COLOR_YELLOW, COLOR_BLACK);
    init_pair(kPairS, COLOR_GREEN, COLOR_BLACK);
    init_pair(kPairZ, COLOR_RED, COLOR_BLACK);
    init_pair(kPairT, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(kPairLocked, COLOR_WHITE, COLOR_BLACK);
    init_pair(kPairBorder, COLOR_WHITE, COLOR_BLACK);
    done = true;
}

// Plain 7-bit ASCII + COLOR_PAIR in one chtype (no ACS / UTF-8 — works on all curses terminals).
void drawStyledCh(int row, int col, char ch, short colorPair, chtype extraAttr)
{
    if (row < 0 || row >= termLines() || col < 0 || col >= termCols())
    {
        return;
    }
    chtype cell = static_cast<chtype>(static_cast<unsigned char>(ch));
    if (colorPair > 0)
    {
        cell |= COLOR_PAIR(static_cast<unsigned>(colorPair));
    }
    cell |= extraAttr;
    mvaddch(row, col, cell);
}

void drawSeparator(int row, int width)
{
    if (row < 0 || row >= termLines() || width <= 0)
    {
        return;
    }
    std::string line(static_cast<std::size_t>(width), '-');
    mvaddnstr(row, 0, line.c_str(), width);
}

void drawBoardColumn(
    int originY,
    int colX,
    int colW,
    int boardW,
    int boardH,
    const PlayerContext& pc,
    bool isTarget,
    bool isDead)
{
    const int innerH = boardH;
    int y = originY;

    const std::string label = pc.name.empty() ? std::string("?") : pc.name;
    {
        std::string head = label;
        if (static_cast<int>(head.size()) > colW)
        {
            head.resize(static_cast<std::size_t>(std::max(1, colW - 3)));
            head += "...";
        }
        mvaddnstr(y++, colX, head.c_str(), colW);
    }

    char cur = ' ';
    char hold = ' ';
    if (pc.current)
    {
        cur = terminoToChar(*pc.current);
    }
    if (pc.hold)
    {
        hold = terminoToChar(*pc.hold);
    }
    char lineCur[64];
    std::snprintf(lineCur, sizeof(lineCur), "current:%c", cur);
    mvaddnstr(y++, colX, lineCur, colW);
    char lineNext[80];
    {
        static const char kPrefix[] = "next:";
        std::size_t pos = 0;
        const std::size_t prefixLen = sizeof(kPrefix) - 1;
        std::memcpy(lineNext, kPrefix, prefixLen);
        pos = prefixLen;
        for (Termino t : pc.queue)
        {
            if (pos + 1 >= sizeof(lineNext))
            {
                break;
            }
            lineNext[pos++] = terminoToChar(t);
        }
        lineNext[pos] = '\0';
    }
    mvaddnstr(y++, colX, lineNext, colW);
    char lineHold[64];
    std::snprintf(lineHold, sizeof(lineHold), "hold:%c", hold);
    mvaddnstr(y++, colX, lineHold, colW);

    const int topWallRow = y;
    if (y < termLines())
    {
        const short bpair = has_colors() ? kPairBorder : 0;
        int x = colX;
        drawStyledCh(y, x++, '+', bpair, A_NORMAL);
        for (int i = 0; i < boardW; ++i)
        {
            drawStyledCh(y, x++, '-', bpair, A_NORMAL);
        }
        drawStyledCh(y, x, '+', bpair, A_NORMAL);
    }
    ++y;

    for (int row = 0; row < innerH; ++row)
    {
        if (y >= termLines())
        {
            return;
        }
        const short bpair = has_colors() ? kPairBorder : 0;
        drawStyledCh(y, colX, '|', bpair, A_NORMAL);
        for (int c = 0; c < boardW; ++c)
        {
            mvaddch(y, colX + 1 + c, static_cast<chtype>(' '));
        }
        drawStyledCh(y, colX + boardW + 1, '|', bpair, A_NORMAL);
        ++y;
    }

    if (y < termLines())
    {
        const short bpair = has_colors() ? kPairBorder : 0;
        int x = colX;
        drawStyledCh(y, x++, '+', bpair, A_NORMAL);
        for (int i = 0; i < boardW; ++i)
        {
            drawStyledCh(y, x++, '-', bpair, A_NORMAL);
        }
        drawStyledCh(y, x, '+', bpair, A_NORMAL);
    }
    ++y;

    if (y < termLines())
    {
        std::string foot;
        if (isDead)
        {
            foot = "--dead--";
        }
        else if (isTarget)
        {
            foot = "--target--";
        }
        if (!foot.empty())
        {
            const int cx = colX + std::max(0, (colW - static_cast<int>(foot.size())) / 2);
            mvaddnstr(y, cx, foot.c_str(), colW);
        }
    }

    const int innerTop = topWallRow + 1;
    // Game y is 0 at bottom of well; terminal rows grow downward — map game y to screen row.
    const int bh = innerH;
    auto bitmap = pc.bitmap;
    for (unsigned r = 0; r < bitmap.dimension().second; ++r)
    {
        for (unsigned c = 0; c < bitmap.dimension().first; ++c)
        {
            if (!bitmap.get(static_cast<int8_t>(c), static_cast<int8_t>(r)))
            {
                continue;
            }
            const int py = innerTop + (bh - 1) - static_cast<int>(r);
            const int px = colX + 1 + static_cast<int>(c);
            const short pair = has_colors() ? kPairLocked : 0;
            const chtype dim = has_colors() ? A_DIM : A_NORMAL;
            drawStyledCh(py, px, '#', pair, dim);
        }
    }

    if (pc.current && pc.applier && pc.checkerFn)
    {
        const int px0 = colX + 1 + pc.x;
        const int py0 = innerTop + pc.y;
        const Termino curT = *pc.current;
        const short piecePair = has_colors() ? terminoColorPair(curT) : 0;

        const auto paintAt = [innerTop, bh, colX, boardW, &bitmap, piecePair](
                                 CellCoord coord, char glyph, chtype extra, bool skipIfBitmap)
        {
            const int gameY = coord.second - innerTop;
            const int py = innerTop + (bh - 1) - gameY;
            if (py < 0 || py >= termLines() || coord.first < 0 || coord.first >= termCols())
            {
                return;
            }
            if (skipIfBitmap)
            {
                const int bx = coord.first - (colX + 1);
                const int gy = coord.second - innerTop;
                if (bx >= 0 && bx < boardW && gy >= 0 && gy < bh
                    && bitmap.get(static_cast<int8_t>(bx), static_cast<int8_t>(gy)))
                {
                    return;
                }
            }
            drawStyledCh(py, coord.first, glyph, piecePair, extra);
        };

        const int ghosty = innerTop + pc.floor;
        pc.applier(
            px0,
            ghosty,
            [&paintAt](CellCoord coord)
            {
                paintAt(coord, ':', A_DIM, true);
            },
            pc.transformer);

        pc.applier(
            px0,
            py0,
            [&paintAt](CellCoord coord)
            {
                paintAt(coord, '#', A_NORMAL, false);
            },
            pc.transformer);
    }
}

int boardBlockBottomRow(int originY, int boardH)
{
    // name, current, next, hold, then top wall + well + bottom wall
    return originY + 4 + (boardH + 2);
}

void drawChatScroll(int firstRow, int lastRowInclusive, int width, const std::deque<std::string>& log)
{
    if (firstRow > lastRowInclusive || width <= 0)
    {
        return;
    }
    int y = lastRowInclusive;
    for (std::size_t i = 0; y >= firstRow; ++i)
    {
        if (i >= log.size())
        {
            move(y, 0);
            clrtoeol();
            --y;
            continue;
        }
        const std::string& ln = log[log.size() - 1 - i];
        mvaddnstr(y, 0, ln.c_str(), width);
        clrtoeol();
        --y;
    }
}

} // namespace

NcursesUi::~NcursesUi()
{
    shutdown();
}

bool NcursesUi::init()
{
    if (mInitialized)
    {
        return true;
    }
    if (::initscr() == nullptr)
    {
        return false;
    }
    ::cbreak();
    ::noecho();
    ::nonl();
    ::intrflush(stdscr, FALSE);
    ::keypad(stdscr, TRUE);
    ::nodelay(stdscr, TRUE);
    if (::has_colors())
    {
        ::start_color();
        initTerminoPalette();
    }
    ::ESCDELAY = 25;

    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = onSigWinch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, nullptr);

    mInitialized = true;
    return true;
}

void NcursesUi::shutdown()
{
    if (!mInitialized)
    {
        return;
    }
    ::endwin();
    mInitialized = false;
}

void NcursesUi::appendLog(std::string line)
{
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
    {
        line.pop_back();
    }
    mLog.push_back(std::move(line));
    while (mLog.size() > kMaxLogLines)
    {
        mLog.pop_front();
    }
}

void NcursesUi::syncInputLine(const std::string& clientName, const char* inputBuf, std::size_t inputLen, bool chatCompose)
{
    if (!mInitialized || termLines() < 1)
    {
        return;
    }
    const int row = termLines() - 1;
    const int w = termCols() > 0 ? termCols() : 80;
    const int maxCols = std::max(1, w - 1);
    const std::string prefix = chatCompose ? "*chat: " : "chat: ";
    const std::string sep = " : ";

    std::string name = clientName;
    const int minInputCols = std::min(24, std::max(8, maxCols / 3));
    int budget = maxCols - minInputCols - static_cast<int>(prefix.size() + sep.size());
    if (budget < 4)
    {
        budget = std::max(1, maxCols / 4);
    }
    if (static_cast<int>(name.size()) > budget)
    {
        const std::size_t keep = static_cast<std::size_t>(std::max(1, budget - 3));
        name.assign(name.data(), std::min(name.size(), keep));
        name += "...";
    }

    std::string prompt = prefix + name + sep;
    const std::size_t room = static_cast<std::size_t>(
        std::max(0, maxCols - static_cast<int>(prompt.size())));
    std::size_t take = std::min(inputLen, room);
    if (take < inputLen && take > 3)
    {
        prompt += "<";
        take -= 1;
    }
    if (take > 0)
    {
        prompt.append(inputBuf + (inputLen - take), inputBuf + inputLen);
    }
    mvaddnstr(row, 0, prompt.c_str(), w);
    clrtoeol();
}

void NcursesUi::pumpStdinKeys(const std::function<void(char)>& sink)
{
    if (!mInitialized)
    {
        return;
    }
    int ch = 0;
    while ((ch = ::getch()) != ERR)
    {
        if (ch == KEY_RESIZE)
        {
            g_resizePending = 1;
            continue;
        }
        if (ch == KEY_LEFT)
        {
            sink(static_cast<char>(27));
            sink(static_cast<char>(91));
            sink(static_cast<char>(68));
        }
        else if (ch == KEY_RIGHT)
        {
            sink(static_cast<char>(27));
            sink(static_cast<char>(91));
            sink(static_cast<char>(67));
        }
        else if (ch == KEY_DOWN)
        {
            sink(static_cast<char>(27));
            sink(static_cast<char>(91));
            sink(static_cast<char>(66));
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == KEY_DC)
        {
            sink(static_cast<char>(127));
        }
        else if (ch == KEY_ENTER || ch == '\r' || ch == '\n')
        {
            sink(static_cast<char>('\n'));
        }
        else if (ch >= 0 && ch < 256)
        {
            sink(static_cast<char>(ch));
        }
    }
}

bool NcursesUi::pollResize()
{
    if (g_resizePending)
    {
        g_resizePending = 0;
        if (mInitialized)
        {
            ::endwin();
            ::refresh();
        }
        return true;
    }
    return false;
}

void NcursesUi::fullRedraw(TetrisClient& client)
{
    if (!mInitialized)
    {
        return;
    }
    erase();

    const int w = termCols() > 0 ? termCols() : 80;
    std::string nameLine = "Name: ";
    nameLine += client.mClientName;
    mvaddnstr(0, 0, nameLine.c_str(), w);
    drawSeparator(1, w);

    const bool inPlayer = static_cast<bool>(client.mPlayer);
    const bool showBoards = inPlayer;
    const bool gameOver = inPlayer && client.mMatchEndedAwaitingLobby;
    const bool playing = inPlayer && client.mPlayer->mGameStarted && !gameOver;
    const bool preMatchLobby = inPlayer && !client.mPlayer->mGameStarted && !gameOver;

    if (showBoards)
    {
        if (playing)
        {
            mvaddnstr(2, 0, "Start", w);
        }
        else if (gameOver)
        {
            move(2, 0);
            clrtoeol();
        }
        else
        {
            mvaddnstr(2, 0, "Waiting for match...", w);
        }
        paintGame(*client.mPlayer, client);
    }
    else
    {
        mvaddnstr(2, 0, "", w);
    }

    const int inputRow = termLines() - 1;
    const int scrollEnd = inputRow - 2;

    if (gameOver)
    {
        const int bh = client.mPlayer->mConfig.boardHeight;
        const int rowAfterBoards = boardBlockBottomRow(3, bh) + 1;
        int row = rowAfterBoards;
        if (row < termLines())
        {
            mvaddnstr(row, 0, "GAME OVER!", w);
            clrtoeol();
            ++row;
        }
        if (row < termLines())
        {
            drawSeparator(row, w);
            ++row;
        }
        if (row <= scrollEnd)
        {
            drawChatScroll(row, scrollEnd, w, mLog);
        }
    }
    else if (!showBoards)
    {
        const int scrollStart = 3;
        if (scrollStart <= scrollEnd)
        {
            drawChatScroll(scrollStart, scrollEnd, w, mLog);
        }
    }
    else if (preMatchLobby)
    {
        const int bh = client.mPlayer->mConfig.boardHeight;
        int row = boardBlockBottomRow(3, bh) + 1;
        if (row < termLines())
        {
            drawSeparator(row, w);
            ++row;
        }
        if (row <= scrollEnd)
        {
            drawChatScroll(row, scrollEnd, w, mLog);
        }
    }
    else if (playing)
    {
        const int bh = client.mPlayer->mConfig.boardHeight;
        int row = boardBlockBottomRow(3, bh) + 1;
        if (row < termLines())
        {
            drawSeparator(row, w);
            ++row;
        }
        if (row <= scrollEnd)
        {
            drawChatScroll(row, scrollEnd, w, mLog);
        }
    }

    const bool gameplayHideTyped =
        playing && !client.gameplayChatCompose();
    syncInputLine(
        client.mClientName,
        gameplayHideTyped ? "" : client.mConsoleInputBuffer,
        gameplayHideTyped ? 0 : client.mConsoleInputBufferIdx,
        client.gameplayChatCompose());

    ::refresh();
}

void NcursesUi::paintGame(Player& player, TetrisClient& client)
{
    if (!mInitialized || termLines() < 8 || termCols() < 20)
    {
        return;
    }

    const int w = termCols();
    const int bh = player.mConfig.boardHeight;
    const int bw = player.mConfig.boardWidth;
    const int colStride = bw + 2 + 3;
    const int n = static_cast<int>(player.mPlayers.size());

    std::vector<uint8_t> order;
    order.reserve(static_cast<std::size_t>(n));
    order.push_back(player.mPlayerId);
    for (const auto& kv : player.mPlayers)
    {
        if (kv.first != player.mPlayerId)
        {
            order.push_back(kv.first);
        }
    }
    std::sort(order.begin() + 1, order.end());

    const int maxCols = std::max(1, (w + 1) / std::max(1, colStride));
    const int show = std::min(n, maxCols);
    const int originY = 3;

    for (int idx = 0; idx < show; ++idx)
    {
        const uint8_t pid = order[static_cast<std::size_t>(idx)];
        auto it = player.mPlayers.find(pid);
        if (it == player.mPlayers.end())
        {
            continue;
        }
        const int colX = idx * colStride;
        if (colX + bw + 2 > w)
        {
            break;
        }
        const bool isTarget = player.mCurrentTarget && *player.mCurrentTarget == pid;
        const bool isDead = it->second.dead;
        drawBoardColumn(originY, colX, colStride - 1, bw, bh, it->second, isTarget, isDead);
    }

    const int reservedBottom = client.mMatchEndedAwaitingLobby ? 4 : 2;
    const int keysLatencyRow = 2;
    if (player.mGameStarted || client.mMatchEndedAwaitingLobby)
    {
        if (keysLatencyRow >= 0 && keysLatencyRow < termLines() && !client.mMatchEndedAwaitingLobby)
        {
            const char k0 = player.mKeyPressHistory[2];
            const char k1 = player.mKeyPressHistory[1];
            const char k2 = player.mKeyPressHistory[0];
            char status[96];
            const int len = std::snprintf(
                status,
                sizeof(status),
                "keys:%3d %3d %3d  latency:%4ldms",
                static_cast<int>(static_cast<unsigned char>(k0)),
                static_cast<int>(static_cast<unsigned char>(k1)),
                static_cast<int>(static_cast<unsigned char>(k2)),
                static_cast<long>(player.mLatencyMeas.count()));
            if (len > 0)
            {
                const int x = std::max(0, w - len);
                mvaddnstr(keysLatencyRow, x, status, w - x);
            }
        }
    }

    const int footRow = boardBlockBottomRow(originY, bh);
    const int moreRow = footRow + 1;
    if (n > show && show >= 1 && moreRow >= 3 && moreRow < termLines() - reservedBottom
        && !client.mMatchEndedAwaitingLobby)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "+%d more players (widen terminal)", n - show);
        mvaddnstr(moreRow, 0, buf, w);
        clrtoeol();
    }
}

} // namespace tetris
