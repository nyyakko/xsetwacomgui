#pragma once

#include "core/SettingsError.hpp"
#include "platform/Device.hpp"
#include "platform/Display.hpp"
#include "platform/Environment.hpp"

#include <imgui/imgui_internal.hpp>

#include <map>

inline std::filesystem::path TABLET_SETTINGS_FILE = get_application_config_path() / "tablet_settings.json";

struct StylusSettings
{
    std::string name = "INVALID";
    Device::Handedness handedness = Device::Handedness::RIGHT;
    Device::Area area = { -1, -1, -1, -1 };
    Device::Pressure pressure = { -1, -1, -1, -1 };
    bool forceFullArea = false;
    bool forceAspectRatio = false;
    std::map<int, X11Action> mappings = {};
};

struct PadSettings
{
    std::string name = "INVALID";
    std::map<int, X11Action> mappings = {};
};

struct DisplaySettings
{
    std::string name = "INVALID";
    Display::Area area = { -1, -1, -1, -1 };
    bool forceFullArea = false;
    bool forceAspectRatio = false;
};

struct TabletSettings
{
private:
    // Should be updated every time a change is made
    static constexpr auto SCHEMA_VERSION = "1.3";

    friend liberror::Result<TabletSettings, SettingsError> load_tablet_settings();
    friend void save_tablet_settings(TabletSettings const& settings);

public:
    StylusSettings stylus {};
    PadSettings pad {};
    DisplaySettings display {};
};

liberror::Result<TabletSettings, SettingsError> load_tablet_settings();
void save_tablet_settings(TabletSettings const& settings);
liberror::Result<void> migrate_tablet_settings(TabletSettings const& settings);
