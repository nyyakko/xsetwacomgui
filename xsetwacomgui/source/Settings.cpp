#include "Settings.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <cstdlib>

std::filesystem::path get_settings_base_path()
{
    auto home = getenv("XDG_CONFIG_HOME");
    if (home == nullptr) home = getenv("HOME");
    assert(home != nullptr && "Why are you homeless?");
    return std::filesystem::path(home) / ".config";
}

bool load_device_settings(DeviceSetttings settings)
{
    std::ifstream stream(SETTINGS_FILE);
    std::stringstream content;
    content << stream.rdbuf();

    try
    {
        auto json = nlohmann::json::parse(content.str());
        settings.forceFullArea    = json["forceFullArea"].get<bool>();
        settings.forceAspectRatio = json["forceAspectRatio"].get<bool>();
        settings.area.offsetX     = json["area"]["offsetX"].get<float>();
        settings.area.offsetY     = json["area"]["offsetY"].get<float>();
        settings.area.width       = json["area"]["width"].get<float>();
        settings.area.height      = json["area"]["height"].get<float>();
        settings.pressure.minX    = json["pressure"]["minX"].get<float>();
        settings.pressure.minY    = json["pressure"]["minY"].get<float>();
        settings.pressure.maxX    = json["pressure"]["maxX"].get<float>();
        settings.pressure.maxY    = json["pressure"]["maxY"].get<float>();
    }
    catch (std::exception const& error)
    {
        return false;
    }

    return true;
}

bool save_device_settings(DeviceSetttings settings)
{
    nlohmann::ordered_json json {
        { "forceFullArea", settings.forceFullArea },
        { "forceAspectRatio", settings.forceAspectRatio },
        {
            "area", {
                { "offsetX", settings.area.offsetX },
                { "offsetY", settings.area.offsetY },
                { "width", settings.area.width },
                { "height", settings.area.height }
            }
        },
        {
            "pressure", {
                { "minX", settings.pressure.minX },
                { "minY", settings.pressure.minY },
                { "maxX", settings.pressure.maxX },
                { "maxY", settings.pressure.maxY },
            }
        }
    };

    std::ofstream stream(SETTINGS_FILE);
    stream << std::setw(4) << json;

    return true;
}
