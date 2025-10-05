#pragma once

#include "settings/Settings.hpp"

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
