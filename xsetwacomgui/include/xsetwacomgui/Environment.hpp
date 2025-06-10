#pragma once

#include <cassert>
#include <filesystem>

#ifdef DEBUG
    #define SETTINGS_PATH std::filesystem::path(HOME) / "build" / "debug"
    #define DATA_PATH std::filesystem::path(HOME)
#else
    #define SETTINGS_PATH get_application_config_path()
    #define DATA_PATH get_application_data_path()
#endif

std::filesystem::path get_system_home_path();
std::filesystem::path get_application_config_path();
std::filesystem::path get_application_data_path();

