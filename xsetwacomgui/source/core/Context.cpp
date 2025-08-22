#include "core/Context.hpp"

#include <liberror/Try.hpp>

using namespace liberror;

Result<void> load_settings_from_driver_to_context(Context& context)
{
    context.device = context.devices.back();
    context.hasChangedDevice = true;
    context.hasChangedDeviceHandedness = true;
    context.tabletSettings.device.name = context.device.name;
    context.tabletSettings.device.area = TRY(get_stylus_area(context.device.id));
    context.tabletSettings.device.pressure = TRY(get_stylus_pressure_curve(context.device.id));
    context.tabletSettings.device.forceFullArea = false;
    context.tabletSettings.device.forceAspectRatio = false;

    context.display = TRY(get_primary_display());
    context.hasChangedDisplay = true;
    context.tabletSettings.display.name = context.display.name;
    context.tabletSettings.display.area = { 0, 0, context.display.area.width, context.display.area.height };
    context.tabletSettings.display.forceFullArea = false;
    context.tabletSettings.display.forceAspectRatio = false;

    return {};
}

Result<void> load_settings_from_context_to_device(Context const& context)
{
    TRY(set_stylus_area(context.device.id, context.tabletSettings.device.area));
    TRY(set_stylus_handedness(context.device.id, context.tabletSettings.device.handedness));
    TRY(set_stylus_pressure_curve(context.device.id, context.tabletSettings.device.pressure));
    auto displayArea = context.tabletSettings.display.area;
    displayArea.offsetX += context.display.area.offsetX;
    displayArea.offsetY += context.display.area.offsetY;
    TRY(set_stylus_output_from_display_area(context.device.id, displayArea));
    return {};
}

