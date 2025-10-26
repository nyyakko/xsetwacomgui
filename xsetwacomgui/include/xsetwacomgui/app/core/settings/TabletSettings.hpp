#pragma once

#include "platform/Environment.hpp"
#include "SettingsError.hpp"
#include "TabletProfile.hpp"

#include <imgui/imgui_internal.hpp>

#include <algorithm>
#include <map>

inline std::filesystem::path TABLET_SETTINGS_FILE = get_application_config_path() / "tablet_settings.json";

class TabletSettings
{
private:
    // Should be updated every time a change is made
    static constexpr auto SCHEMA_VERSION = "1.4";

    friend liberror::Result<TabletSettings, SettingsError> load_tablet_settings();
    friend void save_tablet_settings(TabletSettings const& settings);

public:
    TabletSettings()
        : profiles_{ { "INVALID", {} } }
        , profile_{}
    {
        profile_ = std::ranges::find_if(this->profiles_, [&] (auto const& entry) {
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
        this->profile_ = std::ranges::find_if(this->profiles_, [&] (auto const& entry) {
            return entry.first == that.profile_->first;
        });
    }

    TabletSettings& operator=(TabletSettings const& that)
    {
        this->profiles_ = that.profiles_;
        this->profile_ = std::ranges::find_if(this->profiles_, [&] (auto const& entry) {
            return entry.first == that.profile_->first;
        });
        return *this;
    }

public:
    inline constexpr auto& profiles(this auto& self) { return self.profiles_; }

    inline constexpr auto& profile(this auto& self)
    {
        assert(self.profile_ != self.profiles_.end());
        return self.profile_;
    }

    inline constexpr void profile(std::map<std::string, TabletProfile>::iterator iterator)
    {
        profile_ = iterator;
    }

private:
    std::map<std::string, TabletProfile> profiles_;
    std::map<std::string, TabletProfile>::iterator profile_;
};

liberror::Result<TabletSettings, SettingsError> load_tablet_settings();
liberror::Result<void> migrate_tablet_settings(TabletSettings const& settings);
void save_tablet_settings(TabletSettings const& settings);
