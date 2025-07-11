#pragma once

#include "settings/ApplicationSettings.hpp"
#include "settings/TabletSettings.hpp"
#include "platform/Display.hpp"

struct Context
{
    ApplicationSettings& applicationSettings;
    TabletSettings& tabletSettings;

    std::vector<libwacom::Device>& devices;
    std::vector<Display>& displays;

    libwacom::Device device {};
    Display display {};

    bool handleOutdatedDeviceSettings = false;

    uint8_t hasChangedDevice = false;
    uint8_t hasChangedDeviceHandedness = false;
    bool hasChangedDeviceArea = false;
    bool hasChangedDevicePressure = false;
    uint8_t hasChangedDisplay = false;
    bool hasChangedDisplayArea = false;
    bool hasChangedLanguage = false;
    bool hasChangedScale = false;
    bool hasChangedFont = false;
    bool hasChangedFontStyle = false;
    bool hasChangedTheme = false;
};
