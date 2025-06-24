#include "Settings.hpp"
#include "Environment.hpp"

#include <filesystem>
#include <liberror/Result.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <cstdlib>

liberror::Result<void, SettingsError> load_device_settings(DeviceSettings& settings)
{
    std::ifstream stream(DEVICE_SETTINGS_FILE);
    std::stringstream content;
    content << stream.rdbuf();

    auto previousSettings = settings;

    try
    {
        auto json = nlohmann::json::parse(content.str());

        if (json["version"].is_null() || json["version"].get<std::string>() != DeviceSettings::SCHEMA_VERSION)
        {
            settings = previousSettings;
            return liberror::make_error<SettingsError>(SettingsError::Type::OUTDATED_SCHEMA);
        }

        settings.monitorName             = json["monitor"]["name"].get<std::string>();
        settings.monitorForceFullArea    = json["monitor"]["forceFullArea"].get<bool>();
        settings.monitorForceAspectRatio = json["monitor"]["forceAspectRatio"].get<bool>();
        settings.monitorArea.offsetX     = json["monitor"]["area"]["offsetX"].get<float>();
        settings.monitorArea.offsetY     = json["monitor"]["area"]["offsetY"].get<float>();
        settings.monitorArea.width       = json["monitor"]["area"]["width"].get<float>();
        settings.monitorArea.height      = json["monitor"]["area"]["height"].get<float>();
        settings.deviceName              = json["device"]["name"].get<std::string>();
        settings.deviceHandedness        = libwacom::Handedness::from_string(json["device"]["handedness"].get<std::string>());
        settings.deviceForceFullArea     = json["device"]["forceFullArea"].get<bool>();
        settings.deviceForceAspectRatio  = json["device"]["forceAspectRatio"].get<bool>();
        settings.deviceArea.offsetX      = json["device"]["area"]["offsetX"].get<float>();
        settings.deviceArea.offsetY      = json["device"]["area"]["offsetY"].get<float>();
        settings.deviceArea.width        = json["device"]["area"]["width"].get<float>();
        settings.deviceArea.height       = json["device"]["area"]["height"].get<float>();
        settings.devicePressure.minX     = json["device"]["pressure"]["minX"].get<float>();
        settings.devicePressure.minY     = json["device"]["pressure"]["minY"].get<float>();
        settings.devicePressure.maxX     = json["device"]["pressure"]["maxX"].get<float>();
        settings.devicePressure.maxY     = json["device"]["pressure"]["maxY"].get<float>();
    }
    catch (std::exception const& error)
    {
        settings = previousSettings;
        return liberror::make_error<SettingsError>(SettingsError::Type::READ_FAILURE);
    }

    return {};
}

void save_device_settings(DeviceSettings const& settings)
{
    nlohmann::ordered_json json {
        { "version", DeviceSettings::SCHEMA_VERSION },
        {
            "device", {
                { "name", settings.deviceName },
                { "handedness", settings.deviceHandedness.to_string() },
                {
                    "area", {
                        { "offsetX", settings.deviceArea.offsetX },
                        { "offsetY", settings.deviceArea.offsetY },
                        { "width", settings.deviceArea.width },
                        { "height", settings.deviceArea.height }
                    }
                },
                {
                    "pressure", {
                        { "minX", settings.devicePressure.minX },
                        { "minY", settings.devicePressure.minY },
                        { "maxX", settings.devicePressure.maxX },
                        { "maxY", settings.devicePressure.maxY },
                    }
                },
                { "forceFullArea", settings.deviceForceFullArea },
                { "forceAspectRatio", settings.deviceForceAspectRatio },
            }
        },
        {
            "monitor", {
                { "name", settings.monitorName },
                {
                    "area", {
                        { "offsetX", settings.monitorArea.offsetX },
                        { "offsetY", settings.monitorArea.offsetY },
                        { "width", settings.monitorArea.width },
                        { "height", settings.monitorArea.height }
                    }
                },
                { "forceFullArea", settings.monitorForceFullArea },
                { "forceAspectRatio", settings.monitorForceAspectRatio },
            }
        }
    };

    std::ofstream stream(DEVICE_SETTINGS_FILE);
    stream << std::setw(4) << json;
}

void migrate_device_settings(DeviceSettings const& settings)
{
    static auto newSettingsSchema = get_application_config_path() / "device.json";
    static auto oldSettingsSchema = get_application_config_path() / "device.old.json";

    std::filesystem::rename(DEVICE_SETTINGS_FILE, oldSettingsSchema);

    save_device_settings(settings);

    popen(fmt::format("xdg-open {}", get_application_config_path().string()).data(), "r");
    pclose(popen(fmt::format("git diff {} {} > {}/conflict.diff", oldSettingsSchema.string(), newSettingsSchema.string(), get_application_config_path().string()).data(), "r"));
}

liberror::Result<void, SettingsError> load_application_settings(ApplicationSettings& settings)
{
    std::ifstream stream(APPLICATION_SETTINGS_FILE);
    std::stringstream content;
    content << stream.rdbuf();

    auto previousSettings = settings;

    try
    {
        auto json = nlohmann::json::parse(content.str());

        if (json["version"].is_null() || json["version"].get<std::string>() != ApplicationSettings::SCHEMA_VERSION)
        {
            settings = previousSettings;
            return liberror::make_error<SettingsError>(SettingsError::Type::OUTDATED_SCHEMA);
        }

        settings.theme    = ApplicationSettings::Theme::from_string((json["appearance"]["theme"].get<std::string>()));
        settings.font     = json["appearance"]["font"].get<std::string>();
        settings.scale    = json["display"]["scale"].get<float>();
        settings.language = ApplicationSettings::Language::from_string(json["language"]["language"].get<std::string>());
    }
    catch (std::exception const& error)
    {
        settings = previousSettings;
        return liberror::make_error<SettingsError>(SettingsError::Type::READ_FAILURE);
    }

    return {};
}

void save_application_settings(ApplicationSettings const& settings)
{
    nlohmann::ordered_json json {
        { "version", ApplicationSettings::SCHEMA_VERSION },
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
}

void migrate_application_settings(ApplicationSettings const& settings)
{
    static auto newSettingsSchema = get_application_config_path() / "application.json";
    static auto oldSettingsSchema = get_application_config_path() / "application.old.json";

    std::filesystem::rename(APPLICATION_SETTINGS_FILE, oldSettingsSchema);

    save_application_settings(settings);

    popen(fmt::format("xdg-open {}", get_application_config_path().string()).data(), "r");
    pclose(popen(fmt::format("git diff {} {} > {}/conflict.diff", oldSettingsSchema.string(), newSettingsSchema.string(), get_application_config_path().string()).data(), "r"));
}
