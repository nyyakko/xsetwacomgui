#include "core/Help.hpp"

#include <fmt/format.h>

#include <algorithm>

using namespace std::literals;

void get_help();
void get_config_help();

void get_help(std::span<char const*> const& arguments)
{
    if (auto posHelp = std::ranges::find(arguments, "--help"sv); std::distance(arguments.begin(), posHelp) == 1)
    {
        return get_help();
    }

    if (auto posConfig = std::ranges::find(arguments, "config"sv); posConfig != arguments.end())
    {
        auto commandArguments = arguments.subspan(size_t(std::distance(arguments.begin(), posConfig)));

        if (auto posHelp = std::ranges::find(commandArguments, "--help"sv); posHelp != arguments.end())
        {
            return get_config_help();
        }
    }
}

void get_help()
{
    fmt::println("Usage: " NAME " [--help] [--no-gui] {{config}}");
    fmt::println("\na graphical xsetwacom wrapper for ease of use");
    fmt::println("\nOptional arguments:");
    fmt::println("  --help .. shows help message");
    fmt::println("  --no-gui .. runs the program in the background");
    fmt::println("\nSubcommands:");
    fmt::println("  config .. manages device related configuration");
}

void get_config_help()
{
    fmt::println("Usage: " NAME " config [--help] [--load]");
    fmt::println("\nmanages device related configuration");
    fmt::println("\nOptional arguments:");
    fmt::println("  --help .. shows help message");
    fmt::println("  --load .. loads the saved tablet configuration");
}
