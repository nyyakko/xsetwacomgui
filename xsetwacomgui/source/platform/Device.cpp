#include "platform/Device.hpp"

#include <fmt/core.h>
#include <fmt/format.h>
#include <fplus/split.hpp>
#include <liberror/Try.hpp>
#include <libexec/Execute.hpp>
#include <magic_enum/magic_enum.hpp>

#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>

#include <ranges>
#include <algorithm>
#include <functional>
#include <regex>
#include <sstream>

using namespace liberror;

static Result<std::string> execute(std::string const& command)
{
    auto [out, err] = TRY(libexec::execute("xsetwacom", fplus::split(' ', false, command)));
    if (!err.empty()) return make_error(err);
    return out;
}

Result<std::vector<Device>> get_available_devices()
{
    std::vector<Device> devices {};

    auto fnTrim = [] (auto const& value) {
        auto result = value;
        result.erase(result.begin(), std::ranges::find_if(result, std::not_fn(isspace)));
        result.erase(std::find_if(result.rbegin(), result.rend(), std::not_fn(isspace)).base(), result.end());
        return result;
    };

    auto output = TRY(execute("--list devices"));
    std::regex pattern(R"((.+)\s+id: (\d+)\s+type: (\w+))");
    std::sregex_iterator iterator(output.begin(), output.end(), pattern);
    for (; iterator != std::sregex_iterator{}; iterator = std::next(iterator))
    {
        devices.push_back({
            .name = fnTrim(iterator->str(1)),
            .id = std::atoi(fnTrim(iterator->str(2)).data()),
            .kind = *magic_enum::enum_cast<Device::Kind>(fnTrim(iterator->str(3)))
        });
    }

    return devices;
}

Result<Device::Pressure> get_stylus_pressure_curve(Device stylus)
{
    Device::Pressure pressure {};
    auto command = fmt::format("--get {} PressureCurve", stylus.id);
    auto output = TRY(execute(command));
    std::stringstream sstream(output);
    sstream >> pressure.minX >> pressure.minY;
    pressure.minX /= 100.f;
    pressure.minY /= 100.f;
    sstream >> pressure.maxX >> pressure.maxY;
    pressure.maxX /= 100.f;
    pressure.maxY /= 100.f;
    return pressure;
}

Result<void> set_stylus_pressure_curve(Device stylus, Device::Pressure pressure)
{
    auto command = fmt::format("--set {} PressureCurve {} {} {} {}",
        stylus.id,
        static_cast<int>(std::round(pressure.minX * 100.f)),
        static_cast<int>(std::round(pressure.minY * 100.f)),
        static_cast<int>(std::round(pressure.maxX * 100.f)),
        static_cast<int>(std::round(pressure.maxY * 100.f))
    );
    TRY(execute(command));
    return {};
}

Result<int> get_stylus_threshold(Device stylus)
{
    auto threshold = 0;
    auto command = fmt::format("--get {} Threshold", stylus.id);
    auto output = TRY(execute(command));
    std::stringstream sstream(output);
    sstream >> threshold;
    return threshold;
}

Result<void> set_stylus_threshold(Device stylus, int threshold)
{
    auto command = fmt::format("--set {} Threshold {}", stylus.id, threshold);
    TRY(execute(command));
    return {};
}

Result<int> get_stylus_cursor_proximity(Device stylus)
{
    auto proximity = 0;
    auto command = fmt::format("--get {} CursorProximity", stylus.id);
    auto output = TRY(execute(command));
    std::stringstream sstream(output);
    sstream >> proximity;
    return proximity;
}

Result<void> set_stylus_cursor_proximity(Device stylus, int proximity)
{
    auto command = fmt::format("--set {} CursorProximity {}", stylus.id, proximity);
    TRY(execute(command));
    return {};
}

Result<Device::Area> get_stylus_default_area(Device stylus)
{
    auto previousArea = TRY(get_stylus_area(stylus));
    TRY(reset_stylus_area(stylus));
    auto defaultArea = TRY(get_stylus_area(stylus));
    TRY(set_stylus_area(stylus, previousArea));
    return defaultArea;
}

Result<Device::Area> get_stylus_area(Device stylus)
{
    Device::Area area {};
    auto command = fmt::format("--get {} Area", stylus.id);
    auto output = TRY(execute(command));
    std::stringstream sstream(output);
    sstream >> area.offsetX >> area.offsetY;
    sstream >> area.width >> area.height;
    return area;
}

Result<void> set_stylus_area(Device stylus, Device::Area area)
{
    auto command = fmt::format("--set {} Area {} {} {} {}",
        stylus.id,
        std::round(area.offsetX),
        std::round(area.offsetY),
        std::round(area.width),
        std::round(area.height)
    );
    TRY(execute(command));
    return {};
}

Result<void> reset_stylus_area(Device stylus)
{
    auto command = fmt::format("--set {} ResetArea", stylus.id);
    TRY(execute(command));
    return {};
}

Result<void> set_stylus_output_from_display_name(Device stylus, std::string_view displayName)
{
    auto command = fmt::format("--set {} MapToOutput {}", stylus.id, displayName);
    TRY(execute(command));
    return {};
}

Result<void> set_stylus_output_from_display_area(Device stylus, Display::Area area)
{
    auto command = fmt::format("--set {} MapToOutput {}x{}+{}+{}",
        stylus.id,
        static_cast<int>(std::round(area.width)),
        static_cast<int>(std::round(area.height)),
        static_cast<int>(std::round(area.offsetX)),
        static_cast<int>(std::round(area.offsetY))
    );
    TRY(execute(command));
    return {};
}

Result<void> set_stylus_handedness(Device stylus, Device::Handedness handedness)
{
    std::string command {};

    switch (handedness)
    {
    case Device::Handedness::LEFT: {
        command = fmt::format("--set {} Rotate 3", stylus.id);
        break;
    }
    case Device::Handedness::RIGHT: {
        command = fmt::format("--set {} Rotate 0", stylus.id);
        break;
    }
    }

    TRY(execute(command));

    return {};
}

Result<std::map<int, X11Action>> get_device_button_mappings(Device device)
{
    std::map<int, X11Action> mappings {};

    std::regex pattern(R"(button [^]([^]+))");

    for (auto button : std::views::iota(1zu, 25zu))
    {
        auto output = execute(fmt::format("--get {} Button {}", device.id, button));
        if (!output.has_value()) continue;

        if (!output->starts_with("button"))
        {
            continue;
        }

        std::sregex_iterator iterator(output->begin(), output->end(), pattern);
        for (; iterator != std::sregex_iterator{}; iterator = std::next(iterator))
        {
            mappings.insert({ button, X11Action(std::atoi(iterator->str(1).data())) });
        }
    }

    return mappings;
}

Result<void> set_device_button_mappings(Device device, std::map<int, X11Action> const& mappings)
{
    for (auto const& [button, action] : mappings)
    {
        TRY(execute(fmt::format("--set {} Button {} {}", device.id, button, int(action))));
    }

    return {};
}
