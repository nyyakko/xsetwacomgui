#include "platform/hid/X11/Display.hpp"

#include <fmt/format.h>
#include <liberror/Try.hpp>
#include <libexec/Execute.hpp>
#include <range/v3/view.hpp>

#include <cstdlib>
#include <regex>
#include <algorithm>

using namespace liberror;

static Result<std::string> execute(std::string const& command)
{
    auto [out, err] = TRY(libexec::execute("xrandr", command | ranges::views::split(' ') | ranges::to<std::vector<std::string>>));
    if (!err.empty()) return make_error(err);
    return out;
}

Result<std::vector<Display>> get_available_displays()
{
    std::vector<Display> displays {};

    auto output = TRY(execute("--listactivemonitors"));

    std::regex pattern(R"((\d+):\s*\+(\*?)([A-Za-z0-9\-]+)\s(\d+)\/\d+x(\d+)\/\d+\+(\d+)\+(\d+))");
    std::sregex_iterator iterator(output.begin(), output.end(), pattern);
    for (; iterator != std::sregex_iterator{}; iterator = std::next(iterator))
    {
        Display display {
            .id = std::atoi(iterator->str(1).data()),
            .primary = !iterator->str(2).empty(),
            .area = {
                .offsetX = static_cast<float>(std::atof(iterator->str(6).data())),
                .offsetY = static_cast<float>(std::atof(iterator->str(7).data())),
                .width = static_cast<float>(std::atof(iterator->str(4).data())),
                .height = static_cast<float>(std::atof(iterator->str(5).data())),
            },
        };

        display.name = fmt::format("{} ({}x{})", iterator->str(3), display.area.width, display.area.height);

        displays.push_back(display);
    }

    return displays;
}

Result<Display> get_primary_display()
{
    auto displays = TRY(get_available_displays());
    auto maybeDisplay = std::ranges::find_if(displays, &Display::primary);

    if (maybeDisplay == displays.end())
    {
        return make_error("Could not find primary display");
    }

    return *maybeDisplay;
}
