#pragma once

#include "platform/Environment.hpp"
#include "core/SettingsError.hpp"
#include "ui/FreeType.hpp"

#include <imgui/imgui_internal.hpp>
#include <liberror/Result.hpp>

inline std::filesystem::path APPLICATION_SETTINGS_FILE = get_application_config_path() / "application_settings.json";

struct SettingsApplication
{
private:
    // Should be updated every time a change is made
    static constexpr auto SCHEMA_VERSION = "1.1";

    friend liberror::Result<SettingsApplication, SettingsError> load_application_settings();
    friend void save_application_settings(SettingsApplication const& settings);

public:
    enum class Theme { DARK, LIGHT };

    float scale = 1.0;
    Theme theme = Theme::DARK;
    std::string language = "en_us";
    Font font = { "Default", "Regular", "" };
};

liberror::Result<SettingsApplication, SettingsError> load_application_settings();
liberror::Result<void> migrate_application_settings(SettingsApplication const& settings);
void save_application_settings(SettingsApplication const& settings);
