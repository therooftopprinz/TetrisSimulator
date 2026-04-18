#include <iostream>
#include <regex>
#include <TetrisClient.hpp>
#include <tetris_log.hpp>

namespace
{

void printClientUsage(const char* argv0)
{
    std::cerr
        << "Usage: " << argv0 << " --server=<IPv4>:<port> [--cmd=<line>]\n\n"
        << "Startup options (each argument must be --name=value):\n"
        << "  --server=<IPv4>:<port>  Required. TCP address of the Tetris server.\n"
        << "  --cmd=<text>            Optional. One console line to run after connect\n"
        << "                          (for example: /create or /join id=1).\n\n"
        << "At the interactive prompt, lines without a leading / send chat; type \"/help\" for commands.\n";
}

} // namespace

std::pair<uint32_t, uint16_t> parseIpPort(const std::map<std::string, std::string>& pOptions)
{
    std::regex addressFilter("([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+):([0-9]+)");
    std::smatch match;
    auto it = pOptions.find("server");
    std::pair<uint32_t, uint16_t> rv{};
    if (it == pOptions.cend())
    {
        throw std::runtime_error("No server address specified!");
    }
    else
    {
        if (std::regex_match(it->second, match, addressFilter))
        {
            if (match.size() != 6)
            {
                throw std::runtime_error(std::string("invalid address: `") + it->second + "`");
            }
            uint8_t a = std::stoi(match[1].str());
            uint8_t b = std::stoi(match[2].str());
            uint8_t c = std::stoi(match[3].str());
            uint8_t d = std::stoi(match[4].str());
            uint16_t port = std::stoi(match[5].str());
            rv.first |= (d << 24);
            rv.first |= (c << 16);
            rv.first |= (b << 8);
            rv.first |= a;
            rv.second = port;
        }
        else
        {
            throw std::runtime_error(std::string("invalid address: `") + it->second + "`");
        }
    }
    return rv;
}

int main(int argc, const char *argv[])
{
    std::regex arger("^--(.+?)=(.+?)$");
    std::smatch match;
    std::map<std::string, std::string> options;

    tetris_logger().logless();

    for (int i=1; i<argc; i++)
    {
        auto s = std::string(argv[i]);
        if (s == "--help" || s == "-h")
        {
            printClientUsage(argv[0]);
            return 0;
        }
        if (std::regex_match(s, match, arger))
        {
            options.emplace(match[1].str(), match[2].str());
        }
        else
        {
            printClientUsage(argv[0]);
            throw std::runtime_error(std::string("invalid argument: `") + argv[i] + "`");
        }
    }

    tetris::TetrisClientConfig config{};

    auto address = parseIpPort(options); 

    auto cmdIt = options.find("cmd");
    if (options.end() != cmdIt)
    {
        config.cmd.emplace(cmdIt->second);
    }


    config.ip = address.first;
    config.port = address.second;
    tetris::TetrisClient client(config);
    client.run();
}