#pragma once

#include "platform/hid/X11/Device.hpp"
#include "platform/hid/X11/Display.hpp"

#include <range/v3/algorithm.hpp>

#include <map>

class TabletProfile
{
private:
    struct Stylus
    {
        std::string name = "INVALID";
        Device::Handedness handedness = Device::Handedness::RIGHT;
        Area area;
        Device::Pressure pressure;
        bool forceFullArea;
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
    };

public:
    std::string name = "INVALID";
    Stylus stylus;
    Pad pad;
    Display display;
};

liberror::Result<TabletProfile> make_tablet_profile(std::string_view name, Device const& stylus, Device const& pad, Display const& display);
liberror::Result<void> load_tablet_profile(TabletProfile const& profile, Device const& stylus, Device const& pad, Display const& display);
