#include "app/settings/TabletProfile.hpp"

#include "app/devices/Tablet.hpp"

#include <liberror/Try.hpp>

using namespace liberror;

Result<TabletProfile> make_tablet_profile(std::string_view name, Tablet const& tablet, Display const& display)
{
    TabletProfile profile {};

    profile.name = name;

    profile.stylus.name = tablet.stylus.name;
    profile.stylus.handedness = Device::Handedness::RIGHT;
    profile.stylus.area = TRY(get_stylus_default_area(tablet.stylus));
    profile.stylus.pressure = { 0, 0, 1, 1 };
    profile.stylus.forceFullArea = false;
    profile.stylus.forceAspectRatio = false;
    profile.stylus.mappings = TRY(get_device_default_button_mappings(tablet.stylus));

    profile.pad.name = tablet.pad.name;
    profile.pad.mappings = TRY(get_device_default_button_mappings(tablet.pad));

    profile.display.name = display.name;
    profile.display.area = { 0, 0, display.area.width, display.area.height };
    profile.display.forceFullArea = false;
    profile.display.forceAspectRatio = false;

    return profile;
}

Result<void> load_tablet_profile(TabletProfile const& profile, Tablet const& tablet, Display const& display)
{
    TRY(set_stylus_area(tablet.stylus, profile.stylus.area));
    TRY(set_stylus_handedness(tablet.stylus, profile.stylus.handedness));
    TRY(set_stylus_pressure_curve(tablet.stylus, profile.stylus.pressure));
    TRY(set_device_button_mappings(tablet.stylus, profile.stylus.mappings));
    auto displayArea = profile.display.area;
    displayArea.offsetX += display.area.offsetX;
    displayArea.offsetY += display.area.offsetY;
    TRY(set_stylus_output_from_display_area(tablet.stylus, displayArea));

    TRY(set_device_button_mappings(tablet.pad, profile.pad.mappings));

    return {};
}
