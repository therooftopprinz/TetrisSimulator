#include <TetrisSimulator.hpp>

#include <singleton.hpp>
#include <bfc/thread_pool.hpp>
#include <bfc/memory_pool.hpp>

#include <tetris_log.hpp>

int main()
{
    tetris_logger().logful();

    tetris::singleton<bfc::thread_pool<>>::instantiate();
    tetris::singleton<bfc::log2_memory_pool<>>::instantiate();

    tetris::TetrisSimulatorConfig config{};
    config.port = 9999;

    tetris::TetrisSimulator sim(config);

    sim.run();
}