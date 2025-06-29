#include "platform/Display.hpp"

#include <liberror/Try.hpp>
#include <fmt/format.h>
#include <libexec/Execute.hpp>

#include <cstdlib>
#include <regex>
#include <algorithm>

liberror::Result<Display> get_primary_display()
{
    auto displays = TRY(get_available_displays());
    auto maybeDisplay = std::ranges::find_if(displays, &Display::primary);
    if (maybeDisplay == displays.end())
    {
        return liberror::make_error("Could not find primary display");
    }

    return *maybeDisplay;
}

liberror::Result<std::vector<Display>> get_available_displays()
{
    std::vector<Display> displays {};

    auto [out, err] = TRY(libexec::execute("xrandr", { "--listactivemonitors" }));

    if (!err.empty())
    {
        return liberror::make_error(err);
    }

    std::regex pattern(R"((\d+):\s*\+(\*?)([A-Za-z0-9\-]+)\s(\d+)\/\d+x(\d+)\/\d+\+(\d+)\+(\d+))");
    std::sregex_iterator iterator(out.begin(), out.end(), pattern);
    for (; iterator != std::sregex_iterator{}; iterator = std::next(iterator))
    {
        displays.push_back({
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

    return displays;
}
