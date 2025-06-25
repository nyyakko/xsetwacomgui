#include "settings/ApplicationSettings.hpp"

#include <filesystem>
#include <liberror/Result.hpp>
#include <nlohmann/json.hpp>
#include <fmt/format.h>

#include <fstream>
#include <sstream>
#include <cstdlib>

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
    static auto newSettingsSchema = get_application_config_path() / "application_settings.json";
    static auto oldSettingsSchema = get_application_config_path() / "application_settings.old.json";

    std::filesystem::rename(APPLICATION_SETTINGS_FILE, oldSettingsSchema);

    save_application_settings(settings);

    popen(fmt::format("xdg-open {}", get_application_config_path().string()).data(), "r");
    pclose(popen(fmt::format("git diff {} {} > {}/conflict.diff", oldSettingsSchema.string(), newSettingsSchema.string(), get_application_config_path().string()).data(), "r"));
}
