#include "app/settings/TabletProfile.hpp"

#include <liberror/Try.hpp>

using namespace liberror;

Result<TabletProfile> make_tablet_profile(std::string_view name, Device const& stylus, Device const& pad, Display const& display)
{
    TabletProfile profile {};

    profile.name = name;

    profile.stylus.name = stylus.name;
    profile.stylus.handedness = Device::Handedness::RIGHT;
    profile.stylus.area = TRY(get_stylus_default_area(stylus));
    profile.stylus.pressure = { 0, 0, 1, 1 };
    profile.stylus.forceFullArea = false;
    profile.stylus.mappings = TRY(get_device_default_button_mappings(stylus));

    profile.pad.name = pad.name;
    profile.pad.mappings = TRY(get_device_default_button_mappings(pad));

    profile.display.name = display.name;
    profile.display.area = { 0, 0, display.area.width, display.area.height };
    profile.display.forceFullArea = false;

    return profile;
}

Result<void> load_tablet_profile(TabletProfile const& profile, Device const& stylus, Device const& pad, Display const& display)
{
    TRY(set_stylus_area(stylus, profile.stylus.area));
    TRY(set_stylus_handedness(stylus, profile.stylus.handedness));
    TRY(set_stylus_pressure_curve(stylus, profile.stylus.pressure));
    TRY(set_device_button_mappings(stylus, profile.stylus.mappings));
    auto displayArea = profile.display.area;
    displayArea.offsetX += display.area.offsetX;
    displayArea.offsetY += display.area.offsetY;
    TRY(set_stylus_output_from_display_area(stylus, displayArea));

    TRY(set_device_button_mappings(pad, profile.pad.mappings));

    return {};
}
