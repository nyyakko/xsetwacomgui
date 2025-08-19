#pragma once

#include "platform/Environment.hpp"
#include "core/SettingsError.hpp"
#include "ui/FreeType.hpp"

#include <imgui/imgui_internal.hpp>
#include <libenum/Enum.hpp>
#include <liberror/Result.hpp>

inline std::filesystem::path APPLICATION_SETTINGS_FILE = get_application_config_path() / "application_settings.json";

struct ApplicationSettings
{
private:
    // Should be updated every time a change is made
    static constexpr auto SCHEMA_VERSION = "1.1";

    friend liberror::Result<void, SettingsError> load_application_settings(ApplicationSettings& settings);
    friend void save_application_settings(ApplicationSettings const& settings);
public:
    ENUM_CLASS(Theme, DARK, LIGHT)

    float scale = 1.0;
    Theme theme = Theme::DARK;
    std::string language = "en_us";
    FontInfo font = { "Default", "Regular", "" };
};

liberror::Result<void, SettingsError> load_application_settings(ApplicationSettings& settings);
void save_application_settings(ApplicationSettings const& settings);
liberror::Result<void> migrate_application_settings(ApplicationSettings const& settings);

