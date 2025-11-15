#pragma once

#include "app/core/FreeType.hpp"
#include "platform/Environment.hpp"
#include "SettingsError.hpp"

#include <liberror/Result.hpp>

inline std::filesystem::path APPLICATION_SETTINGS_FILE = get_application_config_path() / "application_settings.json";

class ApplicationSettings
{
    // Should be updated every time a change is made
    static constexpr auto SCHEMA_VERSION = "1.1";

public:
    enum class Theme { DARK, LIGHT };

public:
    ApplicationSettings()
        : scale_{1.0}
        , theme_{Theme::DARK}
        , language_{"en_us"}
        , font_{"Default", "Regular", ""}
    {}

    ApplicationSettings(ApplicationSettings&& that)
        : scale_{std::exchange(that.scale_, 0)}
        , theme_{std::exchange(that.theme_, Theme{})}
        , language_{std::move(that.language_)}
        , font_{std::move(that.font_)}
    {}

    ApplicationSettings& operator=(ApplicationSettings&& that)
    {
        this->scale_ = std::exchange(that.scale_, 0);
        this->theme_ = std::exchange(that.theme_, Theme{});
        this->language_ = std::move(that.language_);
        this->font_ = std::move(that.font_);
        return *this;
    }

    ApplicationSettings(ApplicationSettings const& that)
        : scale_{that.scale_}
        , theme_{that.theme_}
        , language_{that.language_}
        , font_{that.font_}
    {}

    ApplicationSettings& operator=(ApplicationSettings const& that)
    {
        this->scale_ = that.scale_;
        this->theme_ = that.theme_;
        this->language_ = that.language_;
        this->font_ = that.font_;
        return *this;
    }

public:
    friend liberror::Result<ApplicationSettings, SettingsError> load_application_settings();
    friend liberror::Result<void, SettingsError> save_application_settings(ApplicationSettings const& settings);

    // cppcheck-suppress [functionStatic, constParameterReference]
    auto& scale(this auto& self) { return self.scale_; }
    void scale(float scale) { scale_ = scale; }

    // cppcheck-suppress [functionStatic, constParameterReference]
    auto& theme(this auto& self) { return self.theme_; }
    void theme(Theme theme) { theme_ = theme; }

    // cppcheck-suppress [functionStatic, constParameterReference]
    auto& language(this auto& self) { return self.language_; }
    void language(std::string const& language) { language_ = language; }

    // cppcheck-suppress [functionStatic, constParameterReference]
    auto& font(this auto& self) { return self.font_; }
    void font(Font const& font) { font_ = font; }

private:
    float scale_;
    Theme theme_;
    std::string language_;
    Font font_;
};

liberror::Result<ApplicationSettings, SettingsError> load_application_settings();
liberror::Result<void, SettingsError> save_application_settings(ApplicationSettings const& settings);
liberror::Result<void, SettingsError> migrate_application_settings(ApplicationSettings const& settings);
