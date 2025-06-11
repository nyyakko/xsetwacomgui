#pragma once

#include <cassert>
#include <filesystem>

std::filesystem::path get_system_home_path();
std::filesystem::path get_application_config_path();
std::filesystem::path get_application_data_path();

