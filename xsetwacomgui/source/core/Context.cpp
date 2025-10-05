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
    context.settings.tablet.stylus.name = context.tablet.stylus.name;
    context.settings.tablet.stylus.area = TRY(get_stylus_area(context.tablet.stylus));
    context.settings.tablet.stylus.pressure = TRY(get_stylus_pressure_curve(context.tablet.stylus));
    context.settings.tablet.stylus.forceFullArea = false;
    context.settings.tablet.stylus.forceAspectRatio = false;
    context.settings.tablet.stylus.mappings = TRY(get_device_button_mappings(context.tablet.stylus));

    context.tablet.pad = fplus::keep_if([] (auto&& device) { return device.kind == Device::Kind::PAD; }, context.devices).back();
    context.settings.tablet.pad.mappings = TRY(get_device_button_mappings(context.tablet.pad));
    context.settings.tablet.pad.name = context.tablet.pad.name;

    context.display = TRY(get_primary_display());
    context.hasChangedDisplay = true;
    context.settings.tablet.display.name = context.display.name;
    context.settings.tablet.display.area = { 0, 0, context.display.area.width, context.display.area.height };
    context.settings.tablet.display.forceFullArea = false;
    context.settings.tablet.display.forceAspectRatio = false;

    return {};
}

Result<void> apply_settings_from_context_to_device(Context const& context)
{
    TRY(set_stylus_area(context.tablet.stylus, context.settings.tablet.stylus.area));
    TRY(set_stylus_handedness(context.tablet.stylus, context.settings.tablet.stylus.handedness));
    TRY(set_stylus_pressure_curve(context.tablet.stylus, context.settings.tablet.stylus.pressure));
    auto displayArea = context.settings.tablet.display.area;
    displayArea.offsetX += context.display.area.offsetX;
    displayArea.offsetY += context.display.area.offsetY;
    TRY(set_stylus_output_from_display_area(context.tablet.stylus, displayArea));
    TRY(set_device_button_mappings(context.tablet.stylus, context.settings.tablet.stylus.mappings));
    TRY(set_device_button_mappings(context.tablet.pad, context.settings.tablet.pad.mappings));

    return {};
}

