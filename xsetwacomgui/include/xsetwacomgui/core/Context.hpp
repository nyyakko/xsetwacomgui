#pragma once

#include "settings/ApplicationSettings.hpp"
#include "settings/TabletSettings.hpp"
#include "platform/hid/X11/Display.hpp"

struct Settings
{
    ApplicationSettings application;
    TabletSettings tablet;
};

struct Tablet
{
    Device stylus;
    Device pad;
};

struct Context
{
    Settings settings;

    std::vector<Device> devices;
    std::vector<Display> displays;

    Tablet tablet {};
    Display display {};

    bool handleOutdatedDeviceSettings = false;

    bool hasChangedDevice = false;
    bool hasChangedDeviceHandedness = false;
    bool hasChangedDeviceArea = false;
    bool hasChangedDevicePressure = false;
    bool hasChangedDisplay = false;
    bool hasChangedDisplayArea = false;
    bool hasChangedLanguage = false;
    bool hasChangedScale = false;
    bool hasChangedFont = false;
    bool hasChangedFontStyle = false;
    bool hasChangedTheme = false;
    bool hasChangedProfile = false;
};

liberror::Result<void> apply_settings_from_driver_to_context(Context& context);
liberror::Result<void> apply_settings_from_context_to_device(Context const& context);
