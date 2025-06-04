#pragma once

#include <filesystem>

#include <libwacom/Device.hpp>

#define SETTINGS_PATH get_settings_base_path() / NAME
#define SETTINGS_FILE SETTINGS_PATH / "devices.json"

struct DeviceSetttings
{
    libwacom::Area& area;
    libwacom::Pressure& pressure;
    bool& forceFullArea;
    bool& forceAspectRatio;
};

std::filesystem::path get_settings_base_path();
bool load_device_settings(DeviceSetttings settings);
bool save_device_settings(DeviceSetttings settings);

