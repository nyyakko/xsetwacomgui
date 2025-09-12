#include <spdlog/spdlog.h>

#include "settings/TabletSettings.hpp"

#include "platform/hardware/X11/Device.hpp"

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

Result<TabletSettings, SettingsError> load_tablet_settings()
{
    TabletSettings settings {};

    if (!std::filesystem::exists(TABLET_SETTINGS_FILE))
    {
        return make_error<SettingsError>(SettingsError::Type::FILE_NOT_FOUND);
    }

    std::ifstream stream(TABLET_SETTINGS_FILE);
    std::stringstream content;
    content << stream.rdbuf();

    try
    {
        auto json = nlohmann::json::parse(content.str());

        if (json["version"].is_null() || json["version"].get<std::string>() != TabletSettings::SCHEMA_VERSION)
        {
            return make_error<SettingsError>(SettingsError::Type::OUTDATED_SCHEMA);
        }

        settings.display.name             = json["display"]["name"].get<std::string>();
        settings.display.forceFullArea    = json["display"]["forceFullArea"].get<bool>();
        settings.display.forceAspectRatio = json["display"]["forceAspectRatio"].get<bool>();
        settings.display.area.offsetX     = json["display"]["area"]["offsetX"].get<float>();
        settings.display.area.offsetY     = json["display"]["area"]["offsetY"].get<float>();
        settings.display.area.width       = json["display"]["area"]["width"].get<float>();
        settings.display.area.height      = json["display"]["area"]["height"].get<float>();
        settings.stylus.name              = json["tablet"]["stylus"]["name"].get<std::string>();
        settings.stylus.handedness        = *magic_enum::enum_cast<Device::Handedness>(json["tablet"]["stylus"]["handedness"].get<std::string>());
        settings.stylus.forceFullArea     = json["tablet"]["stylus"]["forceFullArea"].get<bool>();
        settings.stylus.forceAspectRatio  = json["tablet"]["stylus"]["forceAspectRatio"].get<bool>();
        settings.stylus.area.offsetX      = json["tablet"]["stylus"]["area"]["offsetX"].get<float>();
        settings.stylus.area.offsetY      = json["tablet"]["stylus"]["area"]["offsetY"].get<float>();
        settings.stylus.area.width        = json["tablet"]["stylus"]["area"]["width"].get<float>();
        settings.stylus.area.height       = json["tablet"]["stylus"]["area"]["height"].get<float>();
        settings.stylus.pressure.minX     = json["tablet"]["stylus"]["pressure"]["minX"].get<float>();
        settings.stylus.pressure.minY     = json["tablet"]["stylus"]["pressure"]["minY"].get<float>();
        settings.stylus.pressure.maxX     = json["tablet"]["stylus"]["pressure"]["maxX"].get<float>();
        settings.stylus.pressure.maxY     = json["tablet"]["stylus"]["pressure"]["maxY"].get<float>();

        for (auto const& entry : json["tablet"]["stylus"]["mappings"])
        {
            settings.stylus.mappings.insert({
                std::atoi(entry.items().begin().key().data()),
                *magic_enum::enum_cast<X11Action>(entry.items().begin().value().get<std::string>())
            });
        }

        settings.pad.name = json["tablet"]["pad"]["name"].get<std::string>();

        for (auto const& entry : json["tablet"]["pad"]["mappings"])
        {
            settings.pad.mappings.insert({
                std::atoi(entry.items().begin().key().data()),
                *magic_enum::enum_cast<X11Action>(entry.items().begin().value().get<std::string>())
            });
        }
    }
    catch (std::exception const& error)
    {
        return make_error<SettingsError>(SettingsError::Type::READ_FAILURE);
    }

    return settings;
}

void save_tablet_settings(TabletSettings const& settings)
{
    nlohmann::ordered_json json {
        { "version", TabletSettings::SCHEMA_VERSION },
        {
            "tablet", {
                {
                    "stylus", {
                        { "name", settings.stylus.name },
                        { "handedness", magic_enum::enum_name<Device::Handedness>(settings.stylus.handedness) },
                        {
                            "area", {
                                { "offsetX", settings.stylus.area.offsetX },
                                { "offsetY", settings.stylus.area.offsetY },
                                { "width", settings.stylus.area.width },
                                { "height", settings.stylus.area.height }
                            }
                        },
                        {
                            "pressure", {
                                { "minX", settings.stylus.pressure.minX },
                                { "minY", settings.stylus.pressure.minY },
                                { "maxX", settings.stylus.pressure.maxX },
                                { "maxY", settings.stylus.pressure.maxY },
                            }
                        },
                        { "forceFullArea", settings.stylus.forceFullArea },
                        { "forceAspectRatio", settings.stylus.forceAspectRatio },
                        { "mappings", nlohmann::json::array() }
                    }
                },
                {
                    "pad", {
                        { "name", settings.pad.name },
                        { "mappings", nlohmann::json::array() }
                    }
                },
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

    for (auto const& mapping : settings.stylus.mappings)
    {
        nlohmann::ordered_json mappingJson {};
        mappingJson[std::to_string(mapping.first)] = magic_enum::enum_name<X11Action>(mapping.second);
        json["tablet"]["stylus"]["mappings"].push_back(mappingJson);
    }

    for (auto const& mapping : settings.pad.mappings)
    {
        nlohmann::ordered_json mappingJson {};
        mappingJson[std::to_string(mapping.first)] = magic_enum::enum_name<X11Action>(mapping.second);
        json["tablet"]["pad"]["mappings"].push_back(mappingJson);
    }

    std::ofstream stream(TABLET_SETTINGS_FILE);
    stream << std::setw(4) << json;
}

Result<void> migrate_tablet_settings(TabletSettings const& settings)
{
    static auto newSettingsSchema = get_application_config_path() / "tablet_settings.json";
    static auto oldSettingsSchema = get_application_config_path() / "tablet_settings.old.json";

    std::filesystem::rename(TABLET_SETTINGS_FILE, oldSettingsSchema);

    save_tablet_settings(settings);

    auto execResult = TRY(libexec::execute("xdg-open", { get_application_config_path() }, libexec::Mode::DETACHED));
    if (!execResult.second.empty()) return make_error(execResult.second);

    execResult = TRY(libexec::execute("git", { "diff", oldSettingsSchema, newSettingsSchema }));
    if (!execResult.second.empty()) return make_error(execResult.second);
    std::ofstream stream(get_application_config_path() / "conflict.diff");
    stream << execResult.first;

    return {};
}
