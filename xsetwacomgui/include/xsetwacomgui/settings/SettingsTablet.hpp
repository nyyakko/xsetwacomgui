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

struct TabletProfile
{
    std::string name = "INVALID";
    StylusSettings stylus;
    PadSettings pad;
    DisplaySettings display;
};

liberror::Result<TabletProfile> make_tablet_profile(std::string_view name, Tablet const& tablet, Display const& display);
liberror::Result<void> load_tablet_profile(TabletProfile const& profile, Tablet const& tablet, Display const& display);

class SettingsTablet
{
private:
    // Should be updated every time a change is made
    static constexpr auto SCHEMA_VERSION = "1.4";

public:
    inline constexpr void add_profile(this auto& self, TabletProfile const& profile) { self.profiles_.emplace(profile.name, profile); }

    inline constexpr auto& profiles(this auto& self) { return self.profiles_; }

    inline constexpr auto& profile(this auto& self) { return self.profiles_.at(self.profile_); }
    inline constexpr void profile(this auto& self, std::string_view profile) { assert(self.profiles_.contains(profile.data())); self.profile_ = profile; }

private:
    friend liberror::Result<SettingsTablet, SettingsError> load_tablet_settings();
    friend void save_tablet_settings(SettingsTablet const& settings);

    std::string profile_ = "INVALID";
    std::map<std::string, TabletProfile> profiles_ { { "INVALID", {} } };
};

liberror::Result<SettingsTablet, SettingsError> load_tablet_settings();
liberror::Result<void> migrate_tablet_settings(SettingsTablet const& settings);
void save_tablet_settings(SettingsTablet const& settings);
