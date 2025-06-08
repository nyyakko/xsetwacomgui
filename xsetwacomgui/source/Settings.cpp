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

bool load_device_settings(DeviceSettings& settings)
{
    std::ifstream stream(DEVICE_SETTINGS_FILE);
    std::stringstream content;
    content << stream.rdbuf();

    try
    {
        auto json = nlohmann::json::parse(content.str());
        settings.monitorName             = json["monitorName"].get<std::string>();
        settings.monitorForceFullArea    = json["monitorForceFullArea"].get<bool>();
        settings.monitorForceAspectRatio = json["monitorForceAspectRatio"].get<bool>();
        settings.monitorArea.offsetX     = json["monitorArea"]["offsetX"].get<float>();
        settings.monitorArea.offsetY     = json["monitorArea"]["offsetY"].get<float>();
        settings.monitorArea.width       = json["monitorArea"]["width"].get<float>();
        settings.monitorArea.height      = json["monitorArea"]["height"].get<float>();
        settings.deviceName              = json["deviceName"].get<std::string>();
        settings.deviceForceFullArea     = json["deviceForceFullArea"].get<bool>();
        settings.deviceForceAspectRatio  = json["deviceForceAspectRatio"].get<bool>();
        settings.deviceArea.offsetX      = json["deviceArea"]["offsetX"].get<float>();
        settings.deviceArea.offsetY      = json["deviceArea"]["offsetY"].get<float>();
        settings.deviceArea.width        = json["deviceArea"]["width"].get<float>();
        settings.deviceArea.height       = json["deviceArea"]["height"].get<float>();
        settings.devicePressure.minX     = json["devicePressure"]["minX"].get<float>();
        settings.devicePressure.minY     = json["devicePressure"]["minY"].get<float>();
        settings.devicePressure.maxX     = json["devicePressure"]["maxX"].get<float>();
        settings.devicePressure.maxY     = json["devicePressure"]["maxY"].get<float>();
    }
    catch (std::exception const& error)
    {
        return false;
    }

    return !(stream.bad() || stream.fail());
}

bool save_device_settings(DeviceSettings const& settings)
{
    nlohmann::ordered_json json {
        { "deviceName", settings.deviceName },
        {
            "deviceArea", {
                { "offsetX", settings.deviceArea.offsetX },
                { "offsetY", settings.deviceArea.offsetY },
                { "width", settings.deviceArea.width },
                { "height", settings.deviceArea.height }
            }
        },
        {
            "devicePressure", {
                { "minX", settings.devicePressure.minX },
                { "minY", settings.devicePressure.minY },
                { "maxX", settings.devicePressure.maxX },
                { "maxY", settings.devicePressure.maxY },
            }
        },
        { "deviceForceFullArea", settings.deviceForceFullArea },
        { "deviceForceAspectRatio", settings.deviceForceAspectRatio },
        { "monitorName", settings.monitorName },
        {
            "monitorArea", {
                { "offsetX", settings.monitorArea.offsetX },
                { "offsetY", settings.monitorArea.offsetY },
                { "width", settings.monitorArea.width },
                { "height", settings.monitorArea.height }
            }
        },
        { "monitorForceFullArea", settings.monitorForceFullArea },
        { "monitorForceAspectRatio", settings.monitorForceAspectRatio },
    };

    std::ofstream stream(DEVICE_SETTINGS_FILE);
    stream << std::setw(4) << json;

    return !(stream.bad() || stream.fail());
}

bool load_application_settings(ApplicationSettings& settings)
{
    std::ifstream stream(APPLICATION_SETTINGS_FILE);
    std::stringstream content;
    content << stream.rdbuf();

    try
    {
        auto json = nlohmann::json::parse(content.str());

        settings.scale    = json["interface"]["scale"].get<float>();
        settings.theme    = ApplicationSettings::Theme::from_string((json["interface"]["theme"].get<std::string>()));
        settings.font     = json["interface"]["font"].get<std::string>();
        settings.language = json["general"]["language"].get<std::string>();
    }
    catch (std::exception const& error)
    {
        return false;
    }

    return !(stream.bad() || stream.fail());
}

bool save_application_settings(ApplicationSettings& settings)
{
    nlohmann::ordered_json json {
        {
            "interface", {
                { "scale", settings.scale },
                { "theme", settings.theme.to_string() },
                { "font", settings.font },
            }
        },
        {
            "general", {
                { "language", settings.language },
            }
        }
    };

    std::ofstream stream(APPLICATION_SETTINGS_FILE);
    stream << std::setw(4) << json;

    return !(stream.bad() || stream.fail());
}
