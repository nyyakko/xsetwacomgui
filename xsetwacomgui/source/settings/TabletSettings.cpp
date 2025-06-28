#include "settings/TabletSettings.hpp"

#include <fmt/format.h>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>
#include <libexec/Execute.hpp>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

liberror::Result<void, SettingsError> load_tablet_settings(TabletSettings& settings)
{
    std::ifstream stream(TABLET_SETTINGS_FILE);
    std::stringstream content;
    content << stream.rdbuf();

    auto previousSettings = settings;

    try
    {
        auto json = nlohmann::json::parse(content.str());

        if (json["version"].is_null() || json["version"].get<std::string>() != TabletSettings::SCHEMA_VERSION)
        {
            settings = previousSettings;
            return liberror::make_error<SettingsError>(SettingsError::Type::OUTDATED_SCHEMA);
        }

        settings.monitor.name             = json["monitor"]["name"].get<std::string>();
        settings.monitor.forceFullArea    = json["monitor"]["forceFullArea"].get<bool>();
        settings.monitor.forceAspectRatio = json["monitor"]["forceAspectRatio"].get<bool>();
        settings.monitor.area.offsetX     = json["monitor"]["area"]["offsetX"].get<float>();
        settings.monitor.area.offsetY     = json["monitor"]["area"]["offsetY"].get<float>();
        settings.monitor.area.width       = json["monitor"]["area"]["width"].get<float>();
        settings.monitor.area.height      = json["monitor"]["area"]["height"].get<float>();
        settings.device.name              = json["device"]["name"].get<std::string>();
        settings.device.handedness        = libwacom::Handedness::from_string(json["device"]["handedness"].get<std::string>());
        settings.device.forceFullArea     = json["device"]["forceFullArea"].get<bool>();
        settings.device.forceAspectRatio  = json["device"]["forceAspectRatio"].get<bool>();
        settings.device.area.offsetX      = json["device"]["area"]["offsetX"].get<float>();
        settings.device.area.offsetY      = json["device"]["area"]["offsetY"].get<float>();
        settings.device.area.width        = json["device"]["area"]["width"].get<float>();
        settings.device.area.height       = json["device"]["area"]["height"].get<float>();
        settings.device.pressure.minX     = json["device"]["pressure"]["minX"].get<float>();
        settings.device.pressure.minY     = json["device"]["pressure"]["minY"].get<float>();
        settings.device.pressure.maxX     = json["device"]["pressure"]["maxX"].get<float>();
        settings.device.pressure.maxY     = json["device"]["pressure"]["maxY"].get<float>();
    }
    catch (std::exception const& error)
    {
        settings = previousSettings;
        return liberror::make_error<SettingsError>(SettingsError::Type::READ_FAILURE);
    }

    return {};
}

void save_tablet_settings(TabletSettings const& settings)
{
    nlohmann::ordered_json json {
        { "version", TabletSettings::SCHEMA_VERSION },
        {
            "device", {
                { "name", settings.device.name },
                { "handedness", settings.device.handedness.to_string() },
                {
                    "area", {
                        { "offsetX", settings.device.area.offsetX },
                        { "offsetY", settings.device.area.offsetY },
                        { "width", settings.device.area.width },
                        { "height", settings.device.area.height }
                    }
                },
                {
                    "pressure", {
                        { "minX", settings.device.pressure.minX },
                        { "minY", settings.device.pressure.minY },
                        { "maxX", settings.device.pressure.maxX },
                        { "maxY", settings.device.pressure.maxY },
                    }
                },
                { "forceFullArea", settings.device.forceFullArea },
                { "forceAspectRatio", settings.device.forceAspectRatio },
            }
        },
        {
            "monitor", {
                { "name", settings.monitor.name },
                {
                    "area", {
                        { "offsetX", settings.monitor.area.offsetX },
                        { "offsetY", settings.monitor.area.offsetY },
                        { "width", settings.monitor.area.width },
                        { "height", settings.monitor.area.height }
                    }
                },
                { "forceFullArea", settings.monitor.forceFullArea },
                { "forceAspectRatio", settings.monitor.forceAspectRatio },
            }
        }
    };

    std::ofstream stream(TABLET_SETTINGS_FILE);
    stream << std::setw(4) << json;
}

liberror::Result<void> migrate_tablet_settings(TabletSettings const& settings)
{
    static auto newSettingsSchema = get_application_config_path() / "tablet_settings.json";
    static auto oldSettingsSchema = get_application_config_path() / "tablet_settings.old.json";

    std::filesystem::rename(TABLET_SETTINGS_FILE, oldSettingsSchema);

    save_tablet_settings(settings);

    TRY(libexec::execute("xdg-open", { get_application_config_path() }, libexec::Mode::DETACHED));
    auto [out, err] = TRY(libexec::execute("git", { "diff", oldSettingsSchema, newSettingsSchema }));
    std::ofstream stream(get_application_config_path() / "conflict.diff");
    stream << out;

    return {};
}
