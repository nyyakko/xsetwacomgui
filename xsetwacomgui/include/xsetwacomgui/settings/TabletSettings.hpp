#pragma once

#include "platform/Environment.hpp"
#include "core/SettingsError.hpp"

#include <imgui/imgui_internal.hpp>
#include <libwacom/Device.hpp>
#include <libenum/Enum.hpp>

inline std::filesystem::path TABLET_SETTINGS_FILE = get_application_config_path() / "tablet_settings.json";

struct DeviceSettings
{
    std::string name = "INVALID";
    libwacom::Handedness handedness = libwacom::Handedness::RIGHT;
    libwacom::Area area = { -1, -1, -1, -1 };
    libwacom::Pressure pressure = { -1, -1, -1, -1 };
    bool forceFullArea = false;
    bool forceAspectRatio = false;
};

struct DisplaySettings
{
    std::string name = "INVALID";
    libwacom::Area area = { -1, -1, -1, -1 };
    bool forceFullArea = false;
    bool forceAspectRatio = false;
};

struct TabletSettings
{
private:
    // Should be updated every time a change is made
    static constexpr auto SCHEMA_VERSION = "1.2";

    friend liberror::Result<void, SettingsError> load_tablet_settings(TabletSettings& settings);
    friend void save_tablet_settings(TabletSettings const& settings);
public:

    DeviceSettings device {};
    DisplaySettings display {};
};

liberror::Result<void, SettingsError> load_tablet_settings(TabletSettings& settings);
void save_tablet_settings(TabletSettings const& settings);
liberror::Result<void> migrate_tablet_settings(TabletSettings const& settings);

