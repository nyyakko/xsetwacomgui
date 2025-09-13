#include "core/Context.hpp"

#include <fplus/filter.hpp>
#include <liberror/Try.hpp>
#include <magic_enum/magic_enum.hpp>

using namespace liberror;

Result<void> apply_settings_from_driver_to_context(Context& context)
{
    context.tablet.stylus = fplus::keep_if([] (auto&& device) { return device.kind == Device::Kind::STYLUS; }, context.devices).back();
    context.hasChangedDevice = true;
    context.hasChangedDeviceHandedness = true;
    context.tabletSettings.stylus.name = context.tablet.stylus.name;
    context.tabletSettings.stylus.area = TRY(get_stylus_area(context.tablet.stylus));
    context.tabletSettings.stylus.pressure = TRY(get_stylus_pressure_curve(context.tablet.stylus));
    context.tabletSettings.stylus.forceFullArea = false;
    context.tabletSettings.stylus.forceAspectRatio = false;
    context.tabletSettings.stylus.mappings = TRY(get_device_button_mappings(context.tablet.stylus));

    context.tablet.pad = fplus::keep_if([] (auto&& device) { return device.kind == Device::Kind::PAD; }, context.devices).back();
    context.tabletSettings.pad.mappings = TRY(get_device_button_mappings(context.tablet.pad));
    context.tabletSettings.pad.name = context.tablet.pad.name;

    context.display = TRY(get_primary_display());
    context.hasChangedDisplay = true;
    context.tabletSettings.display.name = context.display.name;
    context.tabletSettings.display.area = { 0, 0, context.display.area.width, context.display.area.height };
    context.tabletSettings.display.forceFullArea = false;
    context.tabletSettings.display.forceAspectRatio = false;

    return {};
}

Result<void> apply_settings_from_context_to_device(Context const& context)
{
    TRY(set_stylus_area(context.tablet.stylus, context.tabletSettings.stylus.area));
    TRY(set_stylus_handedness(context.tablet.stylus, context.tabletSettings.stylus.handedness));
    TRY(set_stylus_pressure_curve(context.tablet.stylus, context.tabletSettings.stylus.pressure));
    auto displayArea = context.tabletSettings.display.area;
    displayArea.offsetX += context.display.area.offsetX;
    displayArea.offsetY += context.display.area.offsetY;
    TRY(set_stylus_output_from_display_area(context.tablet.stylus, displayArea));
    TRY(set_device_button_mappings(context.tablet.stylus, context.tabletSettings.stylus.mappings));
    TRY(set_device_button_mappings(context.tablet.pad, context.tabletSettings.pad.mappings));

    return {};
}

