#include "Monitor.hpp"

#include <liberror/Try.hpp>
#include <fmt/format.h>

#include <cstdlib>
#include <regex>

namespace xrandr {

liberror::Result<std::string> execute(std::string_view command)
{
    std::string output {};
    auto const fd = popen(fmt::format("xrandr {}", command).data(), "r");
    if (fd == nullptr)
        return liberror::make_error("File descriptor for xrandr command returned as nullptr");

    while (true)
    {
        std::array<char, 512> buffer {0};
        if (fgets(buffer.data(), buffer.size(), fd) == nullptr) break;
        output.append(buffer.data());
    }

    pclose(fd);

    return output;
}

}

liberror::Result<std::vector<Monitor>> get_available_monitors()
{
    std::vector<Monitor> monitors {};

    auto output = TRY(xrandr::execute("--listactivemonitors"));

    std::regex pattern(R"((\d+):\s*\+(\*?)([A-Za-z0-9\-]+)\s(\d+)\/\d+x(\d+)\/\d+\+(\d+)\+(\d+))");
    std::sregex_iterator iterator(output.begin(), output.end(), pattern);
    for (; iterator != std::sregex_iterator{}; iterator = std::next(iterator))
    {
        monitors.push_back({
            .id = std::atoi(iterator->str(1).data()),
            .primary = !iterator->str(2).empty(),
            .offsetX = static_cast<float>(std::atof(iterator->str(6).data())),
            .offsetY = static_cast<float>(std::atof(iterator->str(7).data())),
            .width = static_cast<float>(std::atof(iterator->str(4).data())),
            .height = static_cast<float>(std::atof(iterator->str(5).data())),
            .name = iterator->str(3)
        });
    }

    return monitors;
}
