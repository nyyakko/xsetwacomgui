#pragma once

#include <string>
#include <vector>

#include <liberror/Result.hpp>
#include <libwacom/Device.hpp>

struct Display
{
    int id;
    bool primary;
    libwacom::Area area;
    std::string name;
};

liberror::Result<std::vector<Display>> get_available_displays();
liberror::Result<Display> get_primary_display();
