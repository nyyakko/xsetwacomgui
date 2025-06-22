#pragma once

#include "Environment.hpp"

#include <imgui/imgui_internal.hpp>
#include <libwacom/Device.hpp>
#include <libenum/Enum.hpp>

class [[nodiscard]] SettingsError
{
public:
    ENUM_CLASS(Type,
        READ_FAILURE,
        WRITE_FAILURE,
        OUTDATED_SCHEMA
    )

public:
    using message_t = Type;

    constexpr explicit SettingsError(Type reason) : reason { reason } {}

    constexpr  SettingsError() noexcept = default;
    constexpr ~SettingsError() noexcept = default;

    constexpr SettingsError(SettingsError const& error) : reason { error.reason } {}
    constexpr SettingsError(SettingsError&& error) noexcept : reason { std::move(error.reason) } {}

    constexpr SettingsError& operator=(SettingsError&& error) noexcept
    {
        reason = std::move(error.reason);
        return *this;
    }

    constexpr SettingsError& operator=(SettingsError const& error)
    {
        reason = error.reason;
        return *this;
    }

    [[nodiscard]] constexpr auto const& message() const noexcept { return reason; }

private:
    Type reason;
};

inline std::filesystem::path DEVICE_SETTINGS_FILE = get_application_config_path() / "device.json";
inline std::filesystem::path APPLICATION_SETTINGS_FILE = get_application_config_path() / "application.json";

struct DeviceSettings
{
    // Should be updated every time a change is made
    static constexpr auto SCHEMA_VERSION = "1.0";

    std::string deviceName;
    libwacom::Handedness deviceHandedness;
    libwacom::Area deviceArea;
    libwacom::Pressure devicePressure;
    bool deviceForceFullArea;
    bool deviceForceAspectRatio;
    std::string monitorName;
    libwacom::Area monitorArea;
    bool monitorForceFullArea;
    bool monitorForceAspectRatio;
};

liberror::Result<void, SettingsError> load_device_settings(DeviceSettings& settings);
void save_device_settings(DeviceSettings const& settings);
void migrate_device_settings(DeviceSettings const& settings);

struct ApplicationSettings
{
    // Should be updated every time a change is made
    static constexpr auto SCHEMA_VERSION = "1.0";

    ENUM_CLASS(Theme, DARK, LIGHT)
    ENUM_CLASS(Language, EN_US, PT_BR, RU_RU)

    float scale;
    Theme theme;
    Language language;
    std::string font;
};

liberror::Result<void, SettingsError> load_application_settings(ApplicationSettings& settings);
void save_application_settings(ApplicationSettings const& settings);
void migrate_application_settings(ApplicationSettings const& settings);
