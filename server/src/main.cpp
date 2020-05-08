#include <TetrisSimulator.hpp>

#include <logless/Logger.hpp>

int main()
{
    Logger::getInstance().logful();

    tetris::TetrisSimulatorConfig config{};
    config.port = 9999;

    tetris::TetrisSimulator sim(config);

    sim.run();
}