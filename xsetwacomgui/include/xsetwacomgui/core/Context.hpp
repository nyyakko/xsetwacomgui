#pragma once

#include "settings/ApplicationSettings.hpp"
#include "settings/TabletSettings.hpp"
#include "platform/Display.hpp"

struct Context
{
    ApplicationSettings& applicationSettings;
    TabletSettings& tabletSettings;

    std::vector<Device>& devices;
    std::vector<Display>& displays;

    Device device {};
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
};
