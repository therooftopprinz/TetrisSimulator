#include <bfc/epoll_reactor.hpp>

#include <TetrisSimulator.hpp>

#include <tetris_log.hpp>

#include <csignal>
#include <fcntl.h>
#include <unistd.h>

namespace
{

int g_serverShutdownPipeWrite = -1;

void serverShutdownSignalHandler(int)
{
    unsigned char b = 1;
    if (g_serverShutdownPipeWrite >= 0)
    {
        (void)::write(g_serverShutdownPipeWrite, &b, 1);
    }
}

} // namespace

int main()
{
    tetris_logger().logful();

    bfc::epoll_reactor<> reactor;

    int shutdownPipe[2] = {-1, -1};
    if (pipe2(shutdownPipe, O_CLOEXEC) != 0)
    {
        return 1;
    }
    g_serverShutdownPipeWrite = shutdownPipe[1];

    if (!reactor.add_read_rdy(shutdownPipe[0], [&reactor, readFd = shutdownPipe[0]]() {
            char buf[32];
            (void)::read(readFd, buf, sizeof buf);
            reactor.stop();
        }))
    {
        ::close(shutdownPipe[0]);
        ::close(shutdownPipe[1]);
        return 1;
    }

    struct sigaction sa{};
    sa.sa_handler = serverShutdownSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    tetris::TetrisSimulatorConfig config{};
    config.port = 9999;

    tetris::TetrisSimulator sim(reactor, config);

    reactor.run();

    g_serverShutdownPipeWrite = -1;
    ::close(shutdownPipe[0]);
    ::close(shutdownPipe[1]);
    return 0;
}