#pragma once

#include "platform/hid/X11/Device.hpp"
#include "platform/hid/X11/Display.hpp"

struct Tablet;

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

liberror::Result<TabletProfile> make_tablet_profile(std::string_view name, Tablet const& tablet, Display const& display);
liberror::Result<void> load_tablet_profile(TabletProfile const& profile, Tablet const& tablet, Display const& display);
