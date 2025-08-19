#include "settings/TabletSettings.hpp"

#include <fmt/format.h>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>
#include <libexec/Execute.hpp>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace liberror;

Result<void, SettingsError> load_tablet_settings(TabletSettings& settings)
{
    if (!std::filesystem::exists(TABLET_SETTINGS_FILE))
    {
        return make_error<SettingsError>(SettingsError::Type::FILE_NOT_FOUND);
    }

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
            return make_error<SettingsError>(SettingsError::Type::OUTDATED_SCHEMA);
        }

        settings.display.name             = json["display"]["name"].get<std::string>();
        settings.display.forceFullArea    = json["display"]["forceFullArea"].get<bool>();
        settings.display.forceAspectRatio = json["display"]["forceAspectRatio"].get<bool>();
        settings.display.area.offsetX     = json["display"]["area"]["offsetX"].get<float>();
        settings.display.area.offsetY     = json["display"]["area"]["offsetY"].get<float>();
        settings.display.area.width       = json["display"]["area"]["width"].get<float>();
        settings.display.area.height      = json["display"]["area"]["height"].get<float>();
        settings.device.name              = json["device"]["name"].get<std::string>();
        settings.device.handedness        = *magic_enum::enum_cast<Device::Handedness>(json["device"]["handedness"].get<std::string>());
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
        return make_error<SettingsError>(SettingsError::Type::READ_FAILURE);
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
                { "handedness", magic_enum::enum_name<Device::Handedness>(settings.device.handedness) },
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
            "display", {
                { "name", settings.display.name },
                {
                    "area", {
                        { "offsetX", settings.display.area.offsetX },
                        { "offsetY", settings.display.area.offsetY },
                        { "width", settings.display.area.width },
                        { "height", settings.display.area.height }
                    }
                },
                { "forceFullArea", settings.display.forceFullArea },
                { "forceAspectRatio", settings.display.forceAspectRatio },
            }
        }
    };

    std::ofstream stream(TABLET_SETTINGS_FILE);
    stream << std::setw(4) << json;
}

Result<void> migrate_tablet_settings(TabletSettings const& settings)
{
    static auto newSettingsSchema = get_application_config_path() / "tablet_settings.json";
    static auto oldSettingsSchema = get_application_config_path() / "tablet_settings.old.json";

    std::filesystem::rename(TABLET_SETTINGS_FILE, oldSettingsSchema);

    save_tablet_settings(settings);

    TRY(libexec::execute("xdg-open", { get_application_config_path() }, libexec::Mode::DETACHED));
    auto [out, err] = TRY(libexec::execute("git", { "diff", oldSettingsSchema, newSettingsSchema }));

    if (!err.empty()) return make_error(err);

    std::ofstream stream(get_application_config_path() / "conflict.diff");
    stream << out;

    return {};
}
