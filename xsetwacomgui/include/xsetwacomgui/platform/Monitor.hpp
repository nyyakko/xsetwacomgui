#pragma once

#include <string>
#include <vector>

#include <liberror/Result.hpp>
#include <libwacom/Device.hpp>

struct Monitor
{
    int id;
    bool primary;
    libwacom::Area area;
    std::string name;
};

liberror::Result<std::vector<Monitor>> get_available_monitors();
liberror::Result<Monitor> get_primary_monitor();
