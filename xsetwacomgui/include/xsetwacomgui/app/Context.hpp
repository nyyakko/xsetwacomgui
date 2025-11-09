#pragma once

#include "app/devices/Tablet.hpp"
#include "settings/ApplicationSettings.hpp"

#include <asio.hpp>
#include <liberror/Result.hpp>

struct Context
{
    ApplicationSettings settings {};

    std::vector<Device> devices;
    std::vector<Display> displays;

    asio::io_context stExecutor {8};
    asio::thread_pool mtExecutor {};

    Tablet tablet {};
    Display display {};

    bool handleOutdatedDeviceSettings = false;

    bool hasChangedDeviceSettings = false;
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
