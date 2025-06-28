#include "settings/ApplicationSettings.hpp"

#include <fmt/format.h>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>
#include <libexec/Execute.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
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

        settings.theme       = ApplicationSettings::Theme::from_string((json["appearance"]["theme"].get<std::string>()));
        settings.font.path   = json["appearance"]["font"]["path"].get<std::string>();
        settings.font.family = json["appearance"]["font"]["family"].get<std::string>();
        settings.font.style  = json["appearance"]["font"]["style"].get<std::string>();
        settings.scale       = json["display"]["scale"].get<float>();
        settings.language    = json["language"]["language"].get<std::string>();
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
                { "font", {
                        { "path", settings.font.path },
                        { "family", settings.font.family },
                        { "style", settings.font.style },
                    }
                },
            }
        },
        {
            "display", {
                { "scale", settings.scale },
            }
        },
        {
            "language", {
                { "language", settings.language },
            }
        }
    };

    std::ofstream stream(APPLICATION_SETTINGS_FILE);
    stream << std::setw(4) << json;
}

liberror::Result<void> migrate_application_settings(ApplicationSettings const& settings)
{
    static auto newSettingsSchema = get_application_config_path() / "application_settings.json";
    static auto oldSettingsSchema = get_application_config_path() / "application_settings.old.json";

    std::filesystem::rename(APPLICATION_SETTINGS_FILE, oldSettingsSchema);

    save_application_settings(settings);

    TRY(libexec::execute("xdg-open", { get_application_config_path() }, libexec::Mode::DETACHED));
    auto [out, err] = TRY(libexec::execute("git", { "diff", oldSettingsSchema, newSettingsSchema }));
    std::ofstream stream(get_application_config_path() / "conflict.diff");
    stream << out;

    return {};
}
