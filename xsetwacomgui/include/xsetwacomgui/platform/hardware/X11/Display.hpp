#pragma once

#include <string>
#include <vector>

#include <liberror/Result.hpp>

struct Display
{
    struct Area
    {
        float offsetX, offsetY;
        float width, height;
    };

    int id;
    bool primary;
    Area area;
    std::string name = "";
};

liberror::Result<std::vector<Display>> get_available_displays();
liberror::Result<Display> get_primary_display();
