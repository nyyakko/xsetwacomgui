#include "platform/Monitor.hpp"

#include <liberror/Try.hpp>
#include <fmt/format.h>
#include <libexec/Execute.hpp>

#include <cstdlib>
#include <regex>
#include <algorithm>

liberror::Result<Monitor> get_primary_monitor()
{
    auto monitors = TRY(get_available_monitors());
    auto maybeMonitor = std::ranges::find_if(monitors, &Monitor::primary);
    if (maybeMonitor == monitors.end())
    {
        return liberror::make_error("Could not find primary monitor");
    }

    return *maybeMonitor;
}

liberror::Result<std::vector<Monitor>> get_available_monitors()
{
    std::vector<Monitor> monitors {};

    auto [out, err] = TRY(libexec::execute("xrandr", { "--listactivemonitors" }));

    if (!err.empty())
    {
        return liberror::make_error(err);
    }

    std::regex pattern(R"((\d+):\s*\+(\*?)([A-Za-z0-9\-]+)\s(\d+)\/\d+x(\d+)\/\d+\+(\d+)\+(\d+))");
    std::sregex_iterator iterator(out.begin(), out.end(), pattern);
    for (; iterator != std::sregex_iterator{}; iterator = std::next(iterator))
    {
        monitors.push_back({
            .id = std::atoi(iterator->str(1).data()),
            .primary = !iterator->str(2).empty(),
            .area = {
                .offsetX = static_cast<float>(std::atof(iterator->str(6).data())),
                .offsetY = static_cast<float>(std::atof(iterator->str(7).data())),
                .width = static_cast<float>(std::atof(iterator->str(4).data())),
                .height = static_cast<float>(std::atof(iterator->str(5).data())),
            },
            .name = iterator->str(3)
        });
    }

    return monitors;
}
