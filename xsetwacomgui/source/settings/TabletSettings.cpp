#include <spdlog/spdlog.h>

#include "settings/TabletSettings.hpp"

#include "platform/hid/X11/Device.hpp"

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

        for (auto const& profileJson : json["profiles"])
        {
            TabletProfile profile {};

            profile.display.name             = profileJson.begin().value()["display"]["name"].get<std::string>();
            profile.display.forceFullArea    = profileJson.begin().value()["display"]["forceFullArea"].get<bool>();
            profile.display.forceAspectRatio = profileJson.begin().value()["display"]["forceAspectRatio"].get<bool>();
            profile.display.area.offsetX     = profileJson.begin().value()["display"]["area"]["offsetX"].get<float>();
            profile.display.area.offsetY     = profileJson.begin().value()["display"]["area"]["offsetY"].get<float>();
            profile.display.area.width       = profileJson.begin().value()["display"]["area"]["width"].get<float>();
            profile.display.area.height      = profileJson.begin().value()["display"]["area"]["height"].get<float>();

            profile.stylus.name              = profileJson.begin().value()["tablet"]["stylus"]["name"].get<std::string>();
            profile.stylus.handedness        = *magic_enum::enum_cast<Device::Handedness>(profileJson.begin().value()["tablet"]["stylus"]["handedness"].get<std::string>());
            profile.stylus.forceFullArea     = profileJson.begin().value()["tablet"]["stylus"]["forceFullArea"].get<bool>();
            profile.stylus.forceAspectRatio  = profileJson.begin().value()["tablet"]["stylus"]["forceAspectRatio"].get<bool>();
            profile.stylus.area.offsetX      = profileJson.begin().value()["tablet"]["stylus"]["area"]["offsetX"].get<float>();
            profile.stylus.area.offsetY      = profileJson.begin().value()["tablet"]["stylus"]["area"]["offsetY"].get<float>();
            profile.stylus.area.width        = profileJson.begin().value()["tablet"]["stylus"]["area"]["width"].get<float>();
            profile.stylus.area.height       = profileJson.begin().value()["tablet"]["stylus"]["area"]["height"].get<float>();
            profile.stylus.pressure.minX     = profileJson.begin().value()["tablet"]["stylus"]["pressure"]["minX"].get<float>();
            profile.stylus.pressure.minY     = profileJson.begin().value()["tablet"]["stylus"]["pressure"]["minY"].get<float>();
            profile.stylus.pressure.maxX     = profileJson.begin().value()["tablet"]["stylus"]["pressure"]["maxX"].get<float>();
            profile.stylus.pressure.maxY     = profileJson.begin().value()["tablet"]["stylus"]["pressure"]["maxY"].get<float>();

            for (auto const& entry : profileJson.begin().value()["tablet"]["stylus"]["mappings"])
            {
                profile.stylus.mappings.insert({
                    std::atoi(entry.items().begin().key().data()),
                    *magic_enum::enum_cast<X11Action>(entry.items().begin().value().get<std::string>())
                });
            }

            profile.pad.name = profileJson.begin().value()["tablet"]["pad"]["name"].get<std::string>();

            for (auto const& entry : profileJson.begin().value()["tablet"]["pad"]["mappings"])
            {
                profile.pad.mappings.insert({
                    std::atoi(entry.items().begin().key().data()),
                    *magic_enum::enum_cast<X11Action>(entry.items().begin().value().get<std::string>())
                });
            }

            settings.profiles.emplace(profileJson.begin().key(), profile);
        }

        if (!settings.profiles.contains(json["profile"].get<std::string>()))
        {
            return make_error<SettingsError>(SettingsError::Type::PROFILE_NOT_FOUND);
        }

        settings.profile = json["profile"].get<std::string>();
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
        { "profile", settings.profile },
        { "profiles", nlohmann::json::array() }
    };

    for (auto const& [name, profile] : settings.profiles | std::views::filter([] (auto const& profile) { return profile.first != "INVALID"; }))
    {
        nlohmann::ordered_json profileJson {};

        profileJson[name] = {
            {
                "tablet", {
                    {
                        "stylus", {
                            { "name", profile.stylus.name },
                            { "handedness", magic_enum::enum_name<Device::Handedness>(profile.stylus.handedness) },
                            {
                                "area", {
                                    { "offsetX", profile.stylus.area.offsetX },
                                    { "offsetY", profile.stylus.area.offsetY },
                                    { "width", profile.stylus.area.width },
                                    { "height", profile.stylus.area.height }
                                }
                            },
                            {
                                "pressure", {
                                    { "minX", profile.stylus.pressure.minX },
                                    { "minY", profile.stylus.pressure.minY },
                                    { "maxX", profile.stylus.pressure.maxX },
                                    { "maxY", profile.stylus.pressure.maxY },
                                }
                            },
                            { "forceFullArea", profile.stylus.forceFullArea },
                            { "forceAspectRatio", profile.stylus.forceAspectRatio },
                            { "mappings", nlohmann::json::array() }
                        }
                    },
                    {
                        "pad", {
                            { "name", profile.pad.name },
                            { "mappings", nlohmann::json::array() }
                        }
                    },
                }
            },
            {
                "display", {
                    { "name", profile.display.name },
                    {
                        "area", {
                            { "offsetX", profile.display.area.offsetX },
                            { "offsetY", profile.display.area.offsetY },
                            { "width", profile.display.area.width },
                            { "height", profile.display.area.height }
                        }
                    },
                    { "forceFullArea", profile.display.forceFullArea },
                    { "forceAspectRatio", profile.display.forceAspectRatio },
                }
            }
        };

        for (auto const& mapping : profile.stylus.mappings)
        {
            nlohmann::ordered_json mappingJson {};
            mappingJson[std::to_string(mapping.first)] = magic_enum::enum_name<X11Action>(mapping.second);
            profileJson[name]["tablet"]["stylus"]["mappings"].push_back(mappingJson);
        }

        for (auto const& mapping : profile.pad.mappings)
        {
            nlohmann::ordered_json mappingJson {};
            mappingJson[std::to_string(mapping.first)] = magic_enum::enum_name<X11Action>(mapping.second);
            profileJson[name]["tablet"]["pad"]["mappings"].push_back(mappingJson);
        }

        json["profiles"].push_back(profileJson);
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

Result<TabletProfile> make_default_profile(Tablet const& tablet, Display const& display)
{
    TabletProfile profile;

    profile.stylus.name = tablet.stylus.name;
    profile.stylus.handedness = Device::Handedness::RIGHT;
    profile.stylus.area = TRY(get_stylus_default_area(tablet.stylus));
    profile.stylus.pressure = { 0, 0, 1, 1 };
    profile.stylus.forceFullArea = false;
    profile.stylus.forceAspectRatio = false;
    // FIXME: find a way to get the default mappings
    profile.stylus.mappings = TRY(get_device_button_mappings(tablet.stylus));

    profile.pad.name = tablet.pad.name;
    // FIXME: find a way to get the default mappings
    profile.pad.mappings = TRY(get_device_button_mappings(tablet.pad));

    profile.display.name = display.name;
    profile.display.area = { 0, 0, display.area.width, display.area.height };
    profile.display.forceFullArea = false;
    profile.display.forceAspectRatio = false;

    return profile;
}

Result<void> load_profile_to_tablet(TabletProfile const& profile, Tablet const& tablet, Display const& display)
{
    TRY(set_stylus_area(tablet.stylus, profile.stylus.area));
    TRY(set_stylus_handedness(tablet.stylus, profile.stylus.handedness));
    TRY(set_stylus_pressure_curve(tablet.stylus, profile.stylus.pressure));
    TRY(set_device_button_mappings(tablet.stylus, profile.stylus.mappings));
    auto displayArea = profile.display.area;
    displayArea.offsetX += display.area.offsetX;
    displayArea.offsetY += display.area.offsetY;
    TRY(set_stylus_output_from_display_area(tablet.stylus, displayArea));

    TRY(set_device_button_mappings(tablet.pad, profile.pad.mappings));

    return {};
}
