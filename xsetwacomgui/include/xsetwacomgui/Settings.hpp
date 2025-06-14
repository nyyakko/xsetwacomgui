#pragma once

#include "Environment.hpp"

#include <imgui/imgui_internal.hpp>
#include <libwacom/Device.hpp>

inline std::filesystem::path DEVICE_SETTINGS_FILE = get_application_config_path() / "device.json";
inline std::filesystem::path APPLICATION_SETTINGS_FILE = get_application_config_path() / "application.json";

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
    ENUM_CLASS(Language, EN_US, PT_BR, RU_RU)

    float scale;
    Theme theme;
    Language language;
    std::string font;
};

bool load_application_settings(ApplicationSettings& settings);
bool save_application_settings(ApplicationSettings& settings);
