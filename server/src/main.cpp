#include <TetrisSimulator.hpp>

#include <bfc/Singleton.hpp>
#include <bfc/Timer.hpp>
#include <bfc/ThreadPool.hpp>
#include <bfc/MemoryPool.hpp>

#include <logless/Logger.hpp>

int main()
{
    Logger::getInstance().logful();

    bfc::Singleton<bfc::ThreadPool<>>::instantiate();
    auto& timer = bfc::Singleton<bfc::Timer<>>::instantiate();
    bfc::Singleton<bfc::Log2MemoryPool<>>::instantiate();

    std::thread timerThread([&timer]{
        timer.run();
    });

    tetris::TetrisSimulatorConfig config{};
    config.port = 9999;

    tetris::TetrisSimulator sim(config);

    sim.run();
    timer.stop();
    timerThread.join();
}