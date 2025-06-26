#pragma once

#include "platform/Environment.hpp"
#include "core/SettingsError.hpp"
#include "ui/FreeType.hpp"

#include <imgui/imgui_internal.hpp>
#include <libwacom/Device.hpp>
#include <libenum/Enum.hpp>

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
    ENUM_CLASS(Language, EN_US, PT_BR, RU_RU)

    float scale;
    Theme theme;
    Language language;
    FontInfo font;
};

liberror::Result<void, SettingsError> load_application_settings(ApplicationSettings& settings);
void save_application_settings(ApplicationSettings const& settings);
void migrate_application_settings(ApplicationSettings const& settings);

