#ifndef TETRIS_NCURSES_UI_HPP
#define TETRIS_NCURSES_UI_HPP

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <string>

namespace tetris
{

class TetrisClient;
class Player;

class NcursesUi
{
public:
    NcursesUi() = default;
    ~NcursesUi();

    NcursesUi(const NcursesUi&) = delete;
    NcursesUi& operator=(const NcursesUi&) = delete;

    bool init();
    void shutdown();

    void appendLog(std::string line);
    void syncInputLine(const char* inputBuf, std::size_t inputLen, bool chatCompose);
    void fullRedraw(TetrisClient& client);
    void paintGame(Player& player, TetrisClient& client);

    void pumpStdinKeys(const std::function<void(char)>& sink);
    bool pollResize();

private:
    bool mInitialized = false;
    std::deque<std::string> mLog;
    static constexpr std::size_t kMaxLogLines = 400;
};

} // namespace tetris

#endif
