#pragma once

#include "core/SettingsError.hpp"
#include "platform/Environment.hpp"
#include "platform/hid/X11/Device.hpp"
#include "platform/hid/X11/Display.hpp"

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
    static constexpr auto SCHEMA_VERSION = "1.4";

    friend liberror::Result<TabletSettings, SettingsError> load_tablet_settings();
    friend void save_tablet_settings(TabletSettings const& settings);

public:
    struct Profile
    {
        StylusSettings stylus {};
        PadSettings pad {};
        DisplaySettings display {};

        static liberror::Result<Profile> make_default(Tablet const& tablet, Display const& display);
        static liberror::Result<void> load_to_tablet(Profile const& profile, Tablet const& tablet, Display const& display);
    };

public:
    inline constexpr auto& get_current_profile(this auto& self) { return self.profiles.at(self.profile); }

    inline constexpr auto* operator->() { return &profiles.at(profile); }

public:
    std::string profile = "INVALID";
    std::map<std::string, Profile> profiles { { "INVALID", {} } };
};

liberror::Result<TabletSettings, SettingsError> load_tablet_settings();
void save_tablet_settings(TabletSettings const& settings);
liberror::Result<void> migrate_tablet_settings(TabletSettings const& settings);
