#pragma once

#include "platform/hid/X11/Area.hpp"

#include <string>
#include <vector>

#include <liberror/Result.hpp>

struct Display
{
    int id;
    bool primary;
    Area area;
    std::string name = "INVALID";
};

liberror::Result<std::vector<Display>> get_available_displays();
liberror::Result<Display> get_primary_display();
