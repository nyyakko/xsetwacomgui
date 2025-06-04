#pragma once

#include <filesystem>

#include <imgui/imgui_internal.hpp>
#include <libwacom/Device.hpp>

#define SETTINGS_PATH get_settings_base_path() / NAME
#define SETTINGS_FILE SETTINGS_PATH / "devices.json"

struct Settings
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

std::filesystem::path get_settings_base_path();
bool load_device_settings(Settings& settings);
bool save_device_settings(Settings const& settings);

