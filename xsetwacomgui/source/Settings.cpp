#include "Settings.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <cstdlib>

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

        settings.theme    = ApplicationSettings::Theme::from_string((json["appearance"]["theme"].get<std::string>()));
        settings.font     = json["appearance"]["font"].get<std::string>();
        settings.scale    = json["display"]["scale"].get<float>();
        settings.language = ApplicationSettings::Language::from_string(json["language"]["language"].get<std::string>());
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
            "appearance", {
                { "theme", settings.theme.to_string() },
                { "font", settings.font },
            }
        },
        {
            "display", {
                { "scale", settings.scale },
            }
        },
        {
            "language", {
                { "language", settings.language.to_string() },
            }
        }
    };

    std::ofstream stream(APPLICATION_SETTINGS_FILE);
    stream << std::setw(4) << json;

    return !(stream.bad() || stream.fail());
}
