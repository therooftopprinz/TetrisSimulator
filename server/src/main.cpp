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
    bfc::Singleton<bfc::Timer<>>::instantiate();
    bfc::Singleton<bfc::Log2MemoryPool<>>::instantiate();

    tetris::TetrisSimulatorConfig config{};
    config.port = 9999;

    tetris::TetrisSimulator sim(config);

    sim.run();
}