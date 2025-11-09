#pragma once

#include "platform/Environment.hpp"
#include "platform/hid/X11/Device.hpp"
#include "platform/hid/X11/Display.hpp"
#include "SettingsError.hpp"

#include <range/v3/algorithm.hpp>

#include <map>

inline std::filesystem::path TABLET_SETTINGS_FILE = get_application_config_path() / "tablet_settings.json";

struct TabletProfile
{
    struct Stylus
    {
        std::string name = "INVALID";
        Device::Handedness handedness = Device::Handedness::RIGHT;
        Area area;
        Device::Pressure pressure;
        bool forceFullArea;
        bool forceAspectRatio;
        std::map<int, Action> mappings;
    };

    struct Pad
    {
        std::string name = "INVALID";
        std::map<int, Action> mappings;
    };

    struct Display
    {
        std::string name = "INVALID";
        Area area;
        bool forceFullArea;
        bool forceAspectRatio;
    };

    std::string name = "INVALID";
    Stylus stylus;
    Pad pad;
    Display display;
};

liberror::Result<TabletProfile> make_tablet_profile(std::string_view name, Device const& stylus, Device const& pad, Display const& display);
liberror::Result<void> load_tablet_profile(TabletProfile const& profile, Device const& stylus, Device const& pad, Display const& display);

class TabletSettings
{
    // Should be updated every time a change is made
    static constexpr auto SCHEMA_VERSION = "1.4";

public:
    TabletSettings()
        : profiles_{ { "INVALID", {} } }
        , profile_{}
    {
        profile_ = ranges::find_if(this->profiles_, [&] (auto const& entry) {
            return entry.first == "INVALID";
        });
    }

    TabletSettings(TabletSettings&& that)
        : profiles_{std::move(that.profiles_)}
        , profile_{std::exchange(that.profile_, {})}
    {}

    TabletSettings& operator=(TabletSettings&& that)
    {
        this->profiles_ = std::move(that.profiles_);
        this->profile_ = std::exchange(that.profile_, {});
        return *this;
    }

    TabletSettings(TabletSettings const& that)
        : profiles_{that.profiles_}
        , profile_{}
    {
        this->profile_ = ranges::find_if(this->profiles_, [&] (auto const& entry) {
            return entry.first == that.profile_->first;
        });
    }

    TabletSettings& operator=(TabletSettings const& that)
    {
        this->profiles_ = that.profiles_;
        this->profile_ = ranges::find_if(this->profiles_, [&] (auto const& entry) {
            return entry.first == that.profile_->first;
        });
        return *this;
    }

public:
    friend liberror::Result<TabletSettings, SettingsError> load_tablet_settings();
    friend liberror::Result<void> save_tablet_settings(TabletSettings const& settings);

    // cppcheck-suppress [functionStatic, constParameterReference]
    inline constexpr auto& profiles(this auto& self) { return self.profiles_; }

    // cppcheck-suppress [functionStatic, constParameterReference]
    inline constexpr auto& profile(this auto& self)
    {
        assert(self.profile_ != self.profiles_.end());
        return self.profile_;
    }

    inline constexpr void profile(TabletProfile const& profile) { profile_->second = profile; }

    inline constexpr void profile(std::map<std::string, TabletProfile>::iterator iterator) { profile_ = iterator; }

private:
    std::map<std::string, TabletProfile> profiles_;
    std::map<std::string, TabletProfile>::iterator profile_;
};

liberror::Result<TabletSettings, SettingsError> load_tablet_settings();
liberror::Result<void> save_tablet_settings(TabletSettings const& settings);
liberror::Result<void> migrate_tablet_settings(TabletSettings const& settings);
