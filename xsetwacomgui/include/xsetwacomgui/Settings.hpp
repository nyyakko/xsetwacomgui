#pragma once

#include "Environment.hpp"

#include <imgui/imgui_internal.hpp>
#include <libwacom/Device.hpp>

#define DEVICE_SETTINGS_FILE SETTINGS_PATH / "device.json"
#define APPLICATION_SETTINGS_FILE SETTINGS_PATH / "application.json"

struct DeviceSettings
{
    std::string deviceName;
    libwacom::Area deviceArea;
    libwacom::Pressure devicePressure;
    bool deviceForceFullArea;
    bool deviceForceAspectRatio;
    std::string monitorName;
    libwacom::Area monitorArea;
    bool monitorForceFullArea;
    bool monitorForceAspectRatio;
};

bool load_device_settings(DeviceSettings& settings);
bool save_device_settings(DeviceSettings const& settings);

struct ApplicationSettings
{
    ENUM_CLASS(Theme, DARK, WHITE)

    float scale;
    Theme theme;
    std::string language;
    std::string font;
};

bool load_application_settings(ApplicationSettings& settings);
bool save_application_settings(ApplicationSettings& settings);
