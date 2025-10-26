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
        Area area = { -1, -1, -1, -1 };
        Device::Pressure pressure = { -1, -1, -1, -1 };
        bool forceFullArea = false;
        bool forceAspectRatio = false;
        std::map<int, X11Action> mappings = {};
    };

    struct Pad
    {
        std::string name = "INVALID";
        std::map<int, X11Action> mappings = {};
    };

    struct Display
    {
        std::string name = "INVALID";
        ::Area area = { -1, -1, -1, -1 };
        bool forceFullArea = false;
        bool forceAspectRatio = false;
    };

    std::string name = "INVALID";
    Stylus stylus;
    Pad pad;
    Display display;
};

liberror::Result<TabletProfile> make_tablet_profile(std::string_view name, Tablet const& tablet, Display const& display);
liberror::Result<void> load_tablet_profile(TabletProfile const& profile, Tablet const& tablet, Display const& display);
